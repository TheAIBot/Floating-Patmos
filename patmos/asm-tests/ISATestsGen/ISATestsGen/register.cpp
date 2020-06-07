
#include "register.h"

namespace patmos
{
	regInfo::regInfo(std::string name, int32_t index, bool isReadonly, int32_t readonly_value) : 
		regName(name), regIndex(index), isReadonly(isReadonly), readonly_value(readonly_value)
	{}

	std::string regInfo::getBaseName()
	{
		return regName[0] + std::to_string(regIndex);
	}

	regInfo getRandomReg(std::mt19937& rngGen, const std::vector<regInfo>& regSrc)
	{
		std::uniform_int_distribution<int32_t> distribution(0, regSrc.size() - 1);
		return regSrc[distribution(rngGen)];
	}

	const std::vector<regInfo> registers::GPRs = {
		regInfo("r0", 0, true, 0),
		regInfo("r1", 1),
		regInfo("r2", 2),
		regInfo("r3", 3),
		regInfo("r4", 4),
		regInfo("r5", 5),
		regInfo("r6", 6),
		regInfo("r7", 7),
		regInfo("r8", 8),
		regInfo("r9", 9),
		regInfo("r10", 10),
		regInfo("r11", 11),
		regInfo("r12", 12),
		regInfo("r13", 13),
		regInfo("r14", 14),
		regInfo("r15", 15),
		regInfo("r16", 16),
		regInfo("r17", 17),
		regInfo("r18", 18),
		regInfo("r19", 19),
		regInfo("r20", 20),
		regInfo("r21", 21),
		regInfo("r22", 22),
		regInfo("r23", 23),
		regInfo("r24", 24),
		regInfo("r25", 25),
		regInfo("r26", 26),
		regInfo("r27", 27),
		regInfo("r28", 28),
		regInfo("r29", 29),
		regInfo("r30", 30),
		regInfo("r31", 31)
	};

	const std::vector<regInfo> registers::FPRs = {
		regInfo("f0", 0),
		regInfo("f1", 1),
		regInfo("f2", 2),
		regInfo("f3", 3),
		regInfo("f4", 4),
		regInfo("f5", 5),
		regInfo("f6", 6),
		regInfo("f7", 7),
		regInfo("f8", 8),
		regInfo("f9", 9),
		regInfo("f10", 10),
		regInfo("f11", 11),
		regInfo("f12", 12),
		regInfo("f13", 13),
		regInfo("f14", 14),
		regInfo("f15", 15),
		regInfo("f16", 16),
		regInfo("f17", 17),
		regInfo("f18", 18),
		regInfo("f19", 19),
		regInfo("f20", 20),
		regInfo("f21", 21),
		regInfo("f22", 22),
		regInfo("f23", 23),
		regInfo("f24", 24),
		regInfo("f25", 25),
		regInfo("f26", 26),
		regInfo("f27", 27),
		regInfo("f28", 28),
		regInfo("f29", 29),
		regInfo("f30", 30),
		regInfo("f31", 31)
	};

	const std::vector<regInfo> registers::PRs = {
		regInfo("p0", 0, true, 1),
		regInfo("p1", 1),
		regInfo("p2", 2),
		regInfo("p3", 3),
		regInfo("p4", 4),
		regInfo("p5", 5),
		regInfo("p6", 6),
		regInfo("p7", 7)
	};

	const std::vector<regInfo> registers::SPRs = {
		regInfo("s0", 0),
		regInfo("s1", 1),
		regInfo("sfcsr", 1),
		regInfo("s2", 2),
		regInfo("sl", 2),
		regInfo("s3", 3),
		regInfo("sh", 3),
		//regInfo("s4", 4),
		regInfo("s5", 5),
		regInfo("ss", 5),
		regInfo("s6", 6),
		regInfo("st", 6),
		regInfo("s7", 7),
		regInfo("srb", 7),
		regInfo("s8", 8),
		regInfo("sro", 8),
		regInfo("s9", 9),
		regInfo("sxb", 9),
		regInfo("s10", 10),
		regInfo("sxo", 10),
		//regInfo("s11", 11),
		//regInfo("s12", 12),
		//regInfo("s13", 13),
		//regInfo("s14", 14),
		//regInfo("s15", 15)
	};
}