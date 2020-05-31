// ISASim.cpp : Defines the entry point for the application.
//

#include <iostream>
#include <fstream>
#include <string>
#include <functional>
#include <limits>
#include <array>
#include <vector>
#include <stdexcept>
#include <immintrin.h>
#include <cmath>
#include <memory>
#include <cfenv>
#include "ISASim.h"

#pragma STDC FENV_ACCESS ON
#pragma fenv_access (on)

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

enum class patmos_round_mode
{
	RNE = 0b000,
	RTZ = 0b001,
	RDN = 0b010,
	RUP = 0b011,
	RMM = 0b100
};

enum class x86_round_mode
{
	RNE = FE_TONEAREST,
	RTZ = FE_TOWARDZERO,
	RDN = FE_DOWNWARD,
	RUP = FE_UPWARD
};


x86_round_mode patmos_to_x86_round(const patmos_round_mode round)
{
	switch (round)
	{
	case patmos_round_mode::RNE:
		return x86_round_mode::RNE;
	case patmos_round_mode::RTZ:
		return x86_round_mode::RTZ;
	case patmos_round_mode::RDN:
		return x86_round_mode::RDN;
	case patmos_round_mode::RUP:
		return x86_round_mode::RUP;
	case patmos_round_mode::RMM:
		throw std::runtime_error("x86 has no rounding mode which is equivalent to the patmos RMM rounding mode.");
	default:
		throw std::runtime_error("Invalid patmos rounding mode. Mode: " + std::to_string((int32_t)round));
	}
}

enum class patmos_exceptions
{
	NV = 0b10000,
	DZ = 0b01000,
	OF = 0b00100,
	UF = 0b00010,
	NX = 0b00001,
};

int32_t get_leading_zeroes(const int32_t value)
{
	unsigned long index;
#ifdef __linux__
	index = __builtin_ctz(value);
#elif _WIN32
	_BitScanForward(&index, mask);
#else
	static_assert(false, R"(Platform was detected as neither linux or windows.
You need to write the platform specific way to call the bsf instruction here.)");
#endif
	return index;
}

int32_t get_leading_zeroes(const patmos_exceptions ex)
{
	return get_leading_zeroes((int32_t)ex);
}

int32_t x86_to_patmos_exceptions(const std::fexcept_t& ex)
{
	return 
		(((ex & FE_INVALID) >> get_leading_zeroes(FE_INVALID)) << get_leading_zeroes(patmos_exceptions::NV)) |
		(((ex & FE_DIVBYZERO) >> get_leading_zeroes(FE_DIVBYZERO)) << get_leading_zeroes(patmos_exceptions::DZ)) |
		(((ex & FE_OVERFLOW) >> get_leading_zeroes(FE_OVERFLOW)) << get_leading_zeroes(patmos_exceptions::OF)) |
		(((ex & FE_UNDERFLOW) >> get_leading_zeroes(FE_UNDERFLOW)) << get_leading_zeroes(patmos_exceptions::UF)) |
		(((ex & FE_INEXACT) >> get_leading_zeroes(FE_INEXACT)) << get_leading_zeroes(patmos_exceptions::NX));
}

std::fexcept_t patmos_to_x86_exceptions(const int32_t ex)
{
	return  (std::fexcept_t)(
		(((ex & (int32_t)patmos_exceptions::NV) >> get_leading_zeroes(patmos_exceptions::NV)) << get_leading_zeroes(FE_INVALID)) |
		(((ex & (int32_t)patmos_exceptions::DZ) >> get_leading_zeroes(patmos_exceptions::DZ)) << get_leading_zeroes(FE_DIVBYZERO)) |
		(((ex & (int32_t)patmos_exceptions::OF) >> get_leading_zeroes(patmos_exceptions::OF)) << get_leading_zeroes(FE_OVERFLOW)) |
		(((ex & (int32_t)patmos_exceptions::UF) >> get_leading_zeroes(patmos_exceptions::UF)) << get_leading_zeroes(FE_UNDERFLOW)) |
		(((ex & (int32_t)patmos_exceptions::NX) >> get_leading_zeroes(patmos_exceptions::NX)) << get_leading_zeroes(FE_INEXACT)));
}




enum class InstrFormat
{
	ALUr,
	ALUi,
	ALUl,
	ALUm,
	ALUc,
	ALUci,
	ALUp,
	ALUb,
	SPCt,
	SPCf,
	LDT,
	STT,
	STCi,
	STCr,
	CFLi,
	CFLri,
	CFLrs,
	CFLrt,
	FPUr,
	FPUl,
	FPUrs,
	FPCt,
	FPCf,
	FPUc
};

