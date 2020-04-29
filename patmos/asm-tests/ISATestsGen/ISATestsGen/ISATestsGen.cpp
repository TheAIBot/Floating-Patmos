// ISATestsGen.cpp : Defines the entry point for the application.
//

#include <iostream>
#include <fstream>
#include <string>
#include <functional>
#include <limits>
#include "ISATestsGen.h"

std::string TESTS_DIR_ASM = "../tests/asm";
std::string TESTS_DIR_EXPECTED = "../tests/expected";

class isaTest
{
private:
	std::ofstream asmFile;
	std::ofstream expectedFile;

public:
	isaTest(std::string asmfilepath, std::string expfilepath, std::string filename)
		: asmFile(std::ofstream(asmfilepath + '/' + filename + ".s")),
		expectedFile(std::ofstream(expfilepath + '/' + filename + ".txt"))
	{
		asmFile << ".word 100" << '\n';
	}

	void addInstr(std::string instr)
	{
		asmFile << instr << '\n';
	}

	void setFloatReg(std::string reg, float value)
	{
		addInstr("fmvis " + reg + " = r0");
		addInstr("faddl " + reg + " = " + reg + ", " + std::to_string(value));
	}

	void expectRegisterValue(std::string reg, int32_t value)
	{
		expectedFile << reg << " : " << std::to_string(value) << '\n';
	}
	void expectRegisterValue(std::string reg, float value)
	{
		expectRegisterValue(reg, reinterpret_cast<int32_t&>(value));
	}

	void close()
	{
		asmFile.close();
		expectedFile.close();
	}
};

void makeFPUrTest(std::string instrName, std::function<float(float, float)> op)
{
	isaTest test(TESTS_DIR_ASM, TESTS_DIR_EXPECTED, instrName);
	float f1 = 0.0f;
	float f2 = 1.0f;
	float f3 = 3.27f;
	float f4 = -5.78;
	float f5 = 3.14159265359f;
	test.setFloatReg("f1", f1);
	test.setFloatReg("f2", f2);
	test.setFloatReg("f3", f3);
	test.setFloatReg("f4", f4);
	test.setFloatReg("f5", f5);
	test.addInstr(instrName + " f10 = f1, f2");
	test.addInstr(instrName + " f11 = f2, f3");
	test.addInstr(instrName + " f12 = f3, f4");
	test.addInstr(instrName + " f13 = f4, f5");
	test.expectRegisterValue("f10", op(f1, f2));
	test.expectRegisterValue("f11", op(f2, f3));
	test.expectRegisterValue("f12", op(f3, f4));
	test.expectRegisterValue("f13", op(f4, f5));
	test.close();
}

void makeFPUlTest(std::string instrName, std::function<float(float, float)> op)
{
	isaTest test(TESTS_DIR_ASM, TESTS_DIR_EXPECTED, instrName);
	float f1 = 0.0f;
	float f2 = 1.0f;
	float f3 = 3.27f;
	float f4 = -5.78;
	float f5 = 3.14159265359f;
	test.setFloatReg("f1", f1);
	test.setFloatReg("f2", f2);
	test.setFloatReg("f3", f3);
	test.setFloatReg("f4", f4);
	test.setFloatReg("f5", f5);
	test.addInstr(instrName + " f10 = f1, " + std::to_string(f2));
	test.addInstr(instrName + " f11 = f2, " + std::to_string(f3));
	test.addInstr(instrName + " f12 = f3, " + std::to_string(f4));
	test.addInstr(instrName + " f13 = f4, " + std::to_string(f5));
	test.expectRegisterValue("f10", op(f1, f2));
	test.expectRegisterValue("f11", op(f2, f3));
	test.expectRegisterValue("f12", op(f3, f4));
	test.expectRegisterValue("f13", op(f4, f5));
	test.close();
}

void makeFPUrsTest(std::string instrName, std::function<float(float)> op)
{
	isaTest test(TESTS_DIR_ASM, TESTS_DIR_EXPECTED, instrName);
	float f1 = 0.0f;
	float f2 = 1.0f;
	float f3 = 3.27f;
	float f4 = -5.78;
	float f5 = 3.14159265359f;
	test.setFloatReg("f1", f1);
	test.setFloatReg("f2", f2);
	test.setFloatReg("f3", f3);
	test.setFloatReg("f4", f4);
	test.setFloatReg("f5", f5);
	test.addInstr(instrName + " f10 = f1");
	test.addInstr(instrName + " f11 = f2");
	test.addInstr(instrName + " f12 = f3");
	test.addInstr(instrName + " f13 = f4");
	test.addInstr(instrName + " f14 = f5");
	test.expectRegisterValue("f10", op(f1));
	test.expectRegisterValue("f11", op(f2));
	test.expectRegisterValue("f12", op(f3));
	test.expectRegisterValue("f13", op(f4));
	test.expectRegisterValue("f14", op(f5));
	test.close();
}

