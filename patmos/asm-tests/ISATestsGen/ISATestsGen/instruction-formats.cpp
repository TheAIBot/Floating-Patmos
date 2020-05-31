
#include "instruction-formats.h"

namespace patmos
{
	template<>
	int32_t opSource::getRandom<int32_t>(std::mt19937& rngGen) const
	{
		std::uniform_int_distribution<int32_t> distribution(intRange.inclusiveMin, intRange.inclusiveMax);
		return distribution(rngGen);
	}

	template<>
	float opSource::getRandom<float>(std::mt19937& rngGen) const
	{
		std::uniform_real_distribution<float> distribution(floatRange.inclusiveMin, floatRange.inclusiveMax);
		return distribution(rngGen);
	}

	static valueRange<int32_t> wholeIntRange(std::numeric_limits<int32_t>::lowest(), std::numeric_limits<int32_t>::max());

	ALUr_format::ALUr_format(std::string name, int32_t func, std::function<int32_t(int32_t, int32_t)> funcOp) : uni_format(
		name, Pipes::Both, funcOp,
		modify_bits<1, 0>(), 
		pred_reg(), 
		hardcoded_bits<5, 0b01000>(), 
		reg_dst<reg_type::GPR>(), 
		reg_src<reg_type::GPR>(wholeIntRange), 
		reg_src<reg_type::GPR>(wholeIntRange), 
		hardcoded_bits<3, 0b000>(), 
		instr_dep_bits<4>(func))
	{}

	ALUi_format::ALUi_format(std::string name, int32_t func, std::function<int32_t(int32_t, int32_t)> funcOp) : uni_format(
		name, Pipes::Both, funcOp,
		modify_bits<1, 0>(), 
		pred_reg(), 
		hardcoded_bits<2, 0b00>(), 
		instr_dep_bits<3>(func),
		reg_dst<reg_type::GPR>(),
		reg_src<reg_type::GPR>(wholeIntRange),
		unsigned_imm<12, int32_t>(valueRange<int32_t>(0, (1 << 12) - 1)))
	{}

	ALUl_format::ALUl_format(std::string name, int32_t func, std::function<int32_t(int32_t, int32_t)> funcOp) : uni_format(
		name, Pipes::First, funcOp,
		hardcoded_bits<1, 1>(),
		pred_reg(),
		hardcoded_bits<5, 0b01000>(),
		reg_dst<reg_type::GPR>(),
		reg_src<reg_type::GPR>(wholeIntRange),
		unused_bits<5>(),
		hardcoded_bits<3, 0b000>(),
		instr_dep_bits<4>(func),
		signed_imm<32, int32_t>(wholeIntRange))
	{}

	ALUm_format::ALUm_format(std::string name, int32_t func, std::function<mulRes(uint32_t, uint32_t)> funcOp) : uni_format(
		name, Pipes::First, funcOp,
		modify_bits<1, 0>(),
		pred_reg(),
		hardcoded_bits<5, 0b01000>(),
		unused_bits<5>(),
		reg_src<reg_type::GPR>(wholeIntRange),
		reg_src<reg_type::GPR>(wholeIntRange),
		hardcoded_bits<3, 0b010>(),
		instr_dep_bits<4>(func),
		implicit_dst_regs<mulRes>())
	{}
	void ALUm_format::make_single_op_tests(std::string asmfilepath, std::string expfilepath, std::mt19937& rngGen, int32_t testCount) const
	{
		std::string asm_dir = create_dir_if_not_exists(asmfilepath, instrName);
		std::string uart_dir = create_dir_if_not_exists(expfilepath, instrName);

		for (size_t i = 0; i < testCount; i++)
		{
			isaTest test(asm_dir, uart_dir, instrName + "-" + std::to_string(i));

			testData tData = setRegistersAndGetInstrData(test, rngGen);

			test.addInstr(instrName + " " + string_join(tData.instr_sources, ", "));
			test.addInstr("nop");
			test.addInstr("nop");
			test.addInstr("nop");
			test.expectRegisterValue("sl", (int32_t)std::apply(opFunc, tData.sourceValues).Low);
			test.expectRegisterValue("sh", (int32_t)std::apply(opFunc, tData.sourceValues).High);

			test.close();
		}
	}

	ALUc_format::ALUc_format(std::string name, int32_t func, std::function<bool(int32_t, int32_t)> funcOp) : uni_format(
		name, Pipes::Both, funcOp,
		modify_bits<1, 0>(),
		pred_reg(),
		hardcoded_bits<5, 0b01000>(),
		unused_bits<2>(),
		reg_dst<reg_type::PR>(),
		reg_src<reg_type::GPR>(wholeIntRange),
		reg_src<reg_type::GPR>(wholeIntRange),
		hardcoded_bits<3, 0b011>(),
		instr_dep_bits<4>(func))
	{}

	ALUci_format::ALUci_format(std::string name, int32_t func, std::function<bool(int32_t, int32_t)> funcOp) : uni_format(
		name, Pipes::Both, funcOp,
		modify_bits<1, 0>(),
		pred_reg(),
		hardcoded_bits<5, 0b01000>(),
		unused_bits<2>(),
		reg_dst<reg_type::PR>(),
		reg_src<reg_type::GPR>(wholeIntRange),
		unsigned_imm<5, int32_t>(valueRange<int32_t>(0, (1 << 5) - 1)),
		hardcoded_bits<3, 0b110>(),
		instr_dep_bits<4>(func))
	{}