InstrFormat getInstrFormat(uint32_t instr)
{
	switch (instr & 0b00000'11'0000000000000000000000000)
	{
	case 0b00000'00'0000000000000000000000000:
		return InstrFormat::ALUi;
	case 0b00000'10'0000000000000000000000000:
		return InstrFormat::CFLi;
	}
	switch (instr & 0b00000'11111'000000000000000'111'0000)
	{
	case 0b00000'01000'000000000000000'000'0000:
		return InstrFormat::ALUr;
	case 0b00000'01000'000000000000000'010'0000:
		return InstrFormat::ALUm;
	case 0b00000'01000'000000000000000'011'0000:
		return InstrFormat::ALUc;
	case 0b00000'01000'000000000000000'100'0000:
		return InstrFormat::ALUp;
	case 0b00000'01000'000000000000000'101'0000:
		return InstrFormat::ALUb;
	case 0b00000'01000'000000000000000'110'0000:
		return InstrFormat::ALUci;
	case 0b00000'11111'000000000000000'000'0000:
		return InstrFormat::ALUl;
	case 0b00000'01001'000000000000000'010'0000:
		return InstrFormat::SPCt;
	case 0b00000'01001'000000000000000'011'0000:
		return InstrFormat::SPCf;
	case 0b00000'01101'000000000000000'000'0000:
		return InstrFormat::FPUr;
	case 0b00000'01101'000000000000000'001'0000:
		return InstrFormat::FPUc;
	case 0b00000'01101'000000000000000'010'0000:
		return InstrFormat::FPUl;
	case 0b00000'01101'000000000000000'011'0000:
		return InstrFormat::FPUrs;
	case 0b00000'01110'000000000000000'000'0000:
		return InstrFormat::FPCt;
	case 0b00000'01110'000000000000000'001'0000:
		return InstrFormat::FPCf;
	}
	switch (instr & 0b00000'11111'0000000000000000000000)
	{
	case 0b00000'01010'0000000000000000000000:
		return InstrFormat::LDT;
	case 0b00000'01011'0000000000000000000000:
		return InstrFormat::STT;
	}
	switch (instr & 0b00000'11111'00'11'000000000000000000)
	{
	case 0b00000'01100'00'00'000000000000000000:
		return InstrFormat::STCi;
	case 0b00000'01100'00'01'000000000000000000:
		return InstrFormat::STCr;
	}
	switch (instr & 0b00000'1111'0000000000000000000'11'00)
	{
	case 0b00000'1100'0000000000000000000'00'00:
		return InstrFormat::CFLri;
	case 0b00000'1100'0000000000000000000'01'00:
		return InstrFormat::CFLrs;
	case 0b00000'1100'0000000000000000000'10'00:
		return InstrFormat::CFLrt;
	}

	throw std::runtime_error("Failed to parse the instruction format of the instruction: " + std::to_string(instr));
}

uint32_t getInstrPart(uint32_t instr, uint32_t mask)
{
	unsigned long index;
#ifdef __linux__
	index = __builtin_ctz(mask);
#elif _WIN32
	_BitScanForward(&index, mask);
#else
	static_assert(false, R"(Platform was detected as neither linux or windows.
You need to write the platform specific way to call the bsf instruction here.)");
#endif
	return (instr & mask) >> index;
}

class simulator
{
private:
	std::array<uint32_t, 32> GPRs;
	std::array<float, 32> FPRs;
	uint8_t PRRs;
	std::array<uint32_t, 16> SPRs;
	std::unique_ptr<std::vector<uint8_t>> LocalMemory;
	std::unique_ptr<std::vector<uint8_t>> GlobalMemory;
	std::unique_ptr<std::vector<uint32_t>> InstrMemory;
	std::unique_ptr<std::vector<uint8_t>> UartOutput;
	int32_t PC;
	int32_t Base;
	bool Verbose;
	std::fenv_t fp_env;

	uint32_t getGPR(int32_t gpr) { return GPRs[gpr]; }
	void setGPR(int32_t gpr, uint32_t value) 
	{ 
		GPRs[gpr] = (gpr == 0) ? 0 : value;
	}

	float getFPR(int32_t fpr) { return FPRs[fpr]; }
	void setFPR(int32_t fpr, float value) { FPRs[fpr] = value; }

