#pragma once

#include <cfenv>
#include <cstdint>
#include <string>
#include <stdexcept>

namespace patmos
{
	namespace rounding
	{
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

		patmos_round_mode x86_to_patmos_round(const x86_round_mode round);

		std::string patmos_round_to_string(const patmos_round_mode round);
	}
}