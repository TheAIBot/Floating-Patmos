﻿// ISATestsGen.cpp : Defines the entry point for the application.
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

#pragma STDC FENV_ACCESS ON
#pragma fenv_access (on)



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


void makeClassifyTest(isaTest& test)
{
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
	test.setRegister("f1", f1);
	test.setRegister("f2", f2);
	test.setRegister("f3", f3);
	test.setRegister("f4", f4);
	test.setRegister("f5", f5);
	test.setRegister("f6", f6);
	test.setRegister("f7", f7);
	test.setRegister("f8", f8);
	test.setRegister("f9", f9);
	test.setRegister("f10", f10);
	test.addInstr("fclasss r10 = f1");
	test.addInstr("fclasss r11 = f2");
	test.addInstr("fclasss r12 = f3");
	test.addInstr("fclasss r13 = f4");
	test.addInstr("fclasss r14 = f5");
	test.addInstr("fclasss r15 = f6");
	test.addInstr("fclasss r16 = f7");
	test.addInstr("fclasss r17 = f8");
	test.addInstr("fclasss r18 = f9");
	test.addInstr("fclasss r19 = f10");
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

const std::vector<rounding::x86_round_mode> float_to_int_rounding = {
	rounding::x86_round_mode::RTZ
};

const std::vector<std::function<void(isaTest&)>> fclass_special_test = {
	makeClassifyTest
};

int main(int argc, char const *argv[])
{
	//if (argc != 3)
	//{
	//	std::cerr << "Usage: ISATestsGen <output .s> <output expected>" << std::endl;
	//	return 1;
	//}

	//TESTS_DIR_ASM = argv[1];
	//TESTS_DIR_EXPECTED = argv[2];

	std::string TESTS_DIR_ASM = "C:/Users/Andreas/Documents/GitHub/Floating-Patmos/patmos/asm-tests/ISATestsGen/out/build/x64-Debug/test-asm";
	std::string TESTS_DIR_EXPECTED = "C:/Users/Andreas/Documents/GitHub/Floating-Patmos/patmos/asm-tests/ISATestsGen/out/build/x64-Debug/expected-uart";



	std::cout << "Generating tests..." << std::endl;
    

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

		// FPUr
		new FPUr_format("fadds", std::plus<float>()),
		new FPUr_format("fsubs", std::minus<float>()),
		new FPUr_format("fmuls", std::multiplies<float>()),
		new FPUr_format("fdivs", std::divides<float>()),
		new FPUr_format("fsgnjs", std::copysignf),
		new FPUr_format("fsgnjns", [](float a, float b) { return std::copysignf(a, (!std::signbit(b)) ? -0.0f : +0.0f); }),
		new FPUr_format("fsgnjxs", [](float a, float b) { return std::copysignf(a, (std::signbit(a) != std::signbit(b)) ? -0.0f : +0.0f); }),

		// FPUl
		new FPUl_format("faddsl", std::plus<float>()),
		new FPUl_format("fsubsl", std::minus<float>()),
		new FPUl_format("fmulsl", std::multiplies<float>()),
		new FPUl_format("fdivsl", std::divides<float>()),

		// FPUrs
		new FPUrs_format("fsqrts", [](float a) { return std::sqrt(a); }),

		// FPCt
		new FPCt_format("fcvtis", [](int32_t a) { return static_cast<float>(a); }),
		new FPCt_format("fcvtus", [](int32_t a) { return static_cast<float>(static_cast<uint32_t>(a)); }),
		new FPCt_format("fmvis", [](int32_t a) { return reinterpret_cast<float&>(a); }),

		// FPCf
		new FPCf_format("fcvtsi", [](float a) { return static_cast<int32_t>(a); }, no_special_tests, float_to_int_rounding),
		new FPCf_format("fcvtsu", [](float a) { return static_cast<uint32_t>(a); }, no_special_tests, float_to_int_rounding),
		new FPCf_format("fmvsi", [](float a) { return reinterpret_cast<int32_t&>(a); }),
		new FPCf_format("fclasss", classifyFloat, fclass_special_test),

		//FPUc
		new FPUc_format("feqs", std::equal_to<float>()),
		new FPUc_format("flts", std::less<float>()),
		new FPUc_format("fles", std::less_equal<float>()),
	};

	std::mt19937 rngGen(37);
	for each (baseFormat* instr in instrs)
	{
		instr->makeTests(TESTS_DIR_ASM, TESTS_DIR_EXPECTED, rngGen, 10);
	}	

	std::cout << "Tests generated." << std::endl;

	return 0;
}