	bool getPRR(int32_t prr) { return (PRRs >> prr) & 1; }
	void setPRR(int32_t prr, bool value) 
	{ 
		if (prr == 0)
		{
			return;
			//throw std::runtime_error("Tried to write to p0");
		}
		PRRs = (PRRs & (~(1 << prr))) | (value << prr); 
	}

	int32_t getSPR(int32_t spr) 
	{
		if (spr == 0)
		{
			return PRRs;
		}
		return SPRs[spr];
	}
	void setSPR(int32_t spr, int32_t value) 
	{ 
		if (spr == 0)
		{
			PRRs = value;
		}
		else
		{
			SPRs[spr] = value;
		}
	}

	void SetLocalMemory(uint32_t address, uint32_t value)
	{
		if (address == 0xf0080004)
		{
			UartOutput->push_back(value);
		}
		else 
		{
			throw std::runtime_error("The simulator only supports read/write to uart memory.");
		}
	}

	uint32_t GetLocalMemory(uint32_t address)
	{
		if (address == 0xf0080000)
		{
			return 1;
		}
		else 
		{
			throw std::runtime_error("The simulator only supports read/write to uart memory.");
		}
	}

	uint32_t getNextInstr(int32_t pcOffset = 0)
	{
		return (*InstrMemory)[Base + PC + pcOffset];
	}

	bool isBundle(uint32_t instr)
	{
		return (instr & 0x80000000) != 0;
	}

