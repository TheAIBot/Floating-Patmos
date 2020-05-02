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
		asmFile << ".word 100" << '\n';
		asmFile << "nop" << '\n'; // first instruction is apparently not executed
	}

	void addInstr(std::string instr)
	{
		asmFile << instr << '\n';
	}

	void setFloatReg(std::string reg, float value)
	{
		addInstr("fmvis " + reg + " = r0");
		if (std::isnan(value) || std::isinf(value) || std::fpclassify(value) == FP_SUBNORMAL)
		{
			addInstr("addl r26 = r0, " + std::to_string(reinterpret_cast<uint32_t&>(value)));
			addInstr("fmvis " + reg + " = r26");
		}
		else
		{
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

		asmFile << R"(		.word   28
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
        retnd)" << '\n';
		asmFile.close();
		expectedFile.close();
	}
};

void make_Xd_Rs1_Rs2_Instr(std::string instrName, std::string regType, std::function<int32_t(int32_t, int32_t)> op)
{
	isaTest test(TESTS_DIR_ASM, TESTS_DIR_EXPECTED, instrName);
	std::vector<int32_t> values{ 0, 1, 3, -5, 27893 };
	for (size_t x = 0; x < values.size(); x++)
	{
		for (size_t y = 0; y < values.size(); y++)
		{
			test.setGPReg("r1", values[x]);
			test.setGPReg("r2", values[y]);
			test.addInstr(instrName + " " + regType + "3 = r1, r2");
			test.expectRegisterValue(regType + "3", op(values[x], values[y]));
		}
	}
	test.close();
}

void make_Xd_Rs1_Imm_Instr(std::string instrName, std::string regType, std::function<int32_t(int32_t, int32_t)> op, std::vector<int32_t>& immValues)
{
	isaTest test(TESTS_DIR_ASM, TESTS_DIR_EXPECTED, instrName);
	std::vector<int32_t> valuesA{ 0, 1, 3, -5, 27893 };
	for (size_t x = 0; x < valuesA.size(); x++)
	{
		for (size_t y = 0; y < immValues.size(); y++)
		{
			test.setGPReg("r1", valuesA[x]);
			test.addInstr(instrName + " " + regType + "3 = r1, " + std::to_string(immValues[y]));
			test.expectRegisterValue(regType + "3", op(valuesA[x], immValues[y]));
		}
	}
	test.close();
}

void makeALUrTest(std::string instrName, std::function<int32_t(int32_t, int32_t)> op)
{
	make_Xd_Rs1_Rs2_Instr(instrName, "r", op);
}

void makeALUiTest(std::string instrName, std::function<int32_t(int32_t, int32_t)> op)
{
	std::vector<int32_t> immValues{ 0, 1, 3, 273, 794 };
	make_Xd_Rs1_Imm_Instr(instrName, "r", op, immValues);
}

void makeALUlTest(std::string instrName, std::function<int32_t(int32_t, int32_t)> op)
{
	std::vector<int32_t> immValues{ 85, 217389, -7, -1 };
	make_Xd_Rs1_Imm_Instr(instrName, "r", op, immValues);
}

void makeALUmTest(std::string instrName, std::function<int32_t(uint32_t, uint32_t)> opL, std::function<int32_t(uint32_t, uint32_t)> opH)
{
	isaTest test(TESTS_DIR_ASM, TESTS_DIR_EXPECTED, instrName);
	std::vector<int32_t> values{ 0, 1, 3, -5, 27893 };
	for (size_t x = 0; x < values.size(); x++)
	{
		for (size_t y = 0; y < values.size(); y++)
		{
			test.setGPReg("r1", values[x]);
			test.setGPReg("r2", values[y]);
			test.addInstr(instrName + " r1, r2");
			test.expectRegisterValue("sl", opL(values[x], values[y]));
			test.expectRegisterValue("sh", opH(values[x], values[y]));
		}
	}
	test.close();
}

void makeALUcTest(std::string instrName, std::function<int32_t(int32_t, int32_t)> op)
{
	make_Xd_Rs1_Rs2_Instr(instrName, "p", op);
}

void makeALUciTest(std::string instrName, std::function<int32_t(int32_t, int32_t)> op)
{
	std::vector<int32_t> immValues{ 0, 1, 3, 17, 29 };
	make_Xd_Rs1_Imm_Instr(instrName, "p", op, immValues);
}

