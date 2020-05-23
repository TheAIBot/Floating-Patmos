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





/*

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

*/

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
