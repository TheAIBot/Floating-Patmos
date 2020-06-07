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
		x86_round_mode patmos_to_x86_round(const patmos_round_mode round);

		std::string patmos_round_to_string(const patmos_round_mode round);
	}

	namespace exception
	{
		enum class patmos_exceptions
		{
			NV = 0b10000,
			DZ = 0b01000,
			OF = 0b00100,
			UF = 0b00010,
			NX = 0b00001,
		};

		int32_t x86_to_patmos_exceptions(const std::fexcept_t& ex);
		std::fexcept_t patmos_to_x86_exceptions(const int32_t ex);
	}
}