
#include "float-csr.h"

namespace patmos
{
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

	namespace exception
	{
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
	}
}


