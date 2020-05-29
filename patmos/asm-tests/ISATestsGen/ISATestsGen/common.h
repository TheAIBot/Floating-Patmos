#pragma once

#include <cstdint>

enum class Pipes
{
	First,
	Second,
	Both
};

struct mulRes
{
	uint32_t Low;
	uint32_t High;
};