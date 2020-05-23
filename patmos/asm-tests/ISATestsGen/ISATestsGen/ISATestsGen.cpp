// ISATestsGen.cpp : Defines the entry point for the application.
//

#include <iostream>
#include <fstream>
#include <string>
#include <functional>
#include <limits>
#include <cmath>
#include <charconv>
#include <array>
#include <vector>
#include <cfenv>
#include <random>
#include <tuple>
#include <type_traits>
#include "ISATestsGen.h"

std::string TESTS_DIR_ASM = "../tests/asm";
std::string TESTS_DIR_EXPECTED = "../tests/expected";

class isaTest
{
private:
	std::ofstream asmFile;
	std::ofstream expectedFile;

public:
	isaTest(std::string asmfilepath, std::string expfilepath, std::string filename)
		: asmFile(std::ofstream(asmfilepath + '/' + filename + ".s")),
		expectedFile(std::ofstream(expfilepath + '/' + filename + ".uart"))
	{
		// first instruction is apparently not executed
		asmFile << R"(		nop
        .word 50
        br start
		nop
		nop
		.word   28
#Send r26 to uart
#Wait for uart to be ready
uart1:	add 	r28 = r0, 0xf0080000
		add     r25 = r0, 4
t2:		lwl     r27  = [r28 + 0]
		nop
		btest p1 = r27, r0
	(!p1)	br	t2
		nop
		nop
# Write r26 to uart
		swl	[r28 + 1] = r26
		sri r26 = r26, 8
		sub r25 = r25, 1
		cmpineq p1 = r25, 0
	(p1)	br	t2
		nop
		nop
        retnd
start:  nop
        nop
        nop)" << '\n';
	}

	void addInstr(std::string instr)
	{
		asmFile << instr << '\n';
	}

	template<typename T>
	void setRegister(std::string reg, T value)
	{
		if (reg[0] == 'r')
		{
			setGPReg(reg, value);
		}
		else if (reg[0] == 'f')
		{
			setFloatReg(reg, value);
		}
		else if (reg[0] == 'p')
		{
			setPred(reg, value);
		}
		else if (reg[0] == 's')
		{
			setSpecialReg(reg, value);
		}
		else
		{
			throw std::runtime_error("Wanted to create a tests that expects the register "
				+ reg + " but registers of that type are currently not supported.");
		}
	}

	void setFloatReg(std::string reg, float value)
	{
		if (true || std::isnan(value) || std::isinf(value) || std::fpclassify(value) == FP_SUBNORMAL)
		{
			addInstr("addl r23 = r0, " + std::to_string(reinterpret_cast<uint32_t&>(value)) + " # " + std::to_string(value));
			addInstr("fmvis " + reg + " = r23");
		}
		else
		{
            addInstr("fmvis " + reg + " = r0");
			addInstr("faddsl " + reg + " = " + reg + ", " + std::to_string(value));
		}
	}

	void setGPReg(std::string reg, int32_t value)
	{
		addInstr("addl " + reg + " = r0, " + std::to_string(value));
	}

	void setPred(std::string reg, bool value)
	{
		if (value)
		{
			addInstr("cmpeq " + reg + " = r0, r0");
		}
		else
		{
			addInstr("cmpneq " + reg + " = r0, r0");
		}
	}

	void setSpecialReg(std::string reg, int32_t value)
	{
		addInstr("mts " + reg + " = " + std::to_string(value));
	}

	void expectRegisterValue(std::string reg, int32_t value)
	{
		expectedFile << ((uint8_t)(value >> 0));
		expectedFile << ((uint8_t)(value >> 8));
		expectedFile << ((uint8_t)(value >>16));
		expectedFile << ((uint8_t)(value >>24));
		if (reg[0] == 'r')
		{
			addInstr("add r26 = r0, " + reg);
		}
		else if (reg[0] == 'f')
		{
			addInstr("fmvsi r26 = " + reg);
		}
		else if (reg[0] == 'p')
		{
			addInstr("bcopy r26 = r0, 0, " + reg);
		}
		else if (reg[0] == 's')
		{
			addInstr("mfs r26 = " + reg);
		}
		else
		{
			throw std::runtime_error("Wanted to create a tests that expects the register "
			 + reg + " but registers of that type are currently not supported.");
		}
		
		
		addInstr("callnd uart1");
	}
	void expectRegisterValue(std::string reg, float value)
	{
		expectRegisterValue(reg, reinterpret_cast<int32_t&>(value));
	}

	void close()
	{
		asmFile << "halt" << '\n';
		asmFile << "nop" << '\n';
		asmFile << "nop" << '\n';
		asmFile << "nop" << '\n';
		asmFile.close();
		expectedFile.close();
	}
};

struct mulRes
{
	uint32_t Low;
	uint32_t High;
};

