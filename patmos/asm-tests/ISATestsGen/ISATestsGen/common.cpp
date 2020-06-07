
#include "common.h"

namespace patmos
{
    int32_t fti(float f)
	{
        static_assert(sizeof(int32_t) == sizeof(float));
		int32_t i;
		std::memcpy(&i, &f, sizeof(f));
		return i;
	}

	float itf(int32_t i)
	{
        static_assert(sizeof(int32_t) == sizeof(float));
		float f;
		std::memcpy(&f, &i, sizeof(i));
		return f;
	}
}