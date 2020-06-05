
#include "tester.h"
#include <sstream>

namespace patmos
{
	isaTest::isaTest(std::string asmfilepath, std::string expfilepath, std::string filename) : 
		asm_path(asmfilepath + '/' + filename + ".s"), 
		uart_path(expfilepath + '/' + filename + ".uart"),
		debug_path(expfilepath + '/' + filename + ".debug"),
		program_byte_count(0)
	{}

	void isaTest::addInstr(std::string instr, int32_t instr_bytes)
	{
		program_instrs.push_back(instr);
		program_byte_count += instr_bytes;
	}

	std::string float_to_string(float value)
	{
		std::ostringstream out;
		out.precision(std::numeric_limits<float>::max_digits10);
		out << std::fixed << value;

		return out.str();
	}

	void isaTest::setFloatReg(std::string reg, float value)
	{
		if (std::isnan(value) || std::isinf(value) || std::fpclassify(value) == FP_SUBNORMAL)
		{
			addInstr("addl r23 = r0, " + std::to_string((uint32_t)fti(value)) + " # " + float_to_string(value), 8);
			addInstr("fmvis " + reg + " = r23");
		}
		else
		{
			addInstr("fmvis " + reg + " = r0");
			addInstr("faddsl " + reg + " = " + reg + ", " + float_to_string(value), 8);
		}
	}

	void isaTest::setGPReg(std::string reg, int32_t value)
	{
		addInstr("addl " + reg + " = r0, " + std::to_string(value), 8);
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
		setGPReg("r26", value);
		addInstr("mts " + reg + " = r26");
	}

	void isaTest::move_to_uart_register(std::string reg)
	{
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
	}

	void isaTest::expectRegisterValue(std::string reg, int32_t value)
	{
		expected_uart.push_back(value);
		if (reg == "sfcsr" || reg == "s1")
		{
			debug_info.push_back(test_debug_info(reg, "exception"));
		}
		else
		{
			debug_info.push_back(test_debug_info(reg, "int"));
		}

		move_to_uart_register(reg);
		addInstr("callnd uart1");
	}
	void isaTest::expectRegisterValue(std::string reg, bool value)
	{
		expected_uart.push_back(value);
		debug_info.push_back(test_debug_info(reg, "bool"));
		
		move_to_uart_register(reg);
		addInstr("callnd uart1");
	}
	void isaTest::expectRegisterValue(std::string reg, float value)
	{
		expected_uart.push_back(fti(value));
		debug_info.push_back(test_debug_info(reg, "float"));
		
		move_to_uart_register(reg);
		addInstr("callnd uart1");
	}

	void isaTest::write_asm_file()
	{
		std::ofstream asm_file(asm_path);
		
		//includes the byte count for things such as jumping over the uart function, 
		//the uart function itself and instructions to halt the program
		const int32_t program_overhead_bytes = 28 * 4;

		//specify specificly how many bytes needs to be loaded into the instr cache
		asm_file << ".word " + std::to_string(program_byte_count + program_overhead_bytes) << '\n';
		
		asm_file << R"(br start
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

		asm_file << string_join(program_instrs, "\n") << '\n';
		
		asm_file << "halt" << '\n';
		asm_file << "nop" << '\n';
		asm_file << "nop" << '\n';
		asm_file << "nop" << '\n';
		asm_file.close();
	}

	void isaTest::write_uart_file()
	{
		std::ofstream uart_file(uart_path);

		for (size_t i = 0; i < expected_uart.size(); i++)
		{
			uart_file.write(reinterpret_cast<char*>(&expected_uart[i]), sizeof(int32_t));
		}
		
		uart_file.close();
	}

	void isaTest::write_debug_file()
	{
		std::ofstream debug_file(debug_path);

		for (size_t i = 0; i < debug_info.size(); i++)
		{
			debug_file << debug_info[i].dst_reg + " : " + debug_info[i].value_type << '\n';
		}
		
		debug_file.close();
	}

	void isaTest::close()
	{
		write_asm_file();
		write_uart_file();
		write_debug_file();
	}
}