mulRes SignMul(uint32_t a, uint32_t b)
{
	int32_t op1H = ((int32_t)a) >> 16;
	int32_t op2H = ((int32_t)b) >> 16;
	int32_t op1L = a & 0xffff;
	int32_t op2L = b & 0xffff;

	uint32_t mulLL = op1L * op2L;
	uint32_t mulLH = op1L * op2H;
	uint32_t mulHL = op1H * op2L;
	uint32_t mulHH = op1H * op2H;

	int64_t catHHLL = (((uint64_t)mulHH) << 32) | ((uint64_t)mulLL);
	int64_t catHL = (int64_t)(int32_t)mulHL << 16;
	int64_t catLH = (int64_t)(int32_t)mulLH << 16;

	uint64_t res = catHHLL + catHL + catLH;

	mulRes mRes;
	mRes.Low = res;
	mRes.High = res >> 32;

	return mRes;
}

mulRes UnsignMul(uint32_t a, uint32_t b)
{
	int32_t op1H = ((uint32_t)a) >> 16;
	int32_t op2H = ((uint32_t)b) >> 16;
	int32_t op1L = a & 0xffff;
	int32_t op2L = b & 0xffff;

	uint32_t mulLL = op1L * op2L;
	uint32_t mulLH = op1L * op2H;
	uint32_t mulHL = op1H * op2L;
	uint32_t mulHH = op1H * op2H;

	int64_t catHHLL = (((uint64_t)mulHH) << 32) | ((uint64_t)mulLL);
	int64_t catHL = (int64_t)(uint32_t)mulHL << 16;
	int64_t catLH = (int64_t)(uint32_t)mulLH << 16;

	uint64_t res = catHHLL + catHL + catLH;

	mulRes mRes;
	mRes.Low = res;
	mRes.High = res >> 32;

	return mRes;
}


struct regInfo
{
	std::string regName;
	int32_t regIndex;
	bool isReadonly;

	regInfo(std::string name, int32_t index, bool isReadonly = false) : regName(name), regIndex(index), isReadonly(isReadonly)
	{}

	std::string getBaseName()
	{
		return regName[0] + std::to_string(regIndex);
	}
};

std::vector<regInfo> GPRs = {
	regInfo("r0", 0, true),
	regInfo("r1", 1),
	regInfo("r2", 2),
	regInfo("r3", 3),
	regInfo("r4", 4),
	regInfo("r5", 5),
	regInfo("r6", 6),
	regInfo("r7", 7),
	regInfo("r8", 8),
	regInfo("r9", 9),
	regInfo("r10", 10),
	regInfo("r11", 11),
	regInfo("r12", 12),
	regInfo("r13", 13),
	regInfo("r14", 14),
	regInfo("r15", 15),
	regInfo("r16", 16),
	regInfo("r17", 17),
	regInfo("r18", 18),
	regInfo("r19", 19),
	regInfo("r20", 20),
	regInfo("r21", 21),
	regInfo("r22", 22),
	regInfo("r23", 23),
	regInfo("r24", 24),
	regInfo("r25", 25),
	regInfo("r26", 26),
	regInfo("r27", 27),
	regInfo("r28", 28),
	regInfo("r29", 29),
	regInfo("r30", 30),
	regInfo("r31", 31)
};

std::vector<regInfo> FPRs = {
	regInfo("f0", 0),
	regInfo("f1", 1),
	regInfo("f2", 2),
	regInfo("f3", 3),
	regInfo("f4", 4),
	regInfo("f5", 5),
	regInfo("f6", 6),
	regInfo("f7", 7),
	regInfo("f8", 8),
	regInfo("f9", 9),
	regInfo("f10", 10),
	regInfo("f11", 11),
	regInfo("f12", 12),
	regInfo("f13", 13),
	regInfo("f14", 14),
	regInfo("f15", 15),
	regInfo("f16", 16),
	regInfo("f17", 17),
	regInfo("f18", 18),
	regInfo("f19", 19),
	regInfo("f20", 20),
	regInfo("f21", 21),
	regInfo("f22", 22),
	regInfo("f23", 23),
	regInfo("f24", 24),
	regInfo("f25", 25),
	regInfo("f26", 26),
	regInfo("f27", 27),
	regInfo("f28", 28),
	regInfo("f29", 29),
	regInfo("f30", 30),
	regInfo("f31", 31)
};

std::vector<regInfo> PRs = {
	regInfo("p0", 0, true),
	regInfo("p1", 1),
	regInfo("p2", 2),
	regInfo("p3", 3),
	regInfo("p4", 4),
	regInfo("p5", 5),
	regInfo("p6", 6),
	regInfo("p7", 7)
};

