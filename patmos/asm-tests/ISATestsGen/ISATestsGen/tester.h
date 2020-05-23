#pragma once

#include <iostream>
#include <fstream>
#include <string>
#include <cstdint>

class isaTest
{
private:
	std::ofstream asmFile;
	std::ofstream expectedFile;

	void setFloatReg(std::string reg, float value);
	void setGPReg(std::string reg, int32_t value);
	void setPred(std::string reg, bool value);
	void setSpecialReg(std::string reg, int32_t value);

public:
	isaTest(std::string asmfilepath, std::string expfilepath, std::string filename);

	void addInstr(std::string instr);

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