void makeALUpTest(std::string instrName, std::function<bool(bool, bool)> op)
{
	isaTest test(TESTS_DIR_ASM, TESTS_DIR_EXPECTED, instrName);
	std::vector<bool> values{ true, false };
	for (size_t x = 0; x < values.size(); x++)
	{
		for (size_t y = 0; y < values.size(); y++)
		{
			test.setPred("p1", values[x]);
			test.setPred("p2", values[y]);
			test.addInstr(instrName + " p3 = p1, p2");
			test.expectRegisterValue("p3", op(values[x], values[y]));
		}
	}
	test.close();
}

void makeALUbTest(std::string instrName, std::function<int32_t(int32_t, int32_t, bool)> op)
{
	isaTest test(TESTS_DIR_ASM, TESTS_DIR_EXPECTED, instrName);
	std::vector<int32_t> valuesA{ 85, 217389, -7, -1 };
	std::vector<int32_t> immValues{ 0, 3, 29 };
	std::vector<bool> predValues{ true, false };
	for (size_t x = 0; x < valuesA.size(); x++)
	{
		for (size_t y = 0; y < immValues.size(); y++)
		{
			for (size_t z = 0; z < predValues.size(); z++)
			{
				test.setGPReg("r1", valuesA[x]);
				test.setPred("p2", predValues[y]);
				test.addInstr(instrName + " r3 = r1, " + std::to_string(immValues[y]) + ", p2");
				test.expectRegisterValue("r3", op(valuesA[x], immValues[y], predValues[y]));
			}
		}
	}
	test.close();
}

void makeSPCtfTest(std::string instrName)
{
	isaTest test(TESTS_DIR_ASM, TESTS_DIR_EXPECTED, instrName);
	std::vector<int32_t> values{ 85, 217389, -7, -1 };
	for (size_t x = 0; x < values.size(); x++)
	{
		test.setGPReg("r1", values[x]);
		test.addInstr(instrName + " s3 = r1");
		test.expectRegisterValue("s3", values[x]);
	}
	test.close();
}

void makeBranchDelayTest(std::string instrName, int32_t delay)
{
	isaTest test(TESTS_DIR_ASM, TESTS_DIR_EXPECTED, instrName);
	std::vector<int32_t> values{ 0, 3, 17 };
	for (int32_t x = 0; x < values.size(); x++)
	{
		for (size_t y = 0; y < delay; y++)
		{
			test.setGPReg("r" + std::to_string(y + 1), x + 1);
		}
		test.setGPReg("r" + std::to_string(delay + 1), x + 1);

		test.addInstr(instrName + " x" + std::to_string(x));

		for (size_t y = 0; y < delay; y++)
		{
			test.setGPReg("r" + std::to_string(y + 1), y + 2 + x);
		}
		test.setGPReg("r" + std::to_string(delay + 1), delay + 2 + x);

		for (size_t y = 0; y < values[x]; y++)
		{
			test.addInstr("nop");
		}

		test.addInstr("x" + std::to_string(x) + ": nop");

		for (int32_t y = 0; y < delay; y++)
		{
			test.expectRegisterValue("r" + std::to_string(y + 1), y + 2 + x);
		}
		test.expectRegisterValue("r" + std::to_string(delay + 1), x + 1);

	}
	test.close();
}

void makeBranchNoDelayTest(std::string instrName)
{
	int delay = 2;
	isaTest test(TESTS_DIR_ASM, TESTS_DIR_EXPECTED, instrName);
	std::vector<int32_t> values{ 0, 3, 17 };
	for (int32_t x = 0; x < values.size(); x++)
	{
		for (size_t y = 0; y < delay; y++)
		{
			test.setGPReg("r" + std::to_string(y + 1), x + 1);
		}
		test.setGPReg("r" + std::to_string(delay + 1), x + 1);

		test.addInstr(instrName + " x" + std::to_string(x));

		for (size_t y = 0; y < delay; y++)
		{
			test.setGPReg("r" + std::to_string(y + 1), y + 2 + x);
		}
		test.setGPReg("r" + std::to_string(delay + 1), delay + 2 + x);

		for (size_t y = 0; y < values[x]; y++)
		{
			test.addInstr("nop");
		}

		test.addInstr("x" + std::to_string(x) + ": nop");

		for (int32_t y = 0; y < delay; y++)
		{
			test.expectRegisterValue("r" + std::to_string(y + 1), x + 1);
		}
		test.expectRegisterValue("r" + std::to_string(delay + 1), x + 1);

	}
	test.close();
}