std::vector<regInfo> SPRs = {
	regInfo("s0", 0),
	regInfo("s1", 1),
	regInfo("sfcsr", 1),
	regInfo("s2", 2),
	regInfo("sl", 2),
	regInfo("s3", 3),
	regInfo("sh", 3),
	regInfo("s4", 4),
	regInfo("s5", 5),
	regInfo("ss", 5),
	regInfo("s6", 6),
	regInfo("st", 6),
	regInfo("s7", 7),
	regInfo("srb", 7),
	regInfo("s8", 8),
	regInfo("sro", 8),
	regInfo("s9", 9),
	regInfo("sxb", 9),
	regInfo("s10", 10),
	regInfo("sxo", 10),
	regInfo("s11", 11),
	regInfo("s12", 12),
	regInfo("s13", 13),
	regInfo("s14", 14),
	regInfo("s15", 15)
};

regInfo getRandomReg(std::mt19937& rngGen, std::vector<regInfo>& regSrc)
{
	std::uniform_int_distribution<int32_t> distribution(0, regSrc.size() - 1);
	return regSrc[distribution(rngGen)];
}

enum class opSrc {
	RegSrcInt, 
	RegSrcFloat,
	IntImm, 
	FloatImm
};

template<typename T>
struct valueRange
{
	T inclusiveMin;
	T inclusiveMax;

	valueRange(T inclusiveMin, T inclusiveMax) : inclusiveMin(inclusiveMin), inclusiveMax(inclusiveMax)
	{ }
};

class opSource
{
private:
	opSrc src;
	std::vector<regInfo>* regSrc;
	valueRange<int32_t> intRange;
	valueRange<float> floatRange;

public:
	opSource(std::vector<regInfo>& regSrc, valueRange<int32_t> intRange) : src(opSrc::RegSrcInt), regSrc(&regSrc), intRange(intRange), floatRange(valueRange<float>(0, 0))
	{ }

	opSource(std::vector<regInfo>& regSrc, valueRange<float> floatRange) : src(opSrc::RegSrcFloat), regSrc(&regSrc), intRange(valueRange<int32_t>(0, 0)), floatRange(floatRange)
	{ }

	opSource(valueRange<int32_t> intRange) : src(opSrc::IntImm), regSrc(nullptr), intRange(intRange), floatRange(valueRange<float>(0, 0))
	{ }

	opSource(valueRange<float> floatRange) : src(opSrc::FloatImm), regSrc(nullptr), intRange(valueRange<int32_t>(0, 0)), floatRange(floatRange)
	{ }

	opSrc getSrcType()
	{
		return src;
	} 

	regInfo getRandomRegister(std::mt19937& rngGen) const
	{
		return getRandomReg(rngGen, *regSrc);
	}

	template<class T>
	T getRandom(std::mt19937& rngGen) const
	{
		static_assert(true, "getRandom does not support ");
		return 0;
	}

	template<>
	int32_t getRandom<int32_t>(std::mt19937& rngGen) const
	{
		std::uniform_int_distribution<int32_t> distribution(intRange.inclusiveMin, intRange.inclusiveMax);
		return distribution(rngGen);
	}

	template<>
	float getRandom<float>(std::mt19937& rngGen) const
	{
		std::uniform_real_distribution<float> distribution(floatRange.inclusiveMin, floatRange.inclusiveMax);
		return distribution(rngGen);
	}
};

enum class Pipes
{
	First,
	Second,
	Both
};

class baseFormat
{
protected:
	std::string instrName;
	Pipes pipe;

	baseFormat(std::string name, Pipes pipe) : instrName(name), pipe(pipe)
	{ }

public:
	virtual void makeTests(std::string asmfilepath, std::string expfilepath, std::mt19937& rngGen, int32_t testCount) const = 0;
};

template<bool value>
struct fromRegister {};

template<bool fromReg, typename regValueType>
struct sourceData
{
	using srcType = typename opSource;
	using isFromReg = fromRegister<fromReg>;
	using regValueT = regValueType;
};

template<typename FRet, typename... TArgs>
class uni_format : public baseFormat
{
protected:
	std::vector<regInfo>& destRegs;
	std::array<opSource, sizeof...(TArgs)> sources;
	std::function<FRet(typename TArgs::regValueT...)> func;

	template<std::size_t I, typename VType, typename UseReg>
	std::string setRegAndGetsourceAsString(isaTest& test, std::mt19937& rngGen, std::tuple<typename TArgs::regValueT...>& sourceValues) const
	{
		VType value = std::get<I>(sourceValues);
		if constexpr (std::is_same<fromRegister<true>, UseReg>::value)
		{
			const opSource& src = std::get<I>(sources);

			regInfo rndReg = src.getRandomRegister(rngGen);
			test.setRegister(rndReg.regName, value);
			return rndReg.regName;
		}
		else
		{
			return std::to_string(value);
		}
	}

	template<std::size_t... Is>
	std::vector<std::string> setRegsAndGetSourcesAsStrings(isaTest& test, std::mt19937& rngGen, std::tuple<typename TArgs::regValueT...>& sourceValues, std::index_sequence<Is...> sdf) const
	{
		std::vector<std::string> srcStrs;
		(srcStrs.push_back(setRegAndGetsourceAsString<Is, typename TArgs::regValueT, typename TArgs::isFromReg>(test, rngGen, sourceValues)), ...);
		return srcStrs;
	}