void makeFPCtTest(std::string instrName, std::function<float(int32_t)> op)
{
	isaTest test(TESTS_DIR_ASM, TESTS_DIR_EXPECTED, instrName);
	int32_t r1 = 0;
	int32_t r2 = 1;
	int32_t r3 = 3;
	int32_t r4 = -5;
	int32_t r5 = 33245;
	test.setFloatReg("r1", r1);
	test.setFloatReg("r2", r2);
	test.setFloatReg("r3", r3);
	test.setFloatReg("r4", r4);
	test.setFloatReg("r5", r5);
	test.addInstr(instrName + " f10 = r1");
	test.addInstr(instrName + " f11 = r2");
	test.addInstr(instrName + " f12 = r3");
	test.addInstr(instrName + " f13 = r4");
	test.addInstr(instrName + " f14 = r5");
	test.expectRegisterValue("f10", op(r1));
	test.expectRegisterValue("f11", op(r2));
	test.expectRegisterValue("f12", op(r3));
	test.expectRegisterValue("f13", op(r4));
	test.expectRegisterValue("f14", op(r5));
	test.close();
}

void makeFPCfTest(std::string instrName, std::function<int32_t(float)> op)
{
	isaTest test(TESTS_DIR_ASM, TESTS_DIR_EXPECTED, instrName);
	float f1 = 0.0f;
	float f2 = 1.0f;
	float f3 = 3.27f;
	float f4 = -5.78;
	float f5 = 3.14159265359f;
	test.setFloatReg("f1", f1);
	test.setFloatReg("f2", f2);
	test.setFloatReg("f3", f3);
	test.setFloatReg("f4", f4);
	test.setFloatReg("f5", f5);
	test.addInstr(instrName + " r10 = f1");
	test.addInstr(instrName + " r11 = f2");
	test.addInstr(instrName + " r12 = f3");
	test.addInstr(instrName + " r13 = f4");
	test.addInstr(instrName + " r14 = f5");
	test.expectRegisterValue("r10", op(f1));
	test.expectRegisterValue("r11", op(f2));
	test.expectRegisterValue("r12", op(f3));
	test.expectRegisterValue("r13", op(f4));
	test.expectRegisterValue("r14", op(f5));
	test.close();
}

int32_t classifyFloat(float a)
{
	int32_t ai = reinterpret_cast<int32_t&>(a);
	bool isQuietNan = (ai & (1 << 22)) != 0;

	return (std::isnan(a) && !isQuietNan) << 9 |
		(std::isnan(a)) << 8 |
		(std::signbit(a) && std::isinf(a)) << 7 |
		(std::signbit(a) && std::isnormal(a)) << 6 |
		(std::signbit(a) && !std::isnormal(a)) << 5 |
		(std::signbit(a) && a == 0.0) << 4 |
		(!std::signbit(a) && a == 0.0) << 3 |
		(!std::signbit(a) && !std::isnormal(a)) << 2 |
		(!std::signbit(a) && std::isnormal(a)) << 1 |
		(!std::signbit(a) && std::isinf(a)) << 0;
}

