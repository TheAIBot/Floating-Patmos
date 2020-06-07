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
#include <filesystem>
#include "ISATestsGen.h"

#pragma STDC FENV_ACCESS ON
#pragma fenv_access (on)

namespace patmos
{
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
		mRes.Low = (uint32_t)res;
		mRes.High = (uint32_t)(res >> 32);

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
		mRes.Low = (uint32_t)res;
		mRes.High = (uint32_t)(res >> 32);

		return mRes;
	}

	uint32_t fcvtsu_func(float a)
	{
		if (a < 0.0f)
		{
			std::fexcept_t exceptions = FE_UNDERFLOW;
			if (fesetexceptflag(&exceptions, FE_UNDERFLOW) != 0)
			{
				std::runtime_error("Failed to set floating-point exceptions.");
			}

			return 0;
		}
		else
		{
			return static_cast<uint32_t>(a);
		}
	}

	int32_t classifyFloat(float a)
	{
		int32_t ai = fti(a);
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

	std::vector<baseFormat*> instrs = {
		// ALUr
		new ALUr_format("add"   , 0b0000, std::plus<int32_t>()),
		new ALUr_format("sub"   , 0b0001, std::minus<int32_t>()),
		new ALUr_format("xor"   , 0b0010, std::bit_xor<int32_t>()),
		new ALUr_format("sl"    , 0b0011, [](int32_t a, int32_t b) { return a << (b & 0x1f); }),
		new ALUr_format("sr"    , 0b0100, [](int32_t a, int32_t b) { return ((uint32_t)a) >> (b & 0x1f); }),
		new ALUr_format("sra"   , 0b0101, [](int32_t a, int32_t b) { return a >> (b & 0x1f); }),
		new ALUr_format("or"    , 0b0110, std::bit_or<int32_t>()),
		new ALUr_format("and"   , 0b0111, std::bit_and<int32_t>()),
		new ALUr_format("nor"   , 0b1011, [](int32_t a, int32_t b) { return ~(a | b); }),
		new ALUr_format("shadd" , 0b1100, [](int32_t a, int32_t b) { return (a << 1) + b; }),
		new ALUr_format("shadd2", 0b1101, [](int32_t a, int32_t b) { return (a << 2) + b; }),

		// ALUi
		new ALUi_format("addi", 0b0000, std::plus<int32_t>()),
		new ALUi_format("subi", 0b0001, std::minus<int32_t>()),
		new ALUi_format("xori", 0b0010, std::bit_xor<int32_t>()),
		new ALUi_format("sli" , 0b0011, [](int32_t a, int32_t b) { return a << (b & 0x1f); }),
		new ALUi_format("sri" , 0b0100, [](int32_t a, int32_t b) { return ((uint32_t)a) >> (b & 0x1f); }),
		new ALUi_format("srai", 0b0101, [](int32_t a, int32_t b) { return a >> (b & 0x1f); }),
		new ALUi_format("ori" , 0b0110, std::bit_or<int32_t>()),
		new ALUi_format("andi", 0b0111, std::bit_and<int32_t>()),

		// ALUl
		new ALUl_format("addl"   , 0b0000, std::plus<int32_t>()),
		new ALUl_format("subl"   , 0b0001, std::minus<int32_t>()),
		new ALUl_format("xorl"   , 0b0010, std::bit_xor<int32_t>()),
		new ALUl_format("sll"    , 0b0011, [](int32_t a, int32_t b) { return a << (b & 0x1f); }),
		new ALUl_format("srl"    , 0b0100, [](int32_t a, int32_t b) { return ((uint32_t)a) >> (b & 0x1f); }),
		new ALUl_format("sral"   , 0b0101, [](int32_t a, int32_t b) { return a >> (b & 0x1f); }),
		new ALUl_format("orl"    , 0b0110, std::bit_or<int32_t>()),
		new ALUl_format("andl"   , 0b0111, std::bit_and<int32_t>()),
		new ALUl_format("norl"   , 0b1011, [](int32_t a, int32_t b) { return ~(a | b); }),
		new ALUl_format("shaddl" , 0b1100, [](int32_t a, int32_t b) { return (a << 1) + b; }),
		new ALUl_format("shadd2l", 0b1101, [](int32_t a, int32_t b) { return (a << 2) + b; }),

		// ALUm
		new ALUm_format("mul" , 0b0000, SignMul),
		new ALUm_format("mulu", 0b0001, UnsignMul),

		// ALUc
		new ALUc_format("cmpeq" , 0b0000, std::equal_to()),
		new ALUc_format("cmpneq", 0b0001, std::not_equal_to()),
		new ALUc_format("cmplt" , 0b0010, std::less()),
		new ALUc_format("cmple" , 0b0011, std::less_equal()),
		new ALUc_format("cmpult", 0b0100, std::less<uint32_t>()),
		new ALUc_format("cmpule", 0b0101, std::less_equal<uint32_t>()),
		new ALUc_format("btest" , 0b0110, [](int32_t a, int32_t b) { return (a & (1 << b)) != 0; }),

		// ALUci
		new ALUci_format("cmpieq" , 0b0000, std::equal_to()),
		new ALUci_format("cmpineq", 0b0001, std::not_equal_to()),
		new ALUci_format("cmpilt" , 0b0010, std::less()),
		new ALUci_format("cmpile" , 0b0011, std::less_equal()),
		new ALUci_format("cmpiult", 0b0100, std::less<uint32_t>()),
		new ALUci_format("cmpiule", 0b0101, std::less_equal<uint32_t>()),
		new ALUci_format("btesti" , 0b0110, [](int32_t a, int32_t b) { return (a & (1 << b)) != 0; }),

		// ALUp
		new ALUp_format("por" , 0b0110, std::logical_or()),
		new ALUp_format("pand", 0b0111, std::logical_and()),
		new ALUp_format("pxor", 0b1010, std::not_equal_to()),
		
		// ALUb
		new ALUb_format("bcopy", [](int32_t a, int32_t b, bool pred) { return (a & ~(1 << b)) | (pred << b); }),

		// SPCt
		new SPCt_format("mts"),

		// SPCf
		new SPCf_format("mfs"),
		
		// FPUr
		new FPUr_format("fadds"  , 0b0000, std::plus<float>()),
		new FPUr_format("fsubs"  , 0b0001, std::minus<float>()),
		new FPUr_format("fmuls"  , 0b0010, std::multiplies<float>()),
		new FPUr_format("fdivs"  , 0b0011, std::divides<float>()),
		new FPUr_format("fsgnjs" , 0b0100, std::copysignf),
		new FPUr_format("fsgnjns", 0b0101, [](float a, float b) { return std::copysignf(a, (!std::signbit(b)) ? -0.0f : +0.0f); }),
		new FPUr_format("fsgnjxs", 0b0110, [](float a, float b) { return std::copysignf(a, (std::signbit(a) != std::signbit(b)) ? -0.0f : +0.0f); }),

		// FPUl
		new FPUl_format("faddsl", 0b0000, std::plus<float>()),
		new FPUl_format("fsubsl", 0b0001, std::minus<float>()),
		new FPUl_format("fmulsl", 0b0010, std::multiplies<float>()),
		new FPUl_format("fdivsl", 0b0011, std::divides<float>()),

		// FPUrs
		new FPUrs_format("fsqrts", 0b0000, [](float a) { return std::sqrt(a); }),

		//FPUc
		new FPUc_format("feqs", 0b0000, std::equal_to<float>()),
		new FPUc_format("flts", 0b0001, std::less<float>()),
		new FPUc_format("fles", 0b0010, std::less_equal<float>()),

		// FPCt
		new FPCt_format("fcvtis", 0b0000, [](int32_t a) { return static_cast<float>(a); }),
		new FPCt_format("fcvtus", 0b0001, [](int32_t a) { return static_cast<float>(static_cast<uint32_t>(a)); }),
		new FPCt_format("fmvis" , 0b0010, [](int32_t a) { return itf(a); }),

		// FPCf
		new FPCf_format("fcvtsi" , 0b0000, [](float a) { return static_cast<int32_t>(a); }, no_special_tests, float_to_int_rounding),
		new FPCf_format("fcvtsu" , 0b0001, fcvtsu_func, no_special_tests, float_to_int_rounding),
		new FPCf_format("fmvsi"  , 0b0010, [](float a) { return fti(a); }),
		new FPCf_format("fclasss", 0b0011, classifyFloat, fclass_special_test),
	};
}