	struct testData
	{
		regInfo destReg;
		std::tuple<typename TArgs::regValueT...> sourceValues;
		std::string instr;

		testData() : destReg(regInfo("", 0)), sourceValues(std::tuple<typename TArgs::regValueT...>()), instr("")
		{ }
	};

	testData setRegistersAndGetInstrData(isaTest& test, std::mt19937& rngGen) const
	{
		testData tData;

		tData.destReg = getRandomReg(rngGen, destRegs);

		tData.sourceValues = std::apply([&](const auto&... src) {
			return std::make_tuple(src.getRandom<TArgs::regValueT>(rngGen)...);
			}, sources);

		std::vector<std::string> srcStrs = setRegsAndGetSourcesAsStrings(test, rngGen, tData.sourceValues, std::make_index_sequence<sizeof...(TArgs)>());

		tData.instr = instrName + " " + tData.destReg.regName + " = ";
		for (size_t i = 0; i < srcStrs.size() - 1; i++)
		{
			tData.instr += srcStrs[i] + ", ";
		}
		tData.instr += srcStrs[srcStrs.size() - 1];

		return tData;
	}

public:
	uni_format(std::string name, Pipes pipe, std::vector<regInfo>& destRegs, typename TArgs::srcType... srcs, std::function<FRet(typename TArgs::regValueT...)> func) : baseFormat(name, pipe), destRegs(destRegs), sources({ {srcs...} }), func(func)
	{}

	void makeTests(std::string asmfilepath, std::string expfilepath, std::mt19937& rngGen, int32_t testCount) const override
	{
		if constexpr (std::is_fundamental<FRet>::value)
		{
			for (size_t i = 0; i < testCount; i++)
			{
				isaTest test(asmfilepath, expfilepath, instrName + "-" + std::to_string(i));

				testData tData = setRegistersAndGetInstrData(test, rngGen);

				test.addInstr(tData.instr);
				test.expectRegisterValue(tData.destReg.regName, std::apply(func, tData.sourceValues));

				test.close();
			}
		}
	}
};

valueRange<int32_t> wholeIntRange(std::numeric_limits<int32_t>::min(), std::numeric_limits<int32_t>::max());
opSource gprWholeIntRange(GPRs, wholeIntRange);


class ALUr_format : public uni_format<int32_t, sourceData<true, int32_t>, sourceData<true, int32_t>>
{
public:
	ALUr_format(std::string name, std::function<int32_t(int32_t, int32_t)> func) : uni_format(
		name, Pipes::Both, GPRs,
		gprWholeIntRange,
		gprWholeIntRange,
		func)
	{}
};

class ALUi_format : public uni_format<int32_t, sourceData<true, int32_t>, sourceData<false, int32_t>>
{
public:
	ALUi_format(std::string name, std::function<int32_t(int32_t, int32_t)> func) : uni_format(
		name, Pipes::Both, GPRs, 
		gprWholeIntRange,
		opSource(valueRange<int32_t>(0, (1 << 12) - 1)), 
		func)
	{}
};

class ALUl_format : public uni_format<int32_t, sourceData<true, int32_t>, sourceData<false, int32_t>>
{
public:
	ALUl_format(std::string name, std::function<int32_t(int32_t, int32_t)> func) : uni_format(
		name, Pipes::First, GPRs,
		gprWholeIntRange,
		opSource(wholeIntRange),
		func)
	{}
};

class ALUm_format : public uni_format<mulRes, sourceData<true, int32_t>, sourceData<true, int32_t>>
{
public:
	ALUm_format(std::string name, std::function<mulRes(uint32_t, uint32_t)> func) : uni_format(
		name, Pipes::First, GPRs,
		gprWholeIntRange,
		gprWholeIntRange,
		func)
	{}

	void makeTests(std::string asmfilepath, std::string expfilepath, std::mt19937& rngGen, int32_t testCount) const override
	{
		for (size_t i = 0; i < testCount; i++)
		{
			isaTest test(asmfilepath, expfilepath, instrName + "-" + std::to_string(i));

			testData tData = setRegistersAndGetInstrData(test, rngGen);

			test.addInstr(tData.instr);
			test.addInstr("nop");
			test.addInstr("nop");
			test.addInstr("nop");
			test.expectRegisterValue("sl", (int32_t)std::apply(func, tData.sourceValues).Low);
			test.expectRegisterValue("sh", (int32_t)std::apply(func, tData.sourceValues).High);

			test.close();
		}
	}
};

class ALUc_format : public uni_format<bool, sourceData<true, int32_t>, sourceData<true, int32_t>>
{
public:
	ALUc_format(std::string name, std::function<bool(int32_t, int32_t)> func) : uni_format(
		name, Pipes::Both, PRs,
		gprWholeIntRange,
		gprWholeIntRange,
		func)
	{}
};