void makeClassifyTest(std::string instrName)
{
	isaTest test(TESTS_DIR_ASM, TESTS_DIR_EXPECTED, instrName);
	float f1 = std::numeric_limits<float>::signaling_NaN();
	float f2 = std::numeric_limits<float>::quiet_NaN();
	float f3 = std::copysign(std::numeric_limits<float>::infinity(), -0.0);
	float f4 = -3.5f;
	float f5 = std::copysign(std::numeric_limits<float>::denorm_min(), -0.0);
	float f6 = -0.0f;
	float f7 = +0.0f;
	float f8 = std::copysign(std::numeric_limits<float>::denorm_min(), 0.0);
	float f9 = 3.5f;
	float f10 = std::copysign(std::numeric_limits<float>::infinity(), 0.0);
	test.setFloatReg("f1", f1);
	test.setFloatReg("f2", f2);
	test.setFloatReg("f3", f3);
	test.setFloatReg("f4", f4);
	test.setFloatReg("f5", f5);
	test.setFloatReg("f6", f6);
	test.setFloatReg("f7", f7);
	test.setFloatReg("f8", f8);
	test.setFloatReg("f9", f9);
	test.setFloatReg("f10", f10);
	test.addInstr(instrName + " r10 = f1");
	test.addInstr(instrName + " r11 = f2");
	test.addInstr(instrName + " r12 = f3");
	test.addInstr(instrName + " r13 = f4");
	test.addInstr(instrName + " r14 = f5");
	test.addInstr(instrName + " r15 = f6");
	test.addInstr(instrName + " r16 = f7");
	test.addInstr(instrName + " r17 = f8");
	test.addInstr(instrName + " r18 = f9");
	test.addInstr(instrName + " r19 = f10");
	test.expectRegisterValue("r10", classifyFloat(f1));
	test.expectRegisterValue("r11", classifyFloat(f2));
	test.expectRegisterValue("r12", classifyFloat(f3));
	test.expectRegisterValue("r13", classifyFloat(f4));
	test.expectRegisterValue("r14", classifyFloat(f5));
	test.expectRegisterValue("r15", classifyFloat(f6));
	test.expectRegisterValue("r16", classifyFloat(f7));
	test.expectRegisterValue("r17", classifyFloat(f8));
	test.expectRegisterValue("r18", classifyFloat(f9));
	test.expectRegisterValue("r19", classifyFloat(f10));
	test.close();
}

void makeFPUcTest(std::string instrName, std::function<bool(float, float)> op)
{
	isaTest test(TESTS_DIR_ASM, TESTS_DIR_EXPECTED, instrName);
	float f1 = 0.0f;
	float f2 = 1.0f;
	float f3 = 3.27f;
	float f4 = -5.78;
	float f5 = 3.14159265359f;
	test.setFloatReg("f1", f1);
	test.setFloatReg("f2", f2);
	test.setFloatReg("f3", f3);
	test.setFloatReg("f4", f4);
	test.setFloatReg("f5", f5);
	test.addInstr(instrName + " p1 = f1, f2");
	test.addInstr(instrName + " p2 = f2, f3");
	test.addInstr(instrName + " p3 = f3, f4");
	test.addInstr(instrName + " p4 = f4, f5");
	test.addInstr(instrName + " p5 = f5, f5");
	test.expectRegisterValue("p1", op(f1, f2));
	test.expectRegisterValue("p2", op(f2, f3));
	test.expectRegisterValue("p3", op(f3, f4));
	test.expectRegisterValue("p4", op(f4, f5));
	test.expectRegisterValue("p5", op(f5, f5));
	test.close();
}

int main()
{
	// FPUr tests
	makeFPUrTest("fadds", std::plus<float>());
	makeFPUrTest("fsubs", std::minus<float>());
	makeFPUrTest("fmuls", std::multiplies<float>());
	makeFPUrTest("fdivs", std::divides<float>());
	makeFPUrTest("fsgnjs", std::copysignf);
	makeFPUrTest("fsgnjns", [](float a, float b) { return std::copysignf(a, (!std::signbit(b)) ? -0.0f : +0.0f); });
	makeFPUrTest("fsgnjxs", [](float a, float b) { return std::copysignf(a, (std::signbit(a) != std::signbit(b)) ? -0.0f : +0.0f); });

	// FPUl tests
	makeFPUlTest("faddsl", std::plus<float>());
	makeFPUlTest("fsubsl", std::minus<float>());
	makeFPUlTest("fmulsl", std::multiplies<float>());
	makeFPUlTest("fdivsl", std::divides<float>());

	// FPUrs tests
	makeFPUrsTest("fsqrts", std::sqrtf);

	// FPCt tests
	makeFPCtTest("fcvtis", [](int32_t a) { return static_cast<float>(a); });
	makeFPCtTest("fcvtus", [](int32_t a) { return static_cast<float>(static_cast<uint32_t>(a)); });
	makeFPCtTest("fmvis", [](int32_t a) { return reinterpret_cast<float&>(a); });

	// FPCf tests
	makeFPCtTest("fcvtsi", [](float a) { return static_cast<int32_t>(a); });
	makeFPCtTest("fcvtsu", [](float a) { return static_cast<uint32_t>(a); });
	makeFPCtTest("fmvsi", [](float a) { return reinterpret_cast<int32_t&>(a); });
	makeFPCtTest("fclasss", classifyFloat);
	makeClassifyTest("fclasss");

	//FPUc tests
	makeFPUcTest("feqs", [](float a, float b) { return a == b; });
	makeFPUcTest("flts", [](float a, float b) { return a < b; });
	makeFPUcTest("fles", [](float a, float b) { return a <= b; });

	std::cout << "Hello CMake." << std::endl;
	return 0;
}
