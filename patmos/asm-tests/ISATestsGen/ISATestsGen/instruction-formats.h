#pragma once

#include "common.h"
#include "float-csr.h"
#include "register.h"
#include "tester.h"
#include <string>
#include <array>
#include <tuple>
#include <cstdint>
#include <stdexcept>
#include <functional>
#include <numeric>

template<typename T>
struct valueRange
{
	T inclusiveMin;
	T inclusiveMax;

	constexpr valueRange(T inclusiveMin, T inclusiveMax) : inclusiveMin(inclusiveMin), inclusiveMax(inclusiveMax)
	{ }
};

class opSource
{
private:
	const std::vector<regInfo>* regSrc;
	valueRange<int32_t> intRange;
	valueRange<float> floatRange;

public:
	opSource(const std::vector<regInfo>& regSrc, valueRange<int32_t> intRange) : regSrc(&regSrc), intRange(intRange), floatRange(valueRange<float>(0, 0))
	{ }

	opSource(const std::vector<regInfo>& regSrc, valueRange<float> floatRange) : regSrc(&regSrc), intRange(valueRange<int32_t>(0, 0)), floatRange(floatRange)
	{ }

	opSource(valueRange<int32_t> intRange) : regSrc(nullptr), intRange(intRange), floatRange(valueRange<float>(0, 0))
	{ }

	opSource(valueRange<float> floatRange) : regSrc(nullptr), intRange(valueRange<int32_t>(0, 0)), floatRange(floatRange)
	{ }

	opSource() : regSrc(nullptr), intRange(valueRange<int32_t>(0, 0)), floatRange(valueRange<float>(0, 0))
	{ }

	regInfo getRandomRegister(std::mt19937& rngGen) const
	{
		return getRandomReg(rngGen, *regSrc);
	}

	template<class T>
	T getRandom(std::mt19937& rngGen) const
	{
		static_assert(true, "getRandom does not support ");
		return 0;
	}

	template<>
	int32_t getRandom<int32_t>(std::mt19937& rngGen) const
	{
		std::uniform_int_distribution<int32_t> distribution(intRange.inclusiveMin, intRange.inclusiveMax);
		return distribution(rngGen);
	}

	template<>
	float getRandom<float>(std::mt19937& rngGen) const
	{
		std::uniform_real_distribution<float> distribution(floatRange.inclusiveMin, floatRange.inclusiveMax);
		return distribution(rngGen);
	}
};



class baseFormat
{
protected:
	std::string instrName;
	Pipes pipe;

	baseFormat(std::string name, Pipes pipe) : instrName(name), pipe(pipe)
	{ }

public:
	//virtual void makeTests(std::string asmfilepath, std::string expfilepath, std::mt19937& rngGen, int32_t testCount) const = 0;

	virtual void make_single_op_tests(std::string asmfilepath, std::string expfilepath, std::mt19937& rngGen, int32_t testCount) const = 0;
	//virtual void make_loop_op_tests(std::string asmfilepath, std::string expfilepath, std::mt19937& rngGen, int32_t testCount) const = 0;
	//virtual void make_forwarding_tests(std::string asmfilepath, std::string expfilepath, std::mt19937& rngGen, int32_t testCount) const = 0;
	//virtual void make_pipelined_tests(std::string asmfilepath, std::string expfilepath, std::mt19937& rngGen, int32_t testCount) const = 0;

	virtual std::vector<int32_t> encode(std::string instr) const = 0;
};



enum class reg_type
{
	GPR,
	FPR,
	SPR,
	PR
};

constexpr const std::vector<regInfo>& get_register_from_type(reg_type type)
{
	switch (type)
	{
	case reg_type::GPR:
		return registers::GPRs;
	case reg_type::FPR:
		return registers::FPRs;
	case reg_type::SPR:
		return registers::SPRs;
	case reg_type::PR:
		return registers::PRs;
	default:
		throw std::runtime_error("Invalid register type.");
	}
}



template<int32_t bit_count>
struct bits
{
	static const int32_t bit_count = bit_count;
};

struct explicit_dst_reg {};
struct implicit_dst_reg {};
template<typename T> struct implicit_dst_regs : bits<0>, implicit_dst_reg { using type = T; };