class ALUci_format : public uni_format<bool, sourceData<true, int32_t>, sourceData<false, int32_t>>
{
public:
	ALUci_format(std::string name, std::function<bool(int32_t, int32_t)> func) : uni_format(
		name, Pipes::Both, PRs,
		gprWholeIntRange,
		opSource(valueRange<int32_t>(0, (1 << 5) - 1)),
		func)
	{}
};

class ALUp_format : public uni_format<bool, sourceData<true, bool>, sourceData<true, bool>>
{
public:
	ALUp_format(std::string name, std::function<bool(bool, bool)> func) : uni_format(
		name, Pipes::Both, PRs,
		opSource(PRs, valueRange<int32_t>(0, 1)),
		opSource(PRs, valueRange<int32_t>(0, 1)),
		func)
	{}
};

class ALUb_format : public uni_format<int32_t, sourceData<true, int32_t>, sourceData<false, int32_t>, sourceData<true, bool>>
{
public:
	ALUb_format(std::string name, std::function<int32_t(int32_t, int32_t, bool)> func) : uni_format(
		name, Pipes::Both, GPRs,
		gprWholeIntRange,
		opSource(valueRange<int32_t>(0, (1 << 5) - 1)),
		opSource(PRs, valueRange<int32_t>(0, 1)),
		func)
	{}
};

class SPCt_format : public uni_format<int32_t, sourceData<true, int32_t>>
{
public:
	SPCt_format(std::string name) : uni_format(
		name, Pipes::Both, SPRs,
		gprWholeIntRange,
		[](int32_t a) { return a; })
	{}
};

class SPCf_format : public uni_format<int32_t, sourceData<true, int32_t>>
{
public:
	SPCf_format(std::string name) : uni_format(
		name, Pipes::Both, GPRs,
		opSource(SPRs, wholeIntRange),
		[](int32_t a) { return a; })
	{}
};



void makeFPUrTest(std::string instrName, std::function<float(float, float)> op)
{
	isaTest test(TESTS_DIR_ASM, TESTS_DIR_EXPECTED, instrName);
	float f1 = 17.0f;
	float f2 = 1.0f;
	float f3 = 3.27f;
	float f4 = -5.78;
	float f5 = 3.141592f;
	test.setFloatReg("f1", f1);
	test.setFloatReg("f2", f2);
	test.setFloatReg("f3", f3);
	test.setFloatReg("f4", f4);
	test.setFloatReg("f5", f5);
	test.addInstr(instrName + " f10 = f1, f2");
	test.addInstr(instrName + " f11 = f2, f3");
	test.addInstr(instrName + " f12 = f3, f4");
	test.addInstr(instrName + " f13 = f4, f5");
	test.expectRegisterValue("f10", op(f1, f2));
	test.expectRegisterValue("f11", op(f2, f3));
	test.expectRegisterValue("f12", op(f3, f4));
	test.expectRegisterValue("f13", op(f4, f5));
	test.close();
}

void makeFPUlTest(std::string instrName, std::function<float(float, float)> op)
{
	isaTest test(TESTS_DIR_ASM, TESTS_DIR_EXPECTED, instrName);
	float f1 = 17.0f;
	float f2 = 1.0f;
	float f3 = 3.27f;
	float f4 = -5.78;
	float f5 = 3.141592f;
	test.setFloatReg("f1", f1);
	test.setFloatReg("f2", f2);
	test.setFloatReg("f3", f3);
	test.setFloatReg("f4", f4);
	test.setFloatReg("f5", f5);
	test.addInstr(instrName + " f10 = f1, " + std::to_string(f2));
	test.addInstr(instrName + " f11 = f2, " + std::to_string(f3));
	test.addInstr(instrName + " f12 = f3, " + std::to_string(f4));
	test.addInstr(instrName + " f13 = f4, " + std::to_string(f5));
	test.expectRegisterValue("f10", op(f1, f2));
	test.expectRegisterValue("f11", op(f2, f3));
	test.expectRegisterValue("f12", op(f3, f4));
	test.expectRegisterValue("f13", op(f4, f5));
	test.close();
}

void makeFPUrsTest(std::string instrName, std::function<float(float)> op)
{
	isaTest test(TESTS_DIR_ASM, TESTS_DIR_EXPECTED, instrName);
	float f1 = 17.0f;
	float f2 = 1.0f;
	float f3 = 3.27f;
	float f4 = 123.78;
	float f5 = 3.141592f;
	test.setFloatReg("f1", f1);
	test.setFloatReg("f2", f2);
	test.setFloatReg("f3", f3);
	test.setFloatReg("f4", f4);
	test.setFloatReg("f5", f5);
	test.addInstr(instrName + " f10 = f1");
	test.addInstr(instrName + " f11 = f2");
	test.addInstr(instrName + " f12 = f3");
	test.addInstr(instrName + " f13 = f4");
	test.addInstr(instrName + " f14 = f5");
	test.expectRegisterValue("f10", op(f1));
	test.expectRegisterValue("f11", op(f2));
	test.expectRegisterValue("f12", op(f3));
	test.expectRegisterValue("f13", op(f4));
	test.expectRegisterValue("f14", op(f5));
	test.close();
}