	patmos_round_mode get_fp_rounding_mode()
	{
		return (patmos_round_mode)getInstrPart(getSPR(1), 0b00000000'00000000'00000000'11100000);
	}

	int32_t get_fp_exceptions()
	{
		return getInstrPart(getSPR(1), 0b00000000'00000000'00000000'00011111);
	}

	void save_and_set_fp_env()
	{
		if (fegetenv(&fp_env) != 0)
		{
			std::runtime_error("Failed to store the current floating-point environment.");
		}
		if (fesetround((int32_t)patmos_to_x86_round(get_fp_rounding_mode())) != 0)
		{
			std::runtime_error("Failed to set the floating-point rounding mode.");
		}
		std::fexcept_t patmos_exceptions = patmos_to_x86_exceptions(get_fp_exceptions());
		if (fesetexceptflag(&patmos_exceptions, FE_ALL_EXCEPT) != 0)
		{
			std::runtime_error("Failed to set floating-point exceptions.");
		}
	}

	void restore_fp_env()
	{
		std::fexcept_t x86_exceptions;
		if (fegetexceptflag(&x86_exceptions, FE_ALL_EXCEPT) != 0)
		{
			std::runtime_error("Failed to get floating-point exceptions.");
		}
		int32_t patmos_exceptions = x86_to_patmos_exceptions(x86_exceptions);
		setSPR(1, getSPR(1) | patmos_exceptions);
		
		if (fesetenv(&fp_env) != 0)
		{
			std::runtime_error("Failed to restore the floating-point environment.");
		}
	}

public:
	simulator(std::unique_ptr<std::vector<uint32_t>> instructions)
	{
		std::fill(GPRs.begin(), GPRs.end(), 0);
		std::fill(FPRs.begin(), FPRs.end(), 0.0f);
		PRRs = 0b0000'0001;
		std::fill(SPRs.begin(), SPRs.end(), 0);

		LocalMemory = std::make_unique<std::vector<uint8_t>>();
		GlobalMemory = std::make_unique<std::vector<uint8_t>>();
		InstrMemory = std::move(instructions);
		UartOutput = std::make_unique<std::vector<uint8_t>>();

		// we are just gonna ignore all the
		// memory mapped stuff
		const int32_t localMemorySize = 2048; // 2^11
		const int32_t globalMemorySize = 2097152; // 2^21

		LocalMemory->reserve(localMemorySize);
		GlobalMemory->reserve(globalMemorySize);

		PC = 0;
		Base = 1; // this is wrong but okay for now
		//should really point to the first function
		//to execute and right now it only points
		//to the first instruction
	}

	bool tick()
	{
		uint32_t instrA = getNextInstr();
		bool isDual = isBundle(instrA);

		uint32_t instrB = isDual ? getNextInstr(1) : 0;

		InstrFormat instrAFormat = getInstrFormat(instrA);
		bool hasLongImm = instrAFormat == InstrFormat::ALUl || instrAFormat == InstrFormat::FPUl;

		bool hasHalted = false;
		if (isDual && !hasLongImm)
		{
			auto [halt, bundleDone] = execute(instrA, 0, false);
			if (!bundleDone)
			{
				execute(instrB, 0, false);
			}
			hasHalted = halt;
		}
		else
		{
			auto [halt, _] = execute(instrA, instrB, hasLongImm);
			hasHalted = halt;
		}

		return hasHalted;
	}

	std::vector<uint8_t>* getUart()
	{
		return UartOutput.get();
	}

	void setVerbose(bool value)
	{
		Verbose = value;
	}

private:
	std::tuple<bool, bool> execute(uint32_t instr, uint32_t longImm, bool hasLong)
	{
		InstrFormat iFormat = getInstrFormat(instr);
		uint32_t pred    = getInstrPart(instr, 0b01111000'00000000'00000000'00000000);
		uint32_t rd      = getInstrPart(instr, 0b00000000'00111110'00000000'00000000);
		uint32_t pd      = getInstrPart(instr, 0b00000000'00001110'00000000'00000000);
		uint32_t rs1     = getInstrPart(instr, 0b00000000'00000001'11110000'00000000);
		uint32_t ps1     = getInstrPart(instr, 0b00000000'00000000'11110000'00000000);
		uint32_t rs2     = getInstrPart(instr, 0b00000000'00000000'00001111'10000000);
		uint32_t ps2     = getInstrPart(instr, 0b00000000'00000000'00000111'10000000);
		uint32_t imm     = getInstrPart(instr, 0b00000000'00000000'00001111'11111111);
		uint32_t immc    = getInstrPart(instr, 0b00000000'00000000'00001111'10000000);
		uint32_t immb    = getInstrPart(instr, 0b00000000'00000000'00001111'10000000);
		uint32_t alubps  = getInstrPart(instr, 0b00000000'00000000'00000000'00001111);
		uint32_t sd      = getInstrPart(instr, 0b00000000'00000000'00000000'00001111);
		uint32_t ss      = getInstrPart(instr, 0b00000000'00000000'00000000'00001111);
		uint32_t delay   = getInstrPart(instr, 0b00000000'01000000'00000000'00000000);
		uint32_t immcfl  = getInstrPart(instr, 0b00000000'00111111'11111111'11111111);
		uint32_t immmem  = getInstrPart(instr, 0b00000000'00000000'00000000'01111111);

		if (!(getPRR(pred & 0b111) ^ (pred >> 3)))
		{
			PC += hasLong ? 2 : 1;
			return std::make_tuple(false, false);
		}

		uint32_t aluFunc;
		if (iFormat == InstrFormat::ALUi)
		{
			aluFunc = getInstrPart(instr, 0b00000001110000000000000000000000);
		}
		else
		{
			aluFunc = getInstrPart(instr, 0b00000000000000000000000000001111);
		}

		uint32_t aluOp1 = getGPR(rs1);
		uint32_t aluOp2;
		switch (iFormat)
		{
		case InstrFormat::ALUi:
			aluOp2 = imm;
			break;
		case InstrFormat::ALUl:
			aluOp2 = longImm;
			break;
		case InstrFormat::ALUci:
			aluOp2 = immc;
			break;
		default:
			aluOp2 = getGPR(rs2);
			break;
		}

		bool alupOp1 = getPRR(ps1 & 0b111) ^ (ps1 >> 3);
		bool alupOp2 = getPRR(ps2 & 0b111) ^ (ps2 >> 3);
		bool alubOp1 = getPRR(alubps & 0b111) ^ (alubps >> 3);

		uint32_t spcOp1 = getSPR(ss);

		uint32_t cflFunc;
		if (iFormat == InstrFormat::CFLi)
		{
			cflFunc = getInstrPart(instr, 0b00000001100000000000000000000000);
		}
		else
		{
			cflFunc = getInstrPart(instr, 0b00000000000000000000000000000011);
		}
		cflFunc = (cflFunc << 1) | delay;
		int32_t simmcfl = ((int32_t)(immcfl << 10)) >> 10;
		uint32_t cflOp1 = getGPR(rs1);
		uint32_t cflOp2 = getGPR(rs2);

		float fpuOp1 = getFPR(rs1);
		float fpuOp2;
		if (iFormat == InstrFormat::FPUl)
		{
			fpuOp2 = reinterpret_cast<float&>(longImm);
		}
		else
		{
			fpuOp2 = getFPR(rs2);
		}
		uint32_t fpuFunc = aluFunc;

		uint32_t ldtFunc = rs2;
		uint32_t ldtOpa = getGPR(rs1);

		uint32_t sttFunc = rd;
		uint32_t sttOpa = getGPR(rs1);
		uint32_t sttOps = getGPR(rs2);

		bool halt = false;
		bool bundleAlreadyExecuted = false;

		switch (iFormat)
		{
		case InstrFormat::ALUr:
		case InstrFormat::ALUi:
		case InstrFormat::ALUl:
			switch (aluFunc)
			{
			case 0b0000: // add
				setGPR(rd, aluOp1 + aluOp2);
				break;
			case 0b0001: // sub
				setGPR(rd, aluOp1 - aluOp2);
				break;
			case 0b0010: // xor
				setGPR(rd, aluOp1 ^ aluOp2);
				break;
			case 0b0011: // sl
				setGPR(rd, aluOp1 << (aluOp2 & 0x1f));
				break;
			case 0b0100: // sr
				setGPR(rd, aluOp1 >> (aluOp2 & 0x1f));
				break;
			case 0b0101: // sra
				setGPR(rd, ((int32_t)aluOp1) >> (aluOp2 & 0x1f));
				break;
			case 0b0110: // or
				setGPR(rd, aluOp1 | aluOp2);
				break;
			case 0b0111: // and
				setGPR(rd, aluOp1 & aluOp2);
				break;
			case 0b1011: // nor
				setGPR(rd, ~(aluOp1 | aluOp2));
				break;
			case 0b1100: // shadd
				setGPR(rd, (aluOp1 << 1) + aluOp2);
				break;
			case 0b1101: // shadd2
				setGPR(rd, (aluOp1 << 2) + aluOp2);
				break;
			default:
				throw std::runtime_error("Invalid (ALUr or ALUi or ALUl) func: " + std::to_string(aluFunc));
			}
			PC += hasLong ? 2 : 1;
			break;
		case InstrFormat::ALUm:
			switch (aluFunc)
			{
			case 0b0000: // mul
			{
				int32_t op1H = ((int32_t)aluOp1) >> 16;
				int32_t op2H = ((int32_t)aluOp2) >> 16;
				int32_t op1L = aluOp1 & 0xffff;
				int32_t op2L = aluOp2 & 0xffff;

				uint32_t mulLL = op1L * op2L;
				uint32_t mulLH = op1L * op2H;
				uint32_t mulHL = op1H * op2L;
				uint32_t mulHH = op1H * op2H;

				int64_t catHHLL = (((uint64_t)mulHH) << 32) | ((uint64_t)mulLL);
				int64_t catHL = (int64_t)(int32_t)mulHL << 16;
				int64_t catLH = (int64_t)(int32_t)mulLH << 16;

				uint64_t res = catHHLL + catHL + catLH;

				int32_t resL = res;
				int32_t resH = res >> 32;

				setSPR(2, resL);
				setSPR(3, resH);
				break;
			}
			case 0b0001: // mulu
			{
    			int32_t op1H = ((uint32_t)aluOp1) >> 16;
				int32_t op2H = ((uint32_t)aluOp2) >> 16;
				int32_t op1L = aluOp1 & 0xffff;
				int32_t op2L = aluOp2 & 0xffff;

				uint32_t mulLL = op1L * op2L;
				uint32_t mulLH = op1L * op2H;
				uint32_t mulHL = op1H * op2L;
				uint32_t mulHH = op1H * op2H;

				int64_t catHHLL = (((uint64_t)mulHH) << 32) | ((uint64_t)mulLL);
				int64_t catHL = (int64_t)(uint32_t)mulHL << 16;
				int64_t catLH = (int64_t)(uint32_t)mulLH << 16;

				uint64_t res = catHHLL + catHL + catLH;

				uint32_t resL = res;
				uint32_t resH = res >> 32;

				setSPR(2, resL);
				setSPR(3, resH);
				break;
			}
			default:
				throw std::runtime_error("Invalid ALUm func: " + std::to_string(aluFunc));
			}
			PC++;
			break;
		case InstrFormat::ALUc:
		case InstrFormat::ALUci:
			switch (aluFunc)
			{
			case 0b0000:
				setPRR(pd, aluOp1 == aluOp2);
				break;
			case 0b0001:
				setPRR(pd, aluOp1 != aluOp2);
				break;
			case 0b0010:
				setPRR(pd, (int32_t)aluOp1 < (int32_t)aluOp2);
				break;
			case 0b0011:
				setPRR(pd, (int32_t)aluOp1 <= (int32_t)aluOp2);
				break;
			case 0b0100:
				setPRR(pd, aluOp1 < aluOp2);
				break;
			case 0b0101:
				setPRR(pd, aluOp1 <= aluOp2);
				break;
			case 0b0110:
				setPRR(pd, (aluOp1 & (1 << aluOp2)) != 0);
				break;
			default:
				throw std::runtime_error("Invalid (ALUc or ALUci) func: " + std::to_string(aluFunc));
			}
			PC++;
			break;
		case InstrFormat::ALUp:
			switch (aluFunc)
			{
			case 0b0110:
				setPRR(pd, alupOp1 || alupOp2);
				break;
			case 0b0111:
				setPRR(pd, alupOp1 && alupOp2);
				break;
			case 0b1010:
				setPRR(pd, alupOp1 != alupOp2);
				break;
			default:
				throw std::runtime_error("Invalid ALUp func: " + std::to_string(aluFunc));
			}
			PC++;
			break;
		case InstrFormat::ALUb:
			setGPR(rd, (aluOp1 & ~(1 << immb)) | (alubOp1));
			PC++;
			break;
		case InstrFormat::SPCt:
			setSPR(sd, aluOp1);
			PC++;
			break;
		case InstrFormat::SPCf:
			setGPR(rd, spcOp1);
			PC++;
			break;
		case InstrFormat::LDT:
			switch (ldtFunc)
			{
			case 0b00001:
				setGPR(rd, GetLocalMemory(ldtOpa + (immmem << 2)));
				break;
			default:
				throw std::runtime_error("Simulator only support read/write to uart memory with lwl.");
			}
			PC++;
			break;
		case InstrFormat::STT:
			switch (sttFunc)
			{
			case 0b00001:
				SetLocalMemory(sttOpa + (immmem << 2), sttOps);
				break;
			default:
				throw std::runtime_error("Simulator only support read/write to uart memory with lwl.");
			}
			PC++;
			break;
		case InstrFormat::STCi:
			// not implemented yet
			break;
		case InstrFormat::STCr:
			// not implemented yet
			break;
		case InstrFormat::CFLi:
			switch (cflFunc)
			{
			case 0b000: // callnd
				PC++;
				if (isBundle(instr))
				{
					tick();
					bundleAlreadyExecuted = true;
				}
				setSPR(7, Base);
				setSPR(8, PC);
				Base = immcfl;
				PC = 0;
				break;
			case 0b001: // call
				PC++;
				if (isBundle(instr))
				{
					tick();
					bundleAlreadyExecuted = true;
				}
				tick();
				tick();
				tick();
				setSPR(7, Base); // should be visible to delay instructions?
				setSPR(8, PC);
				Base = immcfl;
				PC = 0;
				break;
			case 0b010: // brnd
				if (isBundle(instr))
				{
					PC++;
					tick();
					bundleAlreadyExecuted = true;
				}
				PC += simmcfl;
				break; 
			case 0b011: // br
			{
				int32_t tmpPC = PC;
				PC++;
				if (isBundle(instr))
				{
					tick();
					bundleAlreadyExecuted = true;
				}
				tick();
				tick();
				PC = tmpPC + simmcfl;
			}
				break;
			case 0b100: // brcfnd
				PC++;
				if (isBundle(instr))
				{
					tick();
					bundleAlreadyExecuted = true;
				}
				Base = immcfl;
				PC = 0;
				break;
			case 0b101: // brcf
				PC++;
				if (isBundle(instr))
				{
					tick();
					bundleAlreadyExecuted = true;
				}
				tick();
				tick();
				tick();
				Base = immcfl;
				PC = 0;
				if (Base == 0)
				{
					halt = true;
				}
				break;
			case 0b110:
				throw std::runtime_error("Trap and exceptions are not supported.");
			default:
				throw std::runtime_error("Invalid Op and delay combination for CFLi format.");
			}
			break;
		case InstrFormat::CFLri:
			switch (cflFunc)
			{
			case 0b000: // retnd
				PC++;
				if (isBundle(instr))
				{
					tick();
					bundleAlreadyExecuted = true;
				}
				Base = getSPR(7);
				PC = getSPR(8);
				break;
			case 0b001: // ret
				PC++;
				if (isBundle(instr))
				{
					tick();
					bundleAlreadyExecuted = true;
				}
				tick();
				tick();
				tick();
				Base = getSPR(7);
				PC = getSPR(8);
				break;
			case 0b010: // xretnd
				PC++;
				if (isBundle(instr))
				{
					tick();
					bundleAlreadyExecuted = true;
				}
				Base = getSPR(9);
				PC = getSPR(10);
				break;
			case 0b011: // xret
				PC++;
				if (isBundle(instr))
				{
					tick();
					bundleAlreadyExecuted = true;
				}
				tick();
				tick();
				tick();
				Base = getSPR(9);
				PC = getSPR(10);
				break;
			default:
				throw std::runtime_error("Invalid Op and delay combination for CFLri format.");
			}
			break;
		case InstrFormat::CFLrs:
			switch (cflFunc)
			{
			case 0b000: // callnd
				PC++;
				if (isBundle(instr))
				{
					tick();
					bundleAlreadyExecuted = true;
				}
				setSPR(7, Base);
				setSPR(8, PC);
				Base = cflOp1 >> 2;
				PC = 0;
				break;
			case 0b001: // call
				PC++;
				if (isBundle(instr))
				{
					tick();
					bundleAlreadyExecuted = true;
				}
				tick();
				tick();
				tick();
				setSPR(7, Base); // set before or after delay instructions?
				setSPR(8, PC);
				Base = cflOp1 >> 2;
				PC = 0;
				break;
			case 0b010: // brnd
				PC++;
				if (isBundle(instr))
				{
					tick();
					bundleAlreadyExecuted = true;
				}
				PC = cflOp1 >> 2;
				break;
			case 0b011: // br
				PC++;
				if (isBundle(instr))
				{
					tick();
					bundleAlreadyExecuted = true;
				}
				tick();
				tick();
				PC = cflOp1 >> 2;
				break;
			default:
				throw std::runtime_error("Invalid Op and delay combination for CFLrs format.");
			}
			break;
		case InstrFormat::CFLrt:
			switch (cflFunc)
			{
			case 0b100: // brcfnd
				PC++;
				if (isBundle(instr))
				{
					tick();
					bundleAlreadyExecuted = true;
				}
				Base = cflOp1 >> 2;
				PC = cflOp2 >> 2;
				break;
			case 0b101: // brcf
				PC++;
				if (isBundle(instr))
				{
					tick();
					bundleAlreadyExecuted = true;
				}
				tick();
				tick();
				tick();
				Base = cflOp1 >> 2;
				PC = cflOp2 >> 2;
				break;
			default:
				throw std::runtime_error("Invalid Op and delay combination for CFLrt format.");
			}
			break;
		case InstrFormat::FPUr:
			save_and_set_fp_env();
			switch (fpuFunc)
			{
			case 0b0000:
				setFPR(rd, fpuOp1 + fpuOp2);
				break;
			case 0b0001:
				setFPR(rd, fpuOp1 - fpuOp2);
				break;
			case 0b0010:
				setFPR(rd, fpuOp1 * fpuOp2);
				break;
			case 0b0011:
				setFPR(rd, fpuOp1 / fpuOp2);
				break;
			case 0b0100:
				setFPR(rd, std::copysignf(fpuOp1, fpuOp2));
				break;
			case 0b0101:
				setFPR(rd, std::copysignf(fpuOp1, (!std::signbit(fpuOp2)) ? -0.0f : +0.0f));
				break;
			case 0b0110:
				setFPR(rd, std::copysignf(fpuOp1, (std::signbit(fpuOp1) != std::signbit(fpuOp2)) ? -0.0f : +0.0f));
				break;
			default:
				restore_fp_env();
				throw std::runtime_error("Invalid FPUr func: " + std::to_string(fpuFunc));
			}
			PC++;
			restore_fp_env();
			break;
		case InstrFormat::FPUl:
			save_and_set_fp_env();
			switch (fpuFunc)
			{
			case 0b0000:
				setFPR(rd, fpuOp1 + fpuOp2);
				break;
			case 0b0001:
				setFPR(rd, fpuOp1 - fpuOp2);
				break;
			case 0b0010:
				setFPR(rd, fpuOp1 * fpuOp2);
				break;
			case 0b0011:
				setFPR(rd, fpuOp1 / fpuOp2);
				break;
			default:
				restore_fp_env();
				throw std::runtime_error("Invalid FPUl func: " + std::to_string(fpuFunc));
			}
			PC += 2;
			restore_fp_env();
			break;
		case InstrFormat::FPUrs:
			save_and_set_fp_env();
			switch (fpuFunc)
			{
			case 0b0000:
				setFPR(rd, std::sqrt(fpuOp1));
				break;
			default:
				restore_fp_env();
				throw std::runtime_error("Invalid FPUrs func: " + std::to_string(fpuFunc));
			}
			PC++;
			restore_fp_env();
			break;
		case InstrFormat::FPCt:
			save_and_set_fp_env();
			switch (fpuFunc)
			{
			case 0b0000:
				setFPR(rd, (float)(int32_t)getGPR(rs1));
				break;
			case 0b0001:
				setFPR(rd, (float)(uint32_t)getGPR(rs1));
				break;
			case 0b0010:
            {
				uint32_t tmp = getGPR(rs1);
				setFPR(rd, reinterpret_cast<float&>(tmp));
            }
				break;
			default:
				restore_fp_env();
				throw std::runtime_error("Invalid FPCt func: " + std::to_string(fpuFunc));
			}
			PC++;
			restore_fp_env();
			break;
		case InstrFormat::FPCf:
			save_and_set_fp_env();
			switch (fpuFunc)
			{
			case 0b0000:
				setGPR(rd, (int32_t)getFPR(rs1));
				break;
			case 0b0001:
				setGPR(rd, (uint32_t)getFPR(rs1));
				break;
			case 0b0010:
            {
				float tmp = getFPR(rs1);
				setGPR(rd, reinterpret_cast<uint32_t&>(tmp));
            }
				break;
			case 0b0011:
				setGPR(rd, classifyFloat(getFPR(rs1)));
				break;
			default:
				restore_fp_env();
				throw std::runtime_error("Invalid FPCf func: " + std::to_string(fpuFunc));
			}
			PC++;
			restore_fp_env();
			break;
		case InstrFormat::FPUc:
			save_and_set_fp_env();
			switch (fpuFunc)
			{
			case 0b0000:
				setPRR(pd, fpuOp1 == fpuOp2);
				break;
			case 0b0001:
				setPRR(pd, fpuOp1 < fpuOp2);
				break;
			case 0b0010:
				setPRR(pd, fpuOp1 <= fpuOp2);
				break;
			default:
				restore_fp_env();
				throw std::runtime_error("Invalid FPUc func: " + std::to_string(fpuFunc));
			}
			PC++;
			restore_fp_env();
			break;
		default:
			throw std::runtime_error("Invalid instruction format.");
		}

		return std::make_tuple(halt, bundleAlreadyExecuted);
	}
};

int main(int argc, char const *argv[])
{
	
	if (argc != 3)
	{
		std::cerr << "Usage: ISASim <input .bin> <output uart>" << std::endl;
		return 1;
	}
	

	std::string binFilePath = argv[1];
	std::string uartFilePath = argv[2];
    
    #pragma STDC FENV_ACCESS ON
    std::fesetround(FE_TONEAREST);
	

	//std::string binFilePath = "/home/cake/t-crest/patmos/asm-tests/tests/bin/fadds.bin";
	//std::string uartFilePath = "/home/cake/t-crest/patmos/asm-tests/tests/isa-actual/fadds.uart";



	try
	{
		std::ifstream inputFile(binFilePath, std::ios::binary);
		std::ofstream outputFile(uartFilePath, std::ios::binary);

		// read binary file
		auto instructions = std::make_unique<std::vector<uint32_t>>();
		while (inputFile.good())
		{
			uint32_t instr = 0;

			// read into buffer
			inputFile.read((char*)&instr, sizeof(uint32_t));

			// check how much was read
			std::streamsize count = inputFile.gcount();
			if (count != sizeof(uint32_t) && count != 0)
			{
				throw std::runtime_error("The binary file was not a multiple of 4.");
			}
			else if (count == 0)
			{
				break;
			}

			instr = __builtin_bswap32(instr);
			instructions->push_back(instr);
		}
		
		inputFile.close();

		simulator sim(std::move(instructions));

		while (!sim.tick())
		{
		}

		auto uartResult = sim.getUart();
		for (size_t i = 0; i < uartResult->size(); i++)
		{
			outputFile << (*uartResult)[i];
		}
		outputFile.close();

		std::cout << "simulation done." << std::endl;
	}
	catch(const std::exception& e)
	{
		std::cerr << e.what() << std::endl;
		return 1;
	}
	
	return 0;
}
