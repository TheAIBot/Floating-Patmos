#pragma once

#include "common.h"
#include <string>
#include <array>
#include <tuple>
#include <cstdint>

class opSource
{
private:
	const std::vector<regInfo>* regSrc;
	valueRange<int32_t> intRange;
	valueRange<float> floatRange;

public:
	opSource(const std::vector<regInfo>& regSrc, valueRange<int32_t> intRange) : regSrc(&regSrc), intRange(intRange), floatRange(valueRange<float>(0, 0))
	{ }

	opSource(const std::vector<regInfo>& regSrc, valueRange<float> floatRange) : regSrc(&regSrc), intRange(valueRange<int32_t>(0, 0)), floatRange(floatRange)
	{ }

	opSource(valueRange<int32_t> intRange) : regSrc(nullptr), intRange(intRange), floatRange(valueRange<float>(0, 0))
	{ }

	opSource(valueRange<float> floatRange) : regSrc(nullptr), intRange(valueRange<int32_t>(0, 0)), floatRange(floatRange)
	{ }

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

template<bool fromReg, typename regValueType>
struct sourceData
{
	static const bool isFromReg = fromReg;
	using srcType = typename opSource;
	using regValueT = regValueType;
};

template<typename FRet, typename... TArgs>
class uni_format : public baseFormat
{
protected:
	const std::vector<regInfo>& destRegs;
	std::array<opSource, sizeof...(TArgs)> sources;
	std::function<FRet(typename TArgs::regValueT...)> func;

	template<std::size_t I, typename VType, bool UseReg>
	std::string setRegAndGetsourceAsString(isaTest& test, std::mt19937& rngGen, std::tuple<typename TArgs::regValueT...>& sourceValues) const
	{
		VType value = std::get<I>(sourceValues);
		if constexpr (UseReg)
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
		(srcStrs.push_back(setRegAndGetsourceAsString<Is, typename TArgs::regValueT, TArgs::isFromReg>(test, rngGen, sourceValues)), ...);
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
	uni_format(std::string name, Pipes pipe, const std::vector<regInfo>& destRegs, typename TArgs::srcType... srcs, std::function<FRet(typename TArgs::regValueT...)> func) : baseFormat(name, pipe), destRegs(destRegs), sources({ {srcs...} }), func(func)
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
opSource gprWholeIntRange(registers::GPRs, wholeIntRange);


class ALUr_format : public uni_format<int32_t, sourceData<true, int32_t>, sourceData<true, int32_t>>
{
public:
	ALUr_format(std::string name, std::function<int32_t(int32_t, int32_t)> func) : uni_format(
		name, Pipes::Both, registers::GPRs,
		gprWholeIntRange,
		gprWholeIntRange,
		func)
	{}
};

class ALUi_format : public uni_format<int32_t, sourceData<true, int32_t>, sourceData<false, int32_t>>
{
public:
	ALUi_format(std::string name, std::function<int32_t(int32_t, int32_t)> func) : uni_format(
		name, Pipes::Both, registers::GPRs,
		gprWholeIntRange,
		opSource(valueRange<int32_t>(0, (1 << 12) - 1)),
		func)
	{}
};

class ALUl_format : public uni_format<int32_t, sourceData<true, int32_t>, sourceData<false, int32_t>>
{
public:
	ALUl_format(std::string name, std::function<int32_t(int32_t, int32_t)> func) : uni_format(
		name, Pipes::First, registers::GPRs,
		gprWholeIntRange,
		opSource(wholeIntRange),
		func)
	{}
};

class ALUm_format : public uni_format<mulRes, sourceData<true, int32_t>, sourceData<true, int32_t>>
{
public:
	ALUm_format(std::string name, std::function<mulRes(uint32_t, uint32_t)> func) : uni_format(
		name, Pipes::First, registers::GPRs,
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
		name, Pipes::Both, registers::PRs,
		gprWholeIntRange,
		gprWholeIntRange,
		func)
	{}
};

class ALUci_format : public uni_format<bool, sourceData<true, int32_t>, sourceData<false, int32_t>>
{
public:
	ALUci_format(std::string name, std::function<bool(int32_t, int32_t)> func) : uni_format(
		name, Pipes::Both, registers::PRs,
		gprWholeIntRange,
		opSource(valueRange<int32_t>(0, (1 << 5) - 1)),
		func)
	{}
};

class ALUp_format : public uni_format<bool, sourceData<true, bool>, sourceData<true, bool>>
{
public:
	ALUp_format(std::string name, std::function<bool(bool, bool)> func) : uni_format(
		name, Pipes::Both, registers::PRs,
		opSource(registers::PRs, valueRange<int32_t>(0, 1)),
		opSource(registers::PRs, valueRange<int32_t>(0, 1)),
		func)
	{}
};

class ALUb_format : public uni_format<int32_t, sourceData<true, int32_t>, sourceData<false, int32_t>, sourceData<true, bool>>
{
public:
	ALUb_format(std::string name, std::function<int32_t(int32_t, int32_t, bool)> func) : uni_format(
		name, Pipes::Both, registers::GPRs,
		gprWholeIntRange,
		opSource(valueRange<int32_t>(0, (1 << 5) - 1)),
		opSource(registers::PRs, valueRange<int32_t>(0, 1)),
		func)
	{}
};

class SPCt_format : public uni_format<int32_t, sourceData<true, int32_t>>
{
public:
	SPCt_format(std::string name) : uni_format(
		name, Pipes::Both, registers::SPRs,
		gprWholeIntRange,
		[](int32_t a) { return a; })
	{}
};

class SPCf_format : public uni_format<int32_t, sourceData<true, int32_t>>
{
public:
	SPCf_format(std::string name) : uni_format(
		name, Pipes::Both, registers::GPRs,
		opSource(registers::SPRs, wholeIntRange),
		[](int32_t a) { return a; })
	{}
};