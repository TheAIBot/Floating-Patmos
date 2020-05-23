#pragma once

template<typename T>
struct valueRange
{
	T inclusiveMin;
	T inclusiveMax;

	valueRange(T inclusiveMin, T inclusiveMax) : inclusiveMin(inclusiveMin), inclusiveMax(inclusiveMax)
	{ }
};

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