void makeFPUrTest(std::string instrName, std::function<float(float, float)> op)
{
	isaTest test(TESTS_DIR_ASM, TESTS_DIR_EXPECTED, instrName);
	float f1 = 0.0f;
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
	float f1 = 0.0f;
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
	float f1 = 0.0f;
	float f2 = 1.0f;
	float f3 = 3.27f;
	float f4 = -5.78;
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
	int32_t r1 = 0;
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
	float f1 = 0.0f;
	float f2 = 1.0f;
	float f3 = 3.27f;
	float f4 = -5.78;
	float f5 = 3.141592f;
	test.setFloatReg("f1", f1);
	test.setFloatReg("f2", f2);
	test.setFloatReg("f3", f3);
	test.setFloatReg("f4", f4);
	test.setFloatReg("f5", f5);
	test.addInstr(instrName + " r10 = f1");
	test.addInstr(instrName + " r11 = f2");
	test.addInstr(instrName + " r12 = f3");
	test.addInstr(instrName + " r13 = f4");
	test.addInstr(instrName + " r14 = f5");
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
	float f1 = 0.0f;
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
	if (argc != 3)
	{
		std::cerr << "Usage: ISATestsGen <output .s> <output expected>" << std::endl;
		return 1;
	}

	TESTS_DIR_ASM = argv[1];
	TESTS_DIR_EXPECTED = argv[2];

	std::cout << "Generating tests..." << std::endl;

	// ALUr
	makeALUrTest("add", std::plus<int32_t>());
	makeALUrTest("sub", std::minus<int32_t>());
	makeALUrTest("xor", std::bit_xor<int32_t>());
	makeALUrTest("sl", [](int32_t a, int32_t b) { return a << (b & 0xf); });
	makeALUrTest("sr", [](int32_t a, int32_t b) { return ((uint32_t)a) >> (b & 0xf); });
	makeALUrTest("sra", [](int32_t a, int32_t b) { return a >> (b & 0xf); });
	makeALUrTest("or", std::bit_or<int32_t>());
	makeALUrTest("and", std::bit_and<int32_t>());
	makeALUrTest("nor", [](int32_t a, int32_t b) { return ~(a | b); });
	makeALUrTest("shadd", [](int32_t a, int32_t b) { return (a << 1) + b; });
	makeALUrTest("shadd2", [](int32_t a, int32_t b) { return (a << 2) + b; });

	// ALUi
	makeALUiTest("addi", std::plus<int32_t>());
	makeALUiTest("subi", std::minus<int32_t>());
	makeALUiTest("xori", std::bit_xor<int32_t>());
	makeALUiTest("sli", [](int32_t a, int32_t b) { return a << (b & 0xf); });
	makeALUiTest("sri", [](int32_t a, int32_t b) { return ((uint32_t)a) >> (b & 0xf); });
	makeALUiTest("srai", [](int32_t a, int32_t b) { return a >> (b & 0xf); });
	makeALUiTest("ori", std::bit_or<int32_t>());
	makeALUiTest("andi", std::bit_and<int32_t>());
	//these don't actually exist even though the book say they do
	//makeALUiTest("nori", [](int32_t a, int32_t b) { return ~(a | b); });
	//makeALUiTest("shaddi", [](int32_t a, int32_t b) { return (a << 1) + b; });
	//makeALUiTest("shadd2i", [](int32_t a, int32_t b) { return (a << 2) + b; });

	// ALUl
	makeALUiTest("addl", std::plus<int32_t>());
	makeALUiTest("subl", std::minus<int32_t>());
	makeALUiTest("xorl", std::bit_xor<int32_t>());
	makeALUiTest("sll", [](int32_t a, int32_t b) { return a << (b & 0xf); });
	makeALUiTest("srl", [](int32_t a, int32_t b) { return ((uint32_t)a) >> (b & 0xf); });
	makeALUiTest("sral", [](int32_t a, int32_t b) { return a >> (b & 0xf); });
	makeALUiTest("orl", std::bit_or<int32_t>());
	makeALUiTest("andl", std::bit_and<int32_t>());
	makeALUiTest("norl", [](int32_t a, int32_t b) { return ~(a | b); });
	makeALUiTest("shaddl", [](int32_t a, int32_t b) { return (a << 1) + b; });
	makeALUiTest("shadd2l", [](int32_t a, int32_t b) { return (a << 2) + b; });

	// ALUm
	makeALUmTest("mul" , [](uint32_t a, uint32_t b) { return (int64_t)a * (int64_t)b; }, [](uint32_t a, uint32_t b) { return (int64_t)(((int64_t)a * (int64_t)b)) >> 32; });
	makeALUmTest("mulu", [](uint32_t a, uint32_t b) { return (uint64_t)a * (uint64_t)b; }, [](uint32_t a, uint32_t b) { return (uint64_t)(((uint64_t)a * (uint64_t)b)) >> 32; });

	// ALUc
	makeALUcTest("cmpeq", [](int32_t a, int32_t b) { return a == b; });
	makeALUcTest("cmpneq", [](int32_t a, int32_t b) { return a != b; });
	makeALUcTest("cmplt", [](int32_t a, int32_t b) { return a < b; });
	makeALUcTest("cmple", [](int32_t a, int32_t b) { return a <= b; });
	makeALUcTest("cmpult", [](int32_t a, int32_t b) { return (uint32_t)a < (uint32_t)b; });
	makeALUcTest("cmpule", [](int32_t a, int32_t b) { return (uint32_t)a <= (uint32_t)b; });
	makeALUcTest("btest", [](int32_t a, int32_t b) { return (a & (1 << b)) != 0; });

	// ALUci
	makeALUciTest("cmpieq", [](int32_t a, int32_t b) { return a == b; });
	makeALUciTest("cmpineq", [](int32_t a, int32_t b) { return a != b; });
	makeALUciTest("cmpilt", [](int32_t a, int32_t b) { return a < b; });
	makeALUciTest("cmpile", [](int32_t a, int32_t b) { return a <= b; });
	makeALUciTest("cmpiult", [](int32_t a, int32_t b) { return (uint32_t)a < (uint32_t)b; });
	makeALUciTest("cmpiule", [](int32_t a, int32_t b) { return (uint32_t)a <= (uint32_t)b; });
	makeALUciTest("btesti", [](int32_t a, int32_t b) { return (a & (1 << b)) != 0; });

	// ALUp
	makeALUpTest("por" , [](bool a, bool b) { return a || b; });
	makeALUpTest("pand", [](bool a, bool b) { return a && b; });
	makeALUpTest("pxor", [](bool a, bool b) { return a != b; });

	// ALUb
	makeALUbTest("bcopy", [](int32_t a, int32_t b, bool pred) { return (a & ~(1 << b)) | (pred << b); });

	// SPCt and SPCf
	makeSPCtfTest("mts");

	// CFLi Delayed
	makeBranchDelayTest("call", 3);
	makeBranchDelayTest("br", 2);
	makeBranchDelayTest("brcf", 3);

	// CFLi Non delayed
	makeBranchNoDelayTest("callnd");
	makeBranchNoDelayTest("brnd");
	makeBranchNoDelayTest("brcfnd");

	// FPUr tests
	makeFPUrTest("fadds", std::plus<float>());
	makeFPUrTest("fsubs", std::minus<float>());
	makeFPUrTest("fmuls", std::multiplies<float>());
	makeFPUrTest("fdivs", std::divides<float>());
	makeFPUrTest("fsgnjs", std::copysignf);
	makeFPUrTest("fsgnjns", [](float a, float b) { return std::copysignf(a, (!std::signbit(b)) ? -0.0f : +0.0f); });
	makeFPUrTest("fsgnjxs", [](float a, float b) { return std::copysignf(a, (std::signbit(a) != std::signbit(b)) ? -0.0f : +0.0f); });

	// FPUl tests
	makeFPUlTest("faddsl", std::plus<float>());
	makeFPUlTest("fsubsl", std::minus<float>());
	makeFPUlTest("fmulsl", std::multiplies<float>());
	makeFPUlTest("fdivsl", std::divides<float>());

	// FPUrs tests
	makeFPUrsTest("fsqrts", [](float a) { return std::sqrt(a); });

	// FPCt tests
	makeFPCtTest("fcvtis", [](int32_t a) { return static_cast<float>(a); });
	makeFPCtTest("fcvtus", [](int32_t a) { return static_cast<float>(static_cast<uint32_t>(a)); });
	makeFPCtTest("fmvis", [](int32_t a) { return reinterpret_cast<float&>(a); });

	// FPCf tests
	makeFPCfTest("fcvtsi", [](float a) { return static_cast<int32_t>(a); });
	makeFPCfTest("fcvtsu", [](float a) { return static_cast<uint32_t>(a); });
	makeFPCfTest("fmvsi", [](float a) { return reinterpret_cast<int32_t&>(a); });
	makeFPCfTest("fclasss", classifyFloat);
	makeClassifyTest("fclasss");

	//FPUc tests
	makeFPUcTest("feqs", [](float a, float b) { return a == b; });
	makeFPUcTest("flts", [](float a, float b) { return a < b; });
	makeFPUcTest("fles", [](float a, float b) { return a <= b; });

	std::cout << "Tests generated." << std::endl;

	return 0;
}