template<reg_type t> struct reg_dst : bits<0> { };
template<> struct reg_dst<reg_type::GPR> : bits<5>, explicit_dst_reg
{
	using type = int32_t; 
	static const reg_type regType = reg_type::GPR; 
};
template<> struct reg_dst<reg_type::FPR> : bits<5>, explicit_dst_reg
{
	using type = float; 
	static const reg_type regType = reg_type::FPR; 
};
template<> struct reg_dst<reg_type::SPR> : bits<5>, explicit_dst_reg
{
	using type = int32_t; 
	static const reg_type regType = reg_type::SPR; 
};
template<> struct reg_dst<reg_type::PR> : bits<3>, explicit_dst_reg
{ 
	using type = bool; 
	static const reg_type regType = reg_type::PR;
};
struct pred_reg : bits<4> {};

template<reg_type t> struct reg_src : bits<0> { };
template<> struct reg_src<reg_type::GPR> : bits<5> 
{ 
	using type = int32_t;
	valueRange<int32_t> range;
	reg_src(valueRange<int32_t> range) : range(range) {}
	reg_src() : range(valueRange<int32_t>(0, 0)) {}
};
template<> struct reg_src<reg_type::FPR> : bits<5> 
{ 
	using type = float;
	valueRange<float> range;
	reg_src(valueRange<float> range) : range(range) {}
	reg_src() : range(valueRange<float>(0, 0)) {}
};
template<> struct reg_src<reg_type::SPR> : bits<5> 
{ 
	using type = int32_t;
	valueRange<int32_t> range;
	reg_src(valueRange<int32_t> range) : range(range) {}
	reg_src() : range(valueRange<int32_t>(0, 0)) {}
};
template<> struct reg_src<reg_type::PR>  : bits<4> 
{ 
	using type = bool;
	valueRange<int32_t> range;
	reg_src() : range(valueRange<int32_t>(0, 1)) {}
};

template<int32_t bit_count, typename T> struct signed_imm : bits<bit_count> 
{ 
	using type = T;
	valueRange<T> range;
	signed_imm(valueRange<T> range) : range(range) {}
	signed_imm() : range(valueRange<T>(0, 0)) {}
};
template<int32_t bit_count, typename T> struct unsigned_imm : bits<bit_count> 
{ 
	using type = T; 
	valueRange<T> range;
	unsigned_imm(valueRange<T> range) : range(range) {}
	unsigned_imm() : range(valueRange<T>(0, 0)) {}
};

template<int32_t bit_count> struct unused_bits : bits<bit_count> {};

template<int32_t bit_count, int32_t bit_value> struct hardcoded_bits : bits<bit_count>
{
	static_assert((((1 << bit_count) - 1) & bit_value) == bit_value, "Value doesn't fit in bits.");
	static const int32_t value = bit_value;
};

template<int32_t bit_count, int32_t name_index> struct modify_bits : bits<bit_count>
{
	static const int32_t index = name_index;
};

template<int32_t bit_count> struct instr_dep_bits : bits<bit_count>
{
	const int32_t value;

	instr_dep_bits(const int32_t bit_value) : value(bit_value) {}
};


template<bool allowed> struct filter { static constexpr bool value = allowed; };

template<typename T> struct reg_dst_filter : filter<false> {};
template<reg_type T> struct reg_dst_filter<reg_dst<T>> : filter<true> {};
template<typename T> struct reg_dst_filter<implicit_dst_regs<T>> : filter<true> {};

template<typename T> struct reg_src_filter : filter<false> {};
template<reg_type T> struct reg_src_filter<reg_src<T>> : filter<true> {};

template<typename T> struct op_src_filter : filter<false> { using type = std::tuple<>; };
template<reg_type T> struct op_src_filter<reg_src<T>> : filter<true> { using type = std::tuple<typename reg_src<T>::type>; };
template<int32_t bit_count, typename T> struct op_src_filter<signed_imm<bit_count, T>> : filter<true> { using type = std::tuple<typename signed_imm<bit_count, T>::type>; };
template<int32_t bit_count, typename T> struct op_src_filter<unsigned_imm<bit_count, T>> : filter<true> { using type = std::tuple<typename unsigned_imm<bit_count, T>::type>; };

template<bool has_long_imm, int32_t bit_count> 
struct check_instr_size 
{
	static_assert((!has_long_imm && bit_count == 32) || (has_long_imm && bit_count == 64), "Incorrect instruction size.");
};

template<typename Ret, typename Args, size_t size, std::size_t... Is>
struct make_func_type : make_func_type<Ret, Args, size - 1, size - 1, Is...> { };
template<typename Ret, typename Args, std::size_t... Is>
struct make_func_type<Ret, Args, 0, Is...>
{
	using type = std::function<Ret(typename std::tuple_element<Is, Args>::type ...)>;
};

