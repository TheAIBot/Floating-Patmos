#pragma once

#include <cstdint>
#include <string>
#include <filesystem>
#include <cstring>

namespace patmos
{

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

	template<class FWIter>
	std::string string_join(FWIter start, const FWIter end, const std::string delim)
	{
		std::string joined = "";
		if (start != end)
		{
			while (start + 1 != end)
			{
				joined += *start + delim;
				start++;
			}

			joined += *start;
		}

		return joined;
	}

	template<typename T>
	std::string string_join(const T& container, const std::string delim)
	{
		return string_join(std::begin(container), std::end(container), delim);
	}

	template<typename... Ts>
	std::string create_dir_if_not_exists(Ts... path_parts)
	{
		std::array<std::string, sizeof...(Ts)> str_arr = { path_parts... };

		std::string dir_path = string_join(str_arr, "/");
		if (!std::filesystem::exists(dir_path) || !std::filesystem::is_directory(dir_path))
		{
			std::filesystem::create_directory(dir_path);
		}

		return dir_path;
	}

	int32_t fti(float f);
	float itf(int32_t i);
}