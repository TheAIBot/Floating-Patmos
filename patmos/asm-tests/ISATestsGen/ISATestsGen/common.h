#pragma once

#include <cstdint>
#include <string>
#include <filesystem>

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

	template<std::size_t size>
	std::string string_join(const std::array<std::string, size>& strs, std::string delim)
	{
		std::string joined = "";
		if constexpr (size > 0)
		{
			for (size_t i = 0; i < size - 1; i++)
			{
				joined += strs[i] + delim;
			}
			joined += strs[size - 1];
		}

		return joined;
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
}