void makeFPCtTest(std::string instrName, std::function<float(int32_t)> op)
{
	isaTest test(TESTS_DIR_ASM, TESTS_DIR_EXPECTED, instrName);
	int32_t r1 = 17;
	int32_t r2 = 1;
	int32_t r3 = 3;
	int32_t r4 = -5;
	int32_t r5 = 33245;
	test.setGPReg("r1", r1);
	test.setGPReg("r2", r2);
	test.setGPReg("r3", r3);
	test.setGPReg("r4", r4);
	test.setGPReg("r5", r5);
	test.addInstr(instrName + " f10 = r1");
	test.addInstr(instrName + " f11 = r2");
	test.addInstr(instrName + " f12 = r3");
	test.addInstr(instrName + " f13 = r4");
	test.addInstr(instrName + " f14 = r5");
	test.expectRegisterValue("f10", op(r1));
	test.expectRegisterValue("f11", op(r2));
	test.expectRegisterValue("f12", op(r3));
	test.expectRegisterValue("f13", op(r4));
	test.expectRegisterValue("f14", op(r5));
	test.close();
}

void makeFPCfTest(std::string instrName, std::function<int32_t(float)> op)
{
	isaTest test(TESTS_DIR_ASM, TESTS_DIR_EXPECTED, instrName);
	float f1 = 17.0f;
	float f2 = 1.0f;
	float f3 = 3.27f;
	float f4 = -5.78;
	float f5 = 3.141592f;
	test.setFloatReg("f10", f1);
	test.setFloatReg("f11", f2);
	test.setFloatReg("f12", f3);
	test.setFloatReg("f13", f4);
	test.setFloatReg("f14", f5);
	test.addInstr(instrName + " r10 = f10");
	test.addInstr(instrName + " r11 = f11");
	test.addInstr(instrName + " r12 = f12");
	test.addInstr(instrName + " r13 = f13");
	test.addInstr(instrName + " r14 = f14");
	test.expectRegisterValue("r10", op(f1));
	test.expectRegisterValue("r11", op(f2));
	test.expectRegisterValue("r12", op(f3));
	test.expectRegisterValue("r13", op(f4));
	test.expectRegisterValue("r14", op(f5));
	test.close();
}

int32_t classifyFloat(float a)
{
	int32_t ai = reinterpret_cast<int32_t&>(a);
	bool isQuietNan = (ai & (1 << 22)) != 0;

	return (std::isnan(a) && !isQuietNan) << 9 |
		   (std::isnan(a) && isQuietNan) << 8 |
		   (std::signbit(a) && std::isinf(a)) << 7 |
		   (std::signbit(a) && std::isnormal(a)) << 6 |
		   (std::signbit(a) && std::fpclassify(a) == FP_SUBNORMAL) << 5 |
		   (std::signbit(a) && a == 0.0) << 4 |
		   (!std::signbit(a) && a == 0.0) << 3 |
		   (!std::signbit(a) && std::fpclassify(a) == FP_SUBNORMAL) << 2 |
		   (!std::signbit(a) && std::isnormal(a)) << 1 |
		   (!std::signbit(a) && std::isinf(a)) << 0;
}

void makeClassifyTest(std::string instrName)
{
	isaTest test(TESTS_DIR_ASM, TESTS_DIR_EXPECTED, instrName + "2");
	float f1 = std::numeric_limits<float>::signaling_NaN();
	float f2 = std::numeric_limits<float>::quiet_NaN();
	float f3 = std::copysign(std::numeric_limits<float>::infinity(), -1.0f);
	float f4 = -3.5f;
	float f5 = std::copysign(std::numeric_limits<float>::denorm_min(), -1.0f);
	float f6 = -0.0f;
	float f7 = 0.0f;
	float f8 = std::copysign(std::numeric_limits<float>::denorm_min(), 1.0f);
	float f9 = 3.5f;
	float f10 = std::copysign(std::numeric_limits<float>::infinity(), 1.0f);
	test.setFloatReg("f1", f1);
	test.setFloatReg("f2", f2);
	test.setFloatReg("f3", f3);
	test.setFloatReg("f4", f4);
	test.setFloatReg("f5", f5);
	test.setFloatReg("f6", f6);
	test.setFloatReg("f7", f7);
	test.setFloatReg("f8", f8);
	test.setFloatReg("f9", f9);
	test.setFloatReg("f10", f10);
	test.addInstr(instrName + " r10 = f1");
	test.addInstr(instrName + " r11 = f2");
	test.addInstr(instrName + " r12 = f3");
	test.addInstr(instrName + " r13 = f4");
	test.addInstr(instrName + " r14 = f5");
	test.addInstr(instrName + " r15 = f6");
	test.addInstr(instrName + " r16 = f7");
	test.addInstr(instrName + " r17 = f8");
	test.addInstr(instrName + " r18 = f9");
	test.addInstr(instrName + " r19 = f10");
	test.expectRegisterValue("r10", classifyFloat(f1));
	test.expectRegisterValue("r11", classifyFloat(f2));
	test.expectRegisterValue("r12", classifyFloat(f3));
	test.expectRegisterValue("r13", classifyFloat(f4));
	test.expectRegisterValue("r14", classifyFloat(f5));
	//test.expectRegisterValue("r15", classifyFloat(f6));
	test.expectRegisterValue("r16", classifyFloat(f7));
	test.expectRegisterValue("r17", classifyFloat(f8));
	test.expectRegisterValue("r18", classifyFloat(f9));
	test.expectRegisterValue("r19", classifyFloat(f10));
	test.close();
}

