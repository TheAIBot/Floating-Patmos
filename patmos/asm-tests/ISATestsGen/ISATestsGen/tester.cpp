
#include "tester.h"


isaTest::isaTest(std::string asmfilepath, std::string expfilepath, std::string filename)
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

void isaTest::addInstr(std::string instr)
{
	asmFile << instr << '\n';
}

void isaTest::setFloatReg(std::string reg, float value)
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

void isaTest::setGPReg(std::string reg, int32_t value)
{
	addInstr("addl " + reg + " = r0, " + std::to_string(value));
}

void isaTest::setPred(std::string reg, bool value)
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

void isaTest::setSpecialReg(std::string reg, int32_t value)
{
	addInstr("mts " + reg + " = " + std::to_string(value));
}

void isaTest::expectRegisterValue(std::string reg, int32_t value)
{
	expectedFile << ((uint8_t)(value >> 0));
	expectedFile << ((uint8_t)(value >> 8));
	expectedFile << ((uint8_t)(value >> 16));
	expectedFile << ((uint8_t)(value >> 24));
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
void isaTest::expectRegisterValue(std::string reg, float value)
{
	expectRegisterValue(reg, reinterpret_cast<int32_t&>(value));
}

void isaTest::close()
{
	asmFile << "halt" << '\n';
	asmFile << "nop" << '\n';
	asmFile << "nop" << '\n';
	asmFile << "nop" << '\n';
	asmFile.close();
	expectedFile.close();
}