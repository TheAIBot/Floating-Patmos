
#include "instruction-formats.h"

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
	for (size_t i = 0; i < testCount; i++)
	{
		isaTest test(asmfilepath, expfilepath, instrName + "-" + std::to_string(i));

		testData tData = setRegistersAndGetInstrData(test, rngGen);

		test.addInstr(instrName + string_join(tData.instr_sources, ", "));
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