template<int32_t size>
std::string string_join(const std::array<std::string, size>& strs, std::string delim)
{
	std::string joined = "";
	if constexpr (size > 0)
	{
		for (size_t i = 0; i < strs.size() - 1; i++)
		{
			joined += strs[i] + ", ";
		}
		joined += strs[strs.size() - 1];
	}

	return joined;
}

template<bool useMakeTests, typename... TArgs>
class uni_format : public baseFormat
{
private:
	using FRetReg = typename std::tuple_element<0, decltype(std::tuple_cat(std::conditional_t<reg_dst_filter<TArgs>::value, std::tuple<TArgs>, std::tuple<>>() ...))>::type;
	using FRet = typename FRetReg::type;
	using FArgs = decltype(std::tuple_cat(op_src_filter<TArgs>::type() ...));
	using FType = typename make_func_type<FRet, FArgs, std::tuple_size<FArgs>::value>::type;
	using FRegs = decltype(std::tuple_cat(std::conditional_t<op_src_filter<TArgs>::value, std::tuple<TArgs>, std::tuple<>>()...));

	static constexpr int32_t func_arg_count = std::tuple_size<FArgs>::value;
	static constexpr int32_t intr_bit_count = (TArgs::bit_count + ...);
	static constexpr bool has_long_imm = std::is_same<hardcoded_bits<1, 1>, typename std::tuple_element<0, std::tuple<TArgs...>>::type>::value;
	static constexpr bool has_explicit_dst_reg = std::is_same<FRetReg, explicit_dst_reg>::value;

	check_instr_size<has_long_imm, intr_bit_count> _;


	template<std::size_t I, typename Sources> 
	struct is_source_from_reg
	{
		static constexpr bool value = reg_src_filter<std::tuple_element<I, Sources>::type>::value;
	};

template<std::size_t I>
std::string setRegAndGetsourceAsString(isaTest& test, std::mt19937& rngGen, FArgs& sourceValues) const
{
	decltype(std::get<I>(sourceValues)) value = std::get<I>(sourceValues);
	if constexpr (is_source_from_reg<I, FRegs>::value)
	{
		const opSource& src = std::get<I>(sources);
		regInfo rndReg = src.getRandomRegister(rngGen);
		test.setRegister(rndReg.regName, value);
		return rndReg.regName;
	}
	else
	{
		return std::to_string(value);
	}
}

template<std::size_t... Is>
std::array<std::string, func_arg_count> setRegsAndGetSourcesAsStrings(isaTest& test, std::mt19937& rngGen, FArgs& sourceValues, std::index_sequence<Is...> _) const
{
	return std::array{
		setRegAndGetsourceAsString<Is>(test, rngGen, sourceValues)...
	};
}

constexpr const std::vector<regInfo>& getDestRegs() const
{
	return get_register_from_type(FRetReg::regType);
}

template<reg_type t>
opSource get_source(reg_src<t> src) const
{
	return opSource(get_register_from_type(t), src.range);
}
template<int32_t bit_count, typename T>
opSource get_source(signed_imm<bit_count, T> src) const
{
	return opSource(src.range);
}
template<int32_t bit_count, typename T>
opSource get_source(unsigned_imm<bit_count, T> src) const
{
	return opSource(src.range);
}


std::array<opSource, func_arg_count> createSources(TArgs... instr_bits) const
{
	std::array<opSource, func_arg_count> srcs;
	int32_t index = 0;

	auto add_if_source = [&](auto instr_part) {
		if constexpr (reg_src_filter<decltype(instr_part)>::value)
		{
			srcs[index++] = get_source(instr_part);
		}
	};
	(add_if_source(instr_bits), ...);

	return srcs;
}

template<std::size_t... Is>
FArgs get_random_source_values(std::mt19937& rngGen, std::index_sequence<Is...> _) const
{
	return std::make_tuple(std::get<Is>(sources).getRandom<std::tuple_element<Is, FArgs>::type>(rngGen)...);
}

protected:
	FType opFunc;
	std::array<opSource, func_arg_count> sources;
	std::tuple<TArgs...> instr_bit_parts;

	struct testData
	{
		regInfo destReg;
		FArgs sourceValues;
		std::array<std::string, func_arg_count> instr_sources;

		testData() : destReg(regInfo("", 0)), sourceValues(FArgs()), instr_sources(std::array<std::string, func_arg_count>())
		{ }
	};