	ALUp_format::ALUp_format(std::string name, int32_t func, std::function<bool(bool, bool)> funcOp) : uni_format(
		name, Pipes::Both, funcOp,
		modify_bits<1, 0>(),
		pred_reg(),
		hardcoded_bits<5, 0b01000>(),
		unused_bits<2>(),
		reg_dst<reg_type::PR>(),
		unused_bits<1>(),
		reg_src<reg_type::PR>(),
		unused_bits<1>(),
		reg_src<reg_type::PR>(),
		hardcoded_bits<3, 0b100>(),
		instr_dep_bits<4>(func))
	{}

	ALUb_format::ALUb_format(std::string name, std::function<int32_t(int32_t, int32_t, bool)> funcOp) : uni_format(
		name, Pipes::Both, funcOp,
		modify_bits<1, 0>(),
		pred_reg(),
		hardcoded_bits<5, 0b01000>(),
		reg_dst<reg_type::GPR>(),
		reg_src<reg_type::GPR>(wholeIntRange),
		unsigned_imm<5, int32_t>(valueRange<int32_t>(0, (1 << 5) - 1)),
		hardcoded_bits<3, 0b101>(),
		reg_src<reg_type::PR>())
	{}

	SPCt_format::SPCt_format(std::string name) : uni_format(
		name, Pipes::Both, [](int32_t a) { return a; },
		modify_bits<1, 0>(), 
		pred_reg(), 
		hardcoded_bits<5, 0b01001>(),
		unused_bits<5>(),
		reg_src<reg_type::GPR>(wholeIntRange),
		unused_bits<5>(),
		hardcoded_bits<3, 0b010>(),
		reg_dst<reg_type::SPR>())
	{}

	SPCf_format::SPCf_format(std::string name) : uni_format(
		name, Pipes::Both, [](int32_t a) { return a; },
		modify_bits<1, 0>(), 
		pred_reg(),
		hardcoded_bits<5, 0b01001>(),
		reg_dst<reg_type::GPR>(),
		unused_bits<10>(),
		hardcoded_bits<3, 0b011>(),
		reg_src<reg_type::SPR>(wholeIntRange))
	{}



	static valueRange<float> floatRange1000(-1000.0f, 1000.0f);

	FPUr_format::FPUr_format(std::string name, int32_t func, std::function<float(float, float)> funcOp) : uni_float_format(
		name, x86_and_patmos_compat_rounding_modes, funcOp,
		modify_bits<1, 0>(),
		pred_reg(),
		hardcoded_bits<5, 0b01101>(),
		reg_dst<reg_type::FPR>(),
		reg_src<reg_type::FPR>(floatRange1000),
		reg_src<reg_type::FPR>(floatRange1000),
		hardcoded_bits<3, 0b000>(),
		instr_dep_bits<4>(func))
	{}

	FPUl_format::FPUl_format(std::string name, int32_t func, std::function<float(float, float)> funcOp) : uni_float_format(
		name, x86_and_patmos_compat_rounding_modes, funcOp,
		hardcoded_bits<1, 1>(),
		pred_reg(),
		hardcoded_bits<5, 0b01101>(),
		reg_dst<reg_type::FPR>(),
		reg_src<reg_type::FPR>(floatRange1000),
		unused_bits<5>(),
		hardcoded_bits<3, 0b010>(),
		instr_dep_bits<4>(func),
		signed_imm<32, float>(floatRange1000))
	{}

	FPUrs_format::FPUrs_format(std::string name, int32_t func, std::function<float(float)> funcOp) : uni_float_format(
		name, x86_and_patmos_compat_rounding_modes, funcOp,
		modify_bits<1, 0>(),
		pred_reg(),
		hardcoded_bits<5, 0b01101>(),
		reg_dst<reg_type::FPR>(),
		reg_src<reg_type::FPR>(floatRange1000),
		unused_bits<5>(),
		hardcoded_bits<3, 0b011>(),
		instr_dep_bits<4>(func))
	{}

	FPUc_format::FPUc_format(std::string name, int32_t func, std::function<bool(float, float)> funcOp) : uni_float_format(
		name, x86_and_patmos_compat_rounding_modes, funcOp,
		modify_bits<1, 0>(),
		pred_reg(),
		hardcoded_bits<5, 0b01101>(),
		unused_bits<2>(),
		reg_dst<reg_type::PR>(),
		reg_src<reg_type::FPR>(floatRange1000),
		reg_src<reg_type::FPR>(floatRange1000),
		hardcoded_bits<3, 0b001>(),
		instr_dep_bits<4>(func))
	{}

	FPCt_format::FPCt_format(std::string name, int32_t func, std::function<float(int32_t)> funcOp) : uni_float_format(
		name, x86_and_patmos_compat_rounding_modes, funcOp,
		modify_bits<1, 0>(),
		pred_reg(),
		hardcoded_bits<5, 0b01110>(),
		reg_dst<reg_type::FPR>(),
		reg_src<reg_type::GPR>(wholeIntRange),
		unused_bits<5>(),
		hardcoded_bits<3, 0b000>(),
		instr_dep_bits<4>(func))
	{}

	FPCf_format::FPCf_format(std::string name, int32_t func, std::function<int32_t(float)> funcOp, const std::vector<std::function<void(isaTest&)>>& special_tests, const std::vector<rounding::x86_round_mode>& rounding_modes) : uni_float_format(
		name, x86_and_patmos_compat_rounding_modes, funcOp,
		modify_bits<1, 0>(),
		pred_reg(),
		hardcoded_bits<5, 0b01110>(),
		reg_dst<reg_type::GPR>(),
		reg_src<reg_type::FPR>(floatRange1000),
		unused_bits<5>(),
		hardcoded_bits<3, 0b001>(),
		instr_dep_bits<4>(func)), special_tests(special_tests)
	{}

}