void makeFPUcTest(std::string instrName, std::function<bool(float, float)> op)
{
	isaTest test(TESTS_DIR_ASM, TESTS_DIR_EXPECTED, instrName);
	float f1 = 17.0f;
	float f2 = 1.0f;
	float f3 = 3.27f;
	float f4 = -5.78;
	float f5 = 3.141592f;
	test.setFloatReg("f1", f1);
	test.setFloatReg("f2", f2);
	test.setFloatReg("f3", f3);
	test.setFloatReg("f4", f4);
	test.setFloatReg("f5", f5);
	test.addInstr(instrName + " p1 = f1, f2");
	test.addInstr(instrName + " p2 = f2, f3");
	test.addInstr(instrName + " p3 = f3, f4");
	test.addInstr(instrName + " p4 = f4, f5");
	test.addInstr(instrName + " p5 = f5, f5");
	test.expectRegisterValue("p1", op(f1, f2));
	test.expectRegisterValue("p2", op(f2, f3));
	test.expectRegisterValue("p3", op(f3, f4));
	test.expectRegisterValue("p4", op(f4, f5));
	test.expectRegisterValue("p5", op(f5, f5));
	test.close();
}

int main(int argc, char const *argv[])
{
	//if (argc != 3)
	//{
	//	std::cerr << "Usage: ISATestsGen <output .s> <output expected>" << std::endl;
	//	return 1;
	//}

	//TESTS_DIR_ASM = argv[1];
	//TESTS_DIR_EXPECTED = argv[2];

	TESTS_DIR_ASM = "C:/Users/Andreas/Documents/GitHub/Floating-Patmos/patmos/asm-tests/ISATestsGen/out/build/x64-Debug/test-asm";
	TESTS_DIR_EXPECTED = "C:/Users/Andreas/Documents/GitHub/Floating-Patmos/patmos/asm-tests/ISATestsGen/out/build/x64-Debug/expected-uart";

	std::cout << "Generating tests..." << std::endl;
    
    #pragma STDC FENV_ACCESS ON
    std::fesetround(FE_TONEAREST);

	std::vector<baseFormat*> instrs = {
		// ALUr
		new ALUr_format("add", std::plus<int32_t>()),
		new ALUr_format("sub", std::minus<int32_t>()),
		new ALUr_format("xor", std::bit_xor<int32_t>()),
		new ALUr_format("sl", [](int32_t a, int32_t b) { return a << (b & 0x1f); }),
		new ALUr_format("sr", [](int32_t a, int32_t b) { return ((uint32_t)a) >> (b & 0x1f); }),
		new ALUr_format("sra", [](int32_t a, int32_t b) { return a >> (b & 0x1f); }),
		new ALUr_format("or", std::bit_or<int32_t>()),
		new ALUr_format("and", std::bit_and<int32_t>()),
		new ALUr_format("nor", [](int32_t a, int32_t b) { return ~(a | b); }),
		new ALUr_format("shadd", [](int32_t a, int32_t b) { return (a << 1) + b; }),
		new ALUr_format("shadd2", [](int32_t a, int32_t b) { return (a << 2) + b; }),

		// ALUi
		new ALUi_format("addi", std::plus<int32_t>()),
		new ALUi_format("subi", std::minus<int32_t>()),
		new ALUi_format("xori", std::bit_xor<int32_t>()),
		new ALUi_format("sli", [](int32_t a, int32_t b) { return a << (b & 0x1f); }),
		new ALUi_format("sri", [](int32_t a, int32_t b) { return ((uint32_t)a) >> (b & 0x1f); }),
		new ALUi_format("srai", [](int32_t a, int32_t b) { return a >> (b & 0x1f); }),
		new ALUi_format("ori", std::bit_or<int32_t>()),
		new ALUi_format("andi", std::bit_and<int32_t>()),

		// ALUl
		new ALUl_format("addl", std::plus<int32_t>()),
		new ALUl_format("subl", std::minus<int32_t>()),
		new ALUl_format("xorl", std::bit_xor<int32_t>()),
		new ALUl_format("sll", [](int32_t a, int32_t b) { return a << (b & 0x1f); }),
		new ALUl_format("srl", [](int32_t a, int32_t b) { return ((uint32_t)a) >> (b & 0x1f); }),
		new ALUl_format("sral", [](int32_t a, int32_t b) { return a >> (b & 0x1f); }),
		new ALUl_format("orl", std::bit_or<int32_t>()),
		new ALUl_format("andl", std::bit_and<int32_t>()),
		new ALUl_format("norl", [](int32_t a, int32_t b) { return ~(a | b); }),
		new ALUl_format("shaddl", [](int32_t a, int32_t b) { return (a << 1) + b; }),
		new ALUl_format("shadd2l", [](int32_t a, int32_t b) { return (a << 2) + b; }),

		// ALUm
		new ALUm_format("mul" , SignMul),
		new ALUm_format("mulu", UnsignMul),

		// ALUc
		new ALUc_format("cmpeq", std::equal_to()),
		new ALUc_format("cmpneq", std::not_equal_to()),
		new ALUc_format("cmplt", std::less()),
		new ALUc_format("cmple", std::less_equal()),
		new ALUc_format("cmpult", std::less<uint32_t>()),
		new ALUc_format("cmpule", std::less_equal<uint32_t>()),
		new ALUc_format("btest", [](int32_t a, int32_t b) { return (a & (1 << b)) != 0; }),

		// ALUci
		new ALUci_format("cmpieq", std::equal_to()),
		new ALUci_format("cmpineq", std::not_equal_to()),
		new ALUci_format("cmpilt", std::less()),
		new ALUci_format("cmpile", std::less_equal()),
		new ALUci_format("cmpiult", std::less<uint32_t>()),
		new ALUci_format("cmpiule", std::less_equal<uint32_t>()),
		new ALUci_format("btesti", [](int32_t a, int32_t b) { return (a & (1 << b)) != 0; }),

		// ALUp
		new ALUp_format("por" , std::logical_or()),
		new ALUp_format("pand", std::logical_and()),
		new ALUp_format("pxor", std::not_equal_to()),

		// ALUb
		new ALUb_format("bcopy", [](int32_t a, int32_t b, bool pred) { return (a & ~(1 << b)) | (pred << b); }),

		// SPCt
		new SPCt_format("mts"),

		// SPCf
		new SPCf_format("mfs"),
	};

	std::mt19937 rngGen(37);

	for each (baseFormat* instr in instrs)
	{
		instr->makeTests(TESTS_DIR_ASM, TESTS_DIR_EXPECTED, rngGen, 10);
	}

	//// FPUr tests
	//makeFPUrTest("fadds", std::plus<float>());
	//makeFPUrTest("fsubs", std::minus<float>());
	//makeFPUrTest("fmuls", std::multiplies<float>());
	//makeFPUrTest("fdivs", std::divides<float>());
	//makeFPUrTest("fsgnjs", std::copysignf);
	//makeFPUrTest("fsgnjns", [](float a, float b) { return std::copysignf(a, (!std::signbit(b)) ? -0.0f : +0.0f); });
	//makeFPUrTest("fsgnjxs", [](float a, float b) { return std::copysignf(a, (std::signbit(a) != std::signbit(b)) ? -0.0f : +0.0f); });
 //   
	//// FPUl tests
	//makeFPUlTest("faddsl", std::plus<float>());
	//makeFPUlTest("fsubsl", std::minus<float>());
	//makeFPUlTest("fmulsl", std::multiplies<float>());
	//makeFPUlTest("fdivsl", std::divides<float>());

	//// FPUrs tests
	//makeFPUrsTest("fsqrts", [](float a) { return std::sqrt(a); });
 //   
	//// FPCt tests
	//makeFPCtTest("fcvtis", [](int32_t a) { return static_cast<float>(a); });
	//makeFPCtTest("fcvtus", [](int32_t a) { return static_cast<float>(static_cast<uint32_t>(a)); });
	//makeFPCtTest("fmvis", [](int32_t a) { return reinterpret_cast<float&>(a); });

	//// FPCf tests
	//makeFPCfTest("fcvtsi", [](float a) { return static_cast<int32_t>(a); });
	//makeFPCfTest("fcvtsu", [](float a) { return static_cast<uint32_t>(a); });
	//makeFPCfTest("fmvsi", [](float a) { return reinterpret_cast<int32_t&>(a); });
	//makeFPCfTest("fclasss", classifyFloat);
	//makeClassifyTest("fclasss");
 //   
	////FPUc tests
	//makeFPUcTest("feqs", [](float a, float b) { return a == b; });
	//makeFPUcTest("flts", [](float a, float b) { return a < b; });
	//makeFPUcTest("fles", [](float a, float b) { return a <= b; });
	

	std::cout << "Tests generated." << std::endl;

	return 0;
}
