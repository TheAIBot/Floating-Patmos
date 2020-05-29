#pragma once

#include <random>
#include <vector>
#include <string>
#include <cstdint>

struct regInfo
{
	std::string regName;
	int32_t regIndex;
	bool isReadonly;

	regInfo(std::string name, int32_t index, bool isReadonly = false);

	std::string getBaseName();
};

regInfo getRandomReg(std::mt19937& rngGen, const std::vector<regInfo>& regSrc);

class registers
{
public:
	static const std::vector<regInfo> GPRs;
	static const std::vector<regInfo> FPRs;
	static const std::vector<regInfo> PRs;
	static const std::vector<regInfo> SPRs;

private:
	registers() {}
};