
#include "float-csr.h"

namespace rounding
{
	patmos_round_mode x86_to_patmos_round(const x86_round_mode round)
	{
		switch (round)
		{
		case x86_round_mode::RNE:
			return patmos_round_mode::RNE;
		case x86_round_mode::RTZ:
			return patmos_round_mode::RTZ;
		case x86_round_mode::RDN:
			return patmos_round_mode::RDN;
		case x86_round_mode::RUP:
			return patmos_round_mode::RUP;
		default:
			throw std::runtime_error("Invalid x86 rounding mode. Mode: " + std::to_string((int32_t)round));
		}
	}

	std::string patmos_round_to_string(const patmos_round_mode round)
	{
		switch (round)
		{
		case patmos_round_mode::RNE:
			return "rne";
		case patmos_round_mode::RTZ:
			return "rtz";
		case patmos_round_mode::RDN:
			return "rdn";
		case patmos_round_mode::RUP:
			return "rup";
		default:
			throw std::runtime_error("Invalid patmos rounding mode. Mode: " + std::to_string((int32_t)round));
		}
	}
}