int main(int argc, char const *argv[])
{
	if (argc != 3)
	{
		std::cerr << "Usage: ISATestsGen <output .s> <output expected>" << std::endl;
		return 1;
	}

	std::string TESTS_DIR_ASM = argv[1];
	std::string TESTS_DIR_EXPECTED = argv[2];

	//std::string TESTS_DIR_ASM = "./test-asm";
	//std::string TESTS_DIR_EXPECTED = "./expected-uart";



	std::cout << "Generating tests..." << std::endl;
	
	std::fesetround(FE_TONEAREST);

	std::mt19937 rngGen(37);

	std::string single_op_asm_dir = patmos::create_dir_if_not_exists(TESTS_DIR_ASM, "single_op_tests");
	std::string single_op_uart_dir = patmos::create_dir_if_not_exists(TESTS_DIR_EXPECTED, "single_op_tests");
	for (patmos::baseFormat* instr : patmos::instrs)
	{
		instr->make_single_op_tests(single_op_asm_dir, single_op_uart_dir, rngGen, 50);
	}	

	std::string forwarding_asm_dir = patmos::create_dir_if_not_exists(TESTS_DIR_ASM, "forwarding_tests");
	std::string forwarding_uart_dir = patmos::create_dir_if_not_exists(TESTS_DIR_EXPECTED, "forwarding_tests");
	for (patmos::baseFormat* instr : patmos::instrs)
	{
		instr->make_forwarding_tests(forwarding_asm_dir, forwarding_uart_dir, rngGen, 50);
	}

	std::cout << "Tests generated." << std::endl;

	return 0;
}