	testData setRegistersAndGetInstrData(isaTest& test, std::mt19937& rngGen) const
	{
		testData tData;
		if constexpr (has_explicit_dst_reg)
		{
			tData.destReg = getRandomReg(rngGen, getDestRegs());
		}
		tData.sourceValues = get_random_source_values(rngGen, std::make_index_sequence<func_arg_count>());
		tData.instr_sources = setRegsAndGetSourcesAsStrings(test, rngGen, tData.sourceValues, std::make_index_sequence<func_arg_count>());

		return tData;
	}

public:
	uni_format(std::string name, Pipes pipe, FType opFunc, TArgs...instr_bits) : baseFormat(name, pipe), opFunc(opFunc), sources(createSources(instr_bits...)), instr_bit_parts(std::make_tuple(instr_bits...))
	{}

	void make_single_op_tests(std::string asmfilepath, std::string expfilepath, std::mt19937& rngGen, int32_t testCount) const override
	{
		if constexpr (useMakeTests)
		{
			for (size_t i = 0; i < testCount; i++)
			{
				isaTest test(asmfilepath, expfilepath, instrName + "-" + std::to_string(i));

				testData tData = setRegistersAndGetInstrData(test, rngGen);


				test.addInstr(instrName + " " + tData.destReg.regName + " = " + string_join(tData.instr_sources, ", "));
				test.expectRegisterValue(tData.destReg.regName, std::apply(opFunc, tData.sourceValues));

				test.close();
			}
		}
		else
		{
			throw std::runtime_error("Expected method to be overwritten but it was still called.");
		}
	}

	template<typename T, std::size_t I, int32_t bit_shift> struct get_bit_shift : get_bit_shift<T, I - 1, bit_shift + std::tuple_element<I, T>::type::bit_count> {};
	template<typename T, int32_t bit_shift> struct get_bit_shift<T, 0, bit_shift> { static constexpr int32_t value = bit_shift; };
	template<typename T, std::size_t... Is> 
	struct get_bit_shifts
	{ 
		static constexpr auto value = std::array{ get_bit_shift<T, 0, Is>::value... };

		get_bit_shifts(std::index_sequence<Is...> _) {}
	};

	template<std::size_t I, typename T> struct get_bit_mask { static constexpr int32_t value = (1 << std::tuple_element<I, T>::type::bit_count) - 1; };
	template<typename T, std::size_t... Is>
	struct get_bit_masks
	{
		static constexpr auto value = std::array{ get_bit_mask<Is, T>::value... };

		get_bit_masks(std::index_sequence<Is...> _) {}
	};

	std::vector<int32_t> encode(std::string instr) const override
	{
		return std::vector<int32_t>();
	}
};









class ALUr_format : public uni_format<true, modify_bits<1, 0>, pred_reg, hardcoded_bits<5, 0b01000>, reg_dst<reg_type::GPR>, reg_src<reg_type::GPR>, reg_src<reg_type::GPR>, hardcoded_bits<3, 0b000>, instr_dep_bits<4>>
{
public:
	ALUr_format(std::string name, int32_t func, std::function<int32_t(int32_t, int32_t)> funcOp);
};
class ALUi_format : public uni_format<true, modify_bits<1, 0>, pred_reg, hardcoded_bits<2, 0b00>, instr_dep_bits<3>, reg_dst<reg_type::GPR>, reg_src<reg_type::GPR>, unsigned_imm<12, int32_t>>
{
public:
	ALUi_format(std::string name, int32_t func, std::function<int32_t(int32_t, int32_t)> funcOp);
};
class ALUl_format : public uni_format<true, hardcoded_bits<1, 1>, pred_reg, hardcoded_bits<5, 0b01000>, reg_dst<reg_type::GPR>, reg_src<reg_type::GPR>, unused_bits<5>, hardcoded_bits<3, 0b000>, instr_dep_bits<4>, signed_imm<32, int32_t>>
{
public:
	ALUl_format(std::string name, int32_t func, std::function<int32_t(int32_t, int32_t)> funcOp);
};
class ALUm_format : public uni_format<false, modify_bits<1, 0>, pred_reg, hardcoded_bits<5, 0b01000>, unused_bits<5>, reg_src<reg_type::GPR>, reg_src<reg_type::GPR>, hardcoded_bits<3, 0b010>, instr_dep_bits<4>, implicit_dst_regs<mulRes>>
{
public:
	ALUm_format(std::string name, int32_t func, std::function<mulRes(uint32_t, uint32_t)> funcOp);

