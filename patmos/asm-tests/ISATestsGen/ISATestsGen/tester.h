#pragma once

#include <iostream>
#include <fstream>
#include <string>
#include <cstdint>
#include <cmath>
#include <charconv>
#include <array>
#include <vector>
#include <cfenv>
#include "common.h"

namespace patmos
{
	class isaTest
	{
	private:
		std::string asm_path;
		std::string uart_path;
		std::vector<std::string> program_instrs;
		std::vector<int32_t> expected_uart;
		int32_t program_byte_count;

		void setFloatReg(std::string reg, float value);
		void setGPReg(std::string reg, int32_t value);
		void setPred(std::string reg, bool value);
		void setSpecialReg(std::string reg, int32_t value);

		void write_asm_file();
		void write_uart_file();

	public:
		isaTest(std::string asmfilepath, std::string expfilepath, std::string filename);

		void addInstr(std::string instr, int32_t instr_bytes = 4);

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

		void expectRegisterValue(std::string reg, int32_t value);
		void expectRegisterValue(std::string reg, float value);

		void close();
	};
}