	void make_single_op_tests(std::string asmfilepath, std::string expfilepath, std::mt19937& rngGen, int32_t testCount) const override;
};
class ALUc_format : public uni_format<true, modify_bits<1, 0>, pred_reg, hardcoded_bits<5, 0b01000>, unused_bits<2>, reg_dst<reg_type::PR>, reg_src<reg_type::GPR>, reg_src<reg_type::GPR>, hardcoded_bits<3, 0b011>, instr_dep_bits<4>>
{
public:
	ALUc_format(std::string name, int32_t func, std::function<bool(int32_t, int32_t)> funcOp);
};
class ALUci_format : public uni_format<true, modify_bits<1, 0>, pred_reg, hardcoded_bits<5, 0b01000>, unused_bits<2>, reg_dst<reg_type::PR>, reg_src<reg_type::GPR>, unsigned_imm<5, int32_t>, hardcoded_bits<3, 0b110>, instr_dep_bits<4>>
{
public:
	ALUci_format(std::string name, int32_t func, std::function<bool(int32_t, int32_t)> funcOp);
};
class ALUp_format : public uni_format<true, modify_bits<1, 0>, pred_reg, hardcoded_bits<5, 0b01000>, unused_bits<2>, reg_dst<reg_type::PR>, unused_bits<1>, reg_src<reg_type::PR>, unused_bits<1>, reg_src<reg_type::PR>, hardcoded_bits<3, 0b100>, instr_dep_bits<4>>
{
public:
	ALUp_format(std::string name, int32_t func, std::function<bool(bool, bool)> funcOp);
};







/*
class ALUp_format : public uni_format<bool, true, sourceData<true, bool>, sourceData<true, bool>>
{
public:
	ALUp_format(std::string name, std::function<bool(bool, bool)> func) : uni_format(
		name, Pipes::Both, registers::PRs,
		opSource(registers::PRs, valueRange<int32_t>(0, 1)),
		opSource(registers::PRs, valueRange<int32_t>(0, 1)),
		func)
	{}
};

class ALUb_format : public uni_format<int32_t, true, sourceData<true, int32_t>, sourceData<false, int32_t>, sourceData<true, bool>>
{
public:
	ALUb_format(std::string name, std::function<int32_t(int32_t, int32_t, bool)> func) : uni_format(
		name, Pipes::Both, registers::GPRs,
		gprWholeIntRange,
		opSource(valueRange<int32_t>(0, (1 << 5) - 1)),
		opSource(registers::PRs, valueRange<int32_t>(0, 1)),
		func)
	{}
};

class SPCt_format : public uni_format<int32_t, true, sourceData<true, int32_t>>
{
public:
	SPCt_format(std::string name) : uni_format(
		name, Pipes::Both, registers::SPRs,
		gprWholeIntRange,
		[](int32_t a) { return a; })
	{}
};

class SPCf_format : public uni_format<int32_t, true, sourceData<true, int32_t>>
{
public:
	SPCf_format(std::string name) : uni_format(
		name, Pipes::Both, registers::GPRs,
		opSource(registers::SPRs, wholeIntRange),
		[](int32_t a) { return a; })
	{}
};



template<typename FRet, bool useMakeTests, typename... TArgs>
class uni_float_format : public uni_format<FRet, false, TArgs...>
{
private:
	const std::vector<rounding::x86_round_mode>& rounding_modes;

public:
	uni_float_format(std::string name, const std::vector<regInfo>& destRegs, typename TArgs::srcType... srcs, std::function<FRet(typename TArgs::regValueT...)> func, const std::vector<rounding::x86_round_mode>& rounding_modes) :
		uni_format(name, Pipes::First, destRegs, srcs..., func), rounding_modes(rounding_modes)
	{}

	void makeTests(std::string asmfilepath, std::string expfilepath, std::mt19937& rngGen, int32_t testCount) const override
	{
		if constexpr (useMakeTests)
		{
			for each (auto x86_round in rounding_modes)
			{
				rounding::patmos_round_mode patmos_round = rounding::x86_to_patmos_round(x86_round);

				for (size_t i = 0; i < testCount; i++)
				{
					isaTest test(asmfilepath, expfilepath, instrName + "-" + rounding::patmos_round_to_string(patmos_round) + "-" + std::to_string(i));

					testData tData = setRegistersAndGetInstrData(test, rngGen);

					test.addInstr("mts sfcsr = " + std::to_string(static_cast<int32_t>(patmos_round) << 5) + " # clear exceptions and set rounding mode to " + rounding::patmos_round_to_string(patmos_round));
					test.addInstr(tData.instr);

					std::fenv_t curr_fenv;
					if (fegetenv(&curr_fenv) != 0)
					{
						std::runtime_error("Failed to store the current floating-point environment.");
					}
					if (fesetround((int32_t)x86_round) != 0)
					{
						std::runtime_error("Failed to set the floating-point rounding mode.");
					}
					if (feclearexcept(FE_ALL_EXCEPT) != 0)
					{
						std::runtime_error("Failed to clear the floating-point exceptions.");
					}

					FRet result = std::apply(func, tData.sourceValues);
					int32_t exceptions = fetestexcept(FE_ALL_EXCEPT);

					if (fesetenv(&curr_fenv) != 0)
					{
						std::runtime_error("Failed to restore the floating-point environment.");
					}

					test.expectRegisterValue(tData.destReg.regName, result);
					test.expectRegisterValue("sfcsr", exceptions);

					test.close();
				}
			}
		}
		else
		{
			throw std::runtime_error("Expected method to be overwritten but it was still called.");
		}
	}
};



static const std::vector<rounding::x86_round_mode> x86_and_patmos_compat_rounding_modes = {
	rounding::x86_round_mode::RNE,
	rounding::x86_round_mode::RTZ,
	rounding::x86_round_mode::RDN,
	rounding::x86_round_mode::RUP,
};
static valueRange<float> floatRange1000(-1000.0f, 1000.0f);
static opSource fprWholeFloatRange(registers::FPRs, floatRange1000);
static const std::vector<std::function<void(isaTest&)>> no_special_tests;



class FPUr_format : public uni_float_format<float, true, sourceData<true, float>, sourceData<true, float>>
{
public:
	FPUr_format(std::string name, std::function<float(float, float)> func) : uni_float_format(
		name, registers::FPRs,
		fprWholeFloatRange,
		fprWholeFloatRange,
		func,
		x86_and_patmos_compat_rounding_modes)
	{}
};

class FPUl_format : public uni_float_format<float, true, sourceData<true, float>, sourceData<false, float>>
{
public:
	FPUl_format(std::string name, std::function<float(float, float)> func) : uni_float_format(
		name, registers::FPRs,
		fprWholeFloatRange,
		opSource(floatRange1000),
		func,
		x86_and_patmos_compat_rounding_modes)
	{}
};

class FPUrs_format : public uni_float_format<float, true, sourceData<true, float>>
{
public:
	FPUrs_format(std::string name, std::function<float(float)> func) : uni_float_format(
		name, registers::FPRs,
		fprWholeFloatRange,
		func,
		x86_and_patmos_compat_rounding_modes)
	{}
};

class FPCt_format : public uni_float_format<float, true, sourceData<true, int32_t>>
{
public:
	FPCt_format(std::string name, std::function<float(int32_t)> func) : uni_float_format(
		name, registers::FPRs,
		gprWholeIntRange,
		func,
		x86_and_patmos_compat_rounding_modes)
	{}
};

class FPCf_format : public uni_float_format<int32_t, true, sourceData<true, float>>
{
	const std::vector<std::function<void(isaTest&)>> special_tests;

public:
	FPCf_format(std::string name, std::function<int32_t(float)> func, const std::vector<std::function<void(isaTest&)>>& special_tests = no_special_tests, const std::vector<rounding::x86_round_mode>& rounding_modes = x86_and_patmos_compat_rounding_modes) : uni_float_format(
		name, registers::GPRs,
		fprWholeFloatRange,
		func,
		rounding_modes), special_tests(special_tests)
	{}

	void makeTests(std::string asmfilepath, std::string expfilepath, std::mt19937& rngGen, int32_t testCount) const override
	{
		uni_float_format::makeTests(asmfilepath, expfilepath, rngGen, testCount);

		for (size_t i = 0; i < special_tests.size(); i++)
		{
			isaTest test(asmfilepath, expfilepath, instrName + "-" + std::to_string(i + testCount));
			special_tests[i](test);
		}
	}
};

class FPUc_format : public uni_float_format<bool, true, sourceData<true, float>, sourceData<true, float>>
{
public:
	FPUc_format(std::string name, std::function<bool(float, float)> func) : uni_float_format(
		name, registers::PRs,
		fprWholeFloatRange,
		fprWholeFloatRange,
		func,
		x86_and_patmos_compat_rounding_modes)
	{}
};
*/