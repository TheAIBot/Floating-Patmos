#pragma once

#include "common.h"
#include "float-csr.h"
#include "register.h"
#include "tester.h"
#include <string>
#include <array>
#include <tuple>
#include <cstdint>
#include <cfenv>
#include <stdexcept>
#include <functional>
#include <numeric>
#include <filesystem>

namespace patmos
{
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
		
		virtual void make_forwarding_tests(std::string asm_path, std::string exp_path, std::mt19937& rng_gen, int32_t test_count) const = 0;

		//virtual void make_loop_op_tests(std::string asmfilepath, std::string expfilepath, std::mt19937& rngGen, int32_t testCount) const = 0;
		//virtual void make_pipelined_tests(std::string asmfilepath, std::string expfilepath, std::mt19937& rngGen, int32_t testCount) const = 0;

		//virtual std::vector<int32_t> encode(std::string instr) const = 0;
	};

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

		template<typename T>
		T getRandom(std::mt19937& rngGen) const
		{
			static_assert(true, "getRandom does not support ");
			return 0;
		}
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


	template<int32_t bit_c>
	struct bits
	{
		static const int32_t bit_count = bit_c;
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
	template<> struct reg_dst<reg_type::SPR> : bits<4>, explicit_dst_reg
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

	template<typename T, int32_t bit_count> struct op_src : bits<bit_count>
	{
		using type = T;
		valueRange<T> range;
		op_src(valueRange<T> range) : range(range) {}
	};
	template<reg_type t> struct reg_src : bits<0> { };
	template<> struct reg_src<reg_type::GPR> : op_src<int32_t, 5>
	{
		using op_src<int32_t, 5>::op_src;
		reg_src() : op_src(valueRange<int32_t>(0, 0)) {}
	};
	template<> struct reg_src<reg_type::FPR> : op_src<float, 5>
	{
		using op_src<float, 5>::op_src;
		reg_src() : op_src(valueRange<float>(0, 0)) {}
	};
	template<> struct reg_src<reg_type::SPR> : op_src<int32_t, 4>
	{
		using op_src<int32_t, 4>::op_src;
		reg_src() : op_src(valueRange<int32_t>(0, 0)) {}
	};
	template<> struct reg_src<reg_type::PR> : op_src<int32_t, 4>
	{
		using op_src<int32_t, 4>::op_src;
		reg_src() : op_src(valueRange<int32_t>(0, 1)) {}
	};

	template<int32_t bit_count, typename T> struct signed_imm : op_src<T, bit_count>
	{
		using op_src<T, bit_count>::op_src;
		signed_imm() : op_src<T, bit_count>(valueRange<T>(0, 0)) {}
	};
	template<int32_t bit_count, typename T> struct unsigned_imm : op_src<T, bit_count>
	{
		using op_src<T, bit_count>::op_src;
		unsigned_imm() : op_src<T, bit_count>(valueRange<T>(0, 0)) {}
	};

	template<int32_t bit_count> struct unused_bits : bits<bit_count> {};

	template<int32_t bit_count, int32_t bit_value> struct const_bits : bits<bit_count>
	{
		static_assert((((1 << bit_count) - 1)& bit_value) == bit_value, "Value doesn't fit in bits.");
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

	template<bool useMakeTests, typename... TArgs>
	class uni_format : public baseFormat
	{
	protected:
		using FRetReg = typename std::tuple_element<0, decltype(std::tuple_cat(std::conditional_t<reg_dst_filter<TArgs>::value, std::tuple<TArgs>, std::tuple<>>() ...))>::type;
		using FRet = typename FRetReg::type;
		using FArgs = decltype(std::tuple_cat(typename op_src_filter<TArgs>::type() ...));
		using FType = typename make_func_type<FRet, FArgs, std::tuple_size<FArgs>::value>::type;
		using FRegs = decltype(std::tuple_cat(std::conditional_t<op_src_filter<TArgs>::value, std::tuple<TArgs>, std::tuple<>>()...));

		static constexpr int32_t func_arg_count = std::tuple_size<FArgs>::value;
		static constexpr int32_t instr_bit_count = (TArgs::bit_count + ...);
		static constexpr int32_t instr_byte_count = instr_bit_count / 8;
		static constexpr bool has_long_imm = std::is_same<const_bits<1, 1>, typename std::tuple_element<0, std::tuple<TArgs...>>::type>::value;
		static constexpr bool has_explicit_dst_reg = std::is_base_of<explicit_dst_reg, FRetReg>::value;
	private:
		check_instr_size<has_long_imm, instr_bit_count> _;


		template<std::size_t I, typename Sources>
		struct is_source_from_reg
		{
			static constexpr bool value = reg_src_filter<typename std::tuple_element<I, Sources>::type>::value;
		};

		template<std::size_t I>
		std::string setRegAndGetsourceAsString(isaTest& test, std::mt19937& rngGen, FArgs& sourceValues) const
		{
			decltype(std::get<I>(sourceValues))& value = std::get<I>(sourceValues);
			if constexpr (is_source_from_reg<I, FRegs>::value)
			{
				const opSource& src = std::get<I>(sources);
				regInfo rndReg = src.getRandomRegister(rngGen);
				test.setRegister(rndReg.regName, value);
				//if register is readonly then the value from it
				//isn't a random value, but the readonly value
				if (rndReg.isReadonly)
				{
					value = static_cast<typename std::decay<decltype(value)>::type>(rndReg.readonly_value);
				}
				
				if constexpr (std::is_same<reg_src<reg_type::SPR>, typename std::tuple_element<I, FRegs>::type>::value)
				{
					//special register s0 isn't readonly, but the first bit of the 
					//register is, so add the value of that to the value of the register.
					//s0 also only stores the first 8 bits.
					if (rndReg.regName == "s0")
					{
						value = (value & 0xff) | 1;
					}
				}				
				
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
				if constexpr (op_src_filter<decltype(instr_part)>::value)
				{
					srcs[index++] = get_source(instr_part);
				}
			};
			(add_if_source(instr_bits), ...);

			return srcs;
		}

		template<typename T>
		T get_random_source_value(std::mt19937& rngGen, const opSource& src) const
		{
			return src.getRandom<T>(rngGen);
		}

		template<std::size_t... Is>
		FArgs get_random_source_values(std::mt19937& rngGen, std::index_sequence<Is...> _) const
		{
			return std::make_tuple(get_random_source_value<typename std::tuple_element<Is, FArgs>::type>(rngGen, std::get<Is>(sources))...);
		}

		template<std::size_t... Is>
		struct partial_index_sequence {};

		template<std::size_t Start, std::size_t FI, std::size_t... Is>
		struct make_partial_index_sequence : make_partial_index_sequence<Start, Is...>
		{};
		template<std::size_t Start, std::size_t... Is>
		struct make_partial_index_sequence<Start, Start, Is...> : partial_index_sequence<Start, Is...>
		{};

		template<std::size_t Ia, std::size_t Ib>
		void if_equal_replace_source_value(FArgs& sourceValues, std::array<std::string, func_arg_count>& instr_sources) const
		{
			if (instr_sources[Ia] == instr_sources[Ib])
			{
				std::get<Ia>(sourceValues) = std::get<Ib>(sourceValues);
			}
		}

		template<std::size_t I, std::size_t... PIs>
		void handle_dup_reg_source(FArgs& sourceValues, std::array<std::string, func_arg_count>& instr_sources, partial_index_sequence<PIs...> _) const
		{
			if constexpr (is_source_from_reg<I, FRegs>::value)
			{
				(if_equal_replace_source_value<I, PIs>(sourceValues, instr_sources), ...);
			}
		}

		template<std::size_t... Is>
		void handle_dup_reg_sources(FArgs& sourceValues, std::array<std::string, func_arg_count>& instr_sources, std::index_sequence<Is...> _) const
		{
			(handle_dup_reg_source<Is>(sourceValues, instr_sources, make_partial_index_sequence<Is, Is...>()), ...);
		}

		template<typename Dst, typename Src>
		struct is_dst_and_src_reg_type_same { static constexpr bool value = false; };

		template<reg_type t>
		struct is_dst_and_src_reg_type_same<reg_dst<t>, reg_src<t>> { static constexpr bool value = true; };

		template<std::size_t... Is>
		static constexpr bool has_same_src_and_dst_reg_type(std::index_sequence<Is...> _)
		{
			return has_explicit_dst_reg && (is_dst_and_src_reg_type_same<FRetReg, typename std::tuple_element<Is, FRegs>::type>::value || ...);
		}

		template<typename T, std::size_t I>
		void replace_source_with_new_value(std::array<std::string, func_arg_count>& sources, std::string source, FArgs& values, T value) const
		{
			if (sources[I] == source)
			{
				std::get<I>(values) = value;
			}
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

		constexpr const std::vector<regInfo>& getDestRegs() const
		{
			if constexpr (has_explicit_dst_reg)
			{
				return get_register_from_type(FRetReg::regType);
			}
			else
			{
				return std::vector<regInfo>();
			}
		}

		testData setRegistersAndGetInstrData(isaTest& test, std::mt19937& rngGen) const
		{
			testData tData;
			if constexpr (has_explicit_dst_reg)
			{
				tData.destReg = getRandomReg(rngGen, getDestRegs());
			}
			tData.sourceValues = get_random_source_values(rngGen, std::make_index_sequence<func_arg_count>());
			tData.instr_sources = setRegsAndGetSourcesAsStrings(test, rngGen, tData.sourceValues, std::make_index_sequence<func_arg_count>());

			handle_dup_reg_sources(tData.sourceValues, tData.instr_sources, std::make_index_sequence<func_arg_count>());

			return tData;
		}

		void set_expected_value(isaTest& test, regInfo& reg, FRet value) const
		{
			if constexpr (std::is_same<FRetReg, reg_dst<reg_type::SPR>>::value)
			{
				//Only possible to write to the first 8 bits of s0
				//and the first bit is always set
				if (reg.regName == "s0")
				{
					test.expectRegisterValue(reg.regName, (value & 0xff) | 1);
					return;
				}
			}

			if (reg.isReadonly)
			{
				test.expectRegisterValue(reg.regName, (FRet)reg.readonly_value);
			}
			else
			{
				test.expectRegisterValue(reg.regName, value);
			}
		}

		template<typename T, std::size_t... Is>
		void replace_sources_with_new_value(std::array<std::string, func_arg_count>& sources, std::string source, FArgs& values, T value, std::index_sequence<Is...> _) const
		{
			(replace_source_with_new_value<T, Is>(sources, source, values, value), ...);
		}

		virtual void make_internal_forwarding_test(std::string asm_path, std::string exp_path, std::mt19937& rng_gen, int32_t test_count, std::size_t op_index) const
		{
			if constexpr (useMakeTests)
			{
				std::string asm_dir = create_dir_if_not_exists(asm_path, "dst-op" + std::to_string(op_index));
				std::string uart_dir = create_dir_if_not_exists(exp_path, "dst-op" + std::to_string(op_index));

				const int32_t pipeline_stages = 5;
				for (size_t i = 0; i < test_count; i++)
				{
					isaTest test(asm_dir, uart_dir, instrName + "-" + std::to_string(i));
					testData tData = setRegistersAndGetInstrData(test, rng_gen);

					FRet result = 0;
					for (size_t z = 0; z < pipeline_stages; z++)
					{
						tData.destReg = getRandomReg(rng_gen, getDestRegs());
						test.addInstr(instrName + " " + tData.destReg.regName + " = " + string_join(tData.instr_sources, ", "), instr_byte_count);
						tData.instr_sources[op_index] = tData.destReg.regName;

						result = tData.destReg.isReadonly ? static_cast<FRet>(tData.destReg.readonly_value) : std::apply(opFunc, tData.sourceValues);
						replace_sources_with_new_value(tData.instr_sources, tData.destReg.regName, tData.sourceValues, result, std::make_index_sequence<func_arg_count>());
					}
					
					set_expected_value(test, tData.destReg, result);

					test.close();
				}
			}
		}

		template<std::size_t I>
		void try_make_internal_forwarding_test(std::string asm_path, std::string exp_path, std::mt19937& rng_gen, int32_t test_count) const
		{
			if constexpr (is_dst_and_src_reg_type_same<FRetReg, typename std::tuple_element<I, FRegs>::type>::value)
			{
				make_internal_forwarding_test(asm_path, exp_path, rng_gen, test_count, I);
			}
		}

		template<std::size_t... Is>
		void make_internal_forwarding_tests(std::string asm_path, std::string exp_path, std::mt19937& rng_gen, int32_t test_count, std::index_sequence<Is...> _) const
		{
			(try_make_internal_forwarding_test<Is>(asm_path, exp_path, rng_gen, test_count), ...);
		}

		

	public:
		uni_format(std::string name, Pipes pipe, FType opFunc, TArgs...instr_bits) : baseFormat(name, pipe), opFunc(opFunc), sources(createSources(instr_bits...)), instr_bit_parts(std::make_tuple(instr_bits...))
		{}

		void make_single_op_tests(std::string asmfilepath, std::string expfilepath, std::mt19937& rngGen, int32_t testCount) const override
		{
			if constexpr (useMakeTests)
			{
				std::string asm_dir = create_dir_if_not_exists(asmfilepath, instrName);
				std::string uart_dir = create_dir_if_not_exists(expfilepath, instrName);

				for (size_t i = 0; i < testCount; i++)
				{
					isaTest test(asm_dir, uart_dir, instrName + "-" + std::to_string(i));
					testData tData = setRegistersAndGetInstrData(test, rngGen);

					test.addInstr(instrName + " " + tData.destReg.regName + " = " + string_join(tData.instr_sources, ", "), instr_byte_count);
					set_expected_value(test, tData.destReg, std::apply(opFunc, tData.sourceValues));

					test.close();
				}
			}
			else
			{
				throw std::runtime_error("Expected method to be overwritten but it was still called.");
			}
		}

		void make_forwarding_tests(std::string asm_path, std::string exp_path, std::mt19937& rng_gen, int32_t test_count) const override
		{
			if constexpr (has_same_src_and_dst_reg_type(std::make_index_sequence<func_arg_count>()))
			{
				std::string asm_dir = create_dir_if_not_exists(asm_path, instrName);
				std::string uart_dir = create_dir_if_not_exists(exp_path, instrName);

				make_internal_forwarding_tests(asm_dir, uart_dir, rng_gen, test_count, std::make_index_sequence<func_arg_count>());
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

		//std::vector<int32_t> encode(std::string instr) const override
		//{
		//	return std::vector<int32_t>();
		//}
	};

	template<bool useMakeTests, typename... TArgs>
	class uni_float_format : public uni_format<false, TArgs...>
	{
	private:
		const std::vector<rounding::x86_round_mode>& rounding_modes;
		using FRet = typename uni_format<false, TArgs...>::FRet;
		using FRetReg = typename uni_format<false, TArgs...>::FRetReg;
		using FArgs = typename uni_format<false, TArgs...>::FArgs;
		using FType = typename uni_format<false, TArgs...>::FType;
		using uni_f = uni_format<false, TArgs...>;

		struct fp_result
		{
			FRet result;
			int32_t exceptions;
		};

		fp_result get_fp_result(rounding::x86_round_mode x86_round, FArgs& source_values) const
		{
			fp_result fp_res;

			std::fenv_t curr_fenv;
			if (fegetenv(&curr_fenv) != 0)
			{
				throw std::runtime_error("Failed to store the current floating-point environment.");
			}
			if (fesetround((int32_t)x86_round) != 0)
			{
				throw std::runtime_error("Failed to set the floating-point rounding mode.");
			}
			if (feclearexcept(FE_ALL_EXCEPT) != 0)
			{
				throw std::runtime_error("Failed to clear floating-point exceptions.");
			}

			fp_res.result = std::apply(this->opFunc, source_values);

			std::fexcept_t x86_exceptions;
			if (fegetexceptflag(&x86_exceptions, FE_ALL_EXCEPT) != 0)
			{
				throw std::runtime_error("Failed to get floating-point exceptions.");
			}
			int32_t patmos_exceptions = exception::x86_to_patmos_exceptions(x86_exceptions);

			if (fesetenv(&curr_fenv) != 0)
			{
				throw std::runtime_error("Failed to restore the floating-point environment.");
			}

			if constexpr (std::is_same<FRetReg, reg_dst<reg_type::FPR>>::value)
			{
				//nan on x86 is not the same nan as per the RISCV spec
				if (std::isnan(fp_res.result))
				{
					fp_res.result = itf(0x7fc00000);
				}
			}

			fp_res.exceptions = patmos_exceptions;
			return fp_res;
		}

		void make_internal_forwarding_test(std::string asm_path, std::string exp_path, std::mt19937& rng_gen, int32_t test_count, std::size_t op_index) const override
		{
			std::string asm_dir = create_dir_if_not_exists(asm_path, "dst-op" + std::to_string(op_index));
			std::string uart_dir = create_dir_if_not_exists(exp_path, "dst-op" + std::to_string(op_index));

			for (auto x86_round : rounding_modes)
			{
				rounding::patmos_round_mode patmos_round = rounding::x86_to_patmos_round(x86_round);
				std::string patmos_round_str = rounding::patmos_round_to_string(patmos_round);

				std::string asm_rounding_dir = create_dir_if_not_exists(asm_dir, "rounding_" + patmos_round_str);
				std::string uart_rounding_dir = create_dir_if_not_exists(uart_dir, "rounding_" + patmos_round_str);

				const int32_t pipeline_stages = 5;
				for (size_t i = 0; i < test_count; i++)
				{
					isaTest test(asm_rounding_dir, uart_rounding_dir, this->instrName + "-" + std::to_string(i));

					int32_t sfcsr_rounding = static_cast<int32_t>(patmos_round) << 5;
					test.setRegister("sfcsr", sfcsr_rounding);

					typename uni_f::testData tData = this->setRegistersAndGetInstrData(test, rng_gen);

					fp_result accum_result{0};
					for (size_t z = 0; z < pipeline_stages; z++)
					{
						tData.destReg = getRandomReg(rng_gen, this->getDestRegs());
						test.addInstr(this->instrName + " " + tData.destReg.regName + " = " + string_join(tData.instr_sources, ", "), uni_f::instr_byte_count);
						tData.instr_sources[op_index] = tData.destReg.regName;

						fp_result fp_res = get_fp_result(x86_round, tData.sourceValues);

						accum_result.result = tData.destReg.isReadonly ? static_cast<FRet>(tData.destReg.readonly_value) : fp_res.result;
						accum_result.exceptions |= fp_res.exceptions;
						this->replace_sources_with_new_value(tData.instr_sources, tData.destReg.regName, tData.sourceValues, fp_res.result, std::make_index_sequence<uni_f::func_arg_count>());
					}
					
					this->set_expected_value(test, tData.destReg, accum_result.result);
					test.expectRegisterValue("sfcsr", accum_result.exceptions | sfcsr_rounding);

					test.close();
				}
			}
		}

	public:
		uni_float_format(std::string name, const std::vector<rounding::x86_round_mode>& rounding_modes, FType opFunc, TArgs...instr_bits) :
			uni_f(name, Pipes::First, opFunc, instr_bits...), rounding_modes(rounding_modes)
		{}

		void make_single_op_tests(std::string asmfilepath, std::string expfilepath, std::mt19937& rngGen, int32_t testCount) const override
		{
			if constexpr (useMakeTests)
			{
				std::string asm_dir = create_dir_if_not_exists(asmfilepath, this->instrName);
				std::string uart_dir = create_dir_if_not_exists(expfilepath, this->instrName);

				for (auto x86_round : rounding_modes)
				{
					rounding::patmos_round_mode patmos_round = rounding::x86_to_patmos_round(x86_round);
					std::string patmos_round_str = rounding::patmos_round_to_string(patmos_round);

					std::string asm_rounding_dir = create_dir_if_not_exists(asm_dir, "rounding_" + patmos_round_str);
					std::string uart_rounding_dir = create_dir_if_not_exists(uart_dir, "rounding_" + patmos_round_str);

					for (size_t i = 0; i < testCount; i++)
					{
						isaTest test(asm_rounding_dir, uart_rounding_dir, this->instrName + "-" + patmos_round_str + "-" + std::to_string(i));

						int32_t sfcsr_rounding = static_cast<int32_t>(patmos_round) << 5;
						test.setRegister("sfcsr", sfcsr_rounding);

						typename uni_f::testData tData = this->setRegistersAndGetInstrData(test, rngGen);
						test.addInstr(this->instrName + " " + tData.destReg.regName + " = " + string_join(tData.instr_sources, ", "), uni_f::instr_byte_count);

						fp_result fp_res = get_fp_result(x86_round, tData.sourceValues);

						this->set_expected_value(test, tData.destReg, fp_res.result);
						test.expectRegisterValue("sfcsr", fp_res.exceptions | sfcsr_rounding);

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













	class ALUr_format : public uni_format<true, modify_bits<1, 0>, pred_reg, const_bits<5, 0b01000>, reg_dst<reg_type::GPR>, reg_src<reg_type::GPR>, reg_src<reg_type::GPR>, const_bits<3, 0b000>, instr_dep_bits<4>>
	{
	public:
		ALUr_format(std::string name, int32_t func, std::function<int32_t(int32_t, int32_t)> funcOp);
	};
	class ALUi_format : public uni_format<true, modify_bits<1, 0>, pred_reg, const_bits<2, 0b00>, instr_dep_bits<3>, reg_dst<reg_type::GPR>, reg_src<reg_type::GPR>, unsigned_imm<12, int32_t>>
	{
	public:
		ALUi_format(std::string name, int32_t func, std::function<int32_t(int32_t, int32_t)> funcOp);
	};
	class ALUl_format : public uni_format<true, const_bits<1, 1>, pred_reg, const_bits<5, 0b01000>, reg_dst<reg_type::GPR>, reg_src<reg_type::GPR>, unused_bits<5>, const_bits<3, 0b000>, instr_dep_bits<4>, signed_imm<32, int32_t>>
	{
	public:
		ALUl_format(std::string name, int32_t func, std::function<int32_t(int32_t, int32_t)> funcOp);
	};
	class ALUm_format : public uni_format<false, modify_bits<1, 0>, pred_reg, const_bits<5, 0b01000>, unused_bits<5>, reg_src<reg_type::GPR>, reg_src<reg_type::GPR>, const_bits<3, 0b010>, instr_dep_bits<4>, implicit_dst_regs<mulRes>>
	{
	public:
		ALUm_format(std::string name, int32_t func, std::function<mulRes(uint32_t, uint32_t)> funcOp);

		void make_single_op_tests(std::string asmfilepath, std::string expfilepath, std::mt19937& rngGen, int32_t testCount) const override;
	};
	class ALUc_format : public uni_format<true, modify_bits<1, 0>, pred_reg, const_bits<5, 0b01000>, unused_bits<2>, reg_dst<reg_type::PR>, reg_src<reg_type::GPR>, reg_src<reg_type::GPR>, const_bits<3, 0b011>, instr_dep_bits<4>>
	{
	public:
		ALUc_format(std::string name, int32_t func, std::function<bool(int32_t, int32_t)> funcOp);
	};
	class ALUci_format : public uni_format<true, modify_bits<1, 0>, pred_reg, const_bits<5, 0b01000>, unused_bits<2>, reg_dst<reg_type::PR>, reg_src<reg_type::GPR>, unsigned_imm<5, int32_t>, const_bits<3, 0b110>, instr_dep_bits<4>>
	{
	public:
		ALUci_format(std::string name, int32_t func, std::function<bool(int32_t, int32_t)> funcOp);
	};
	class ALUp_format : public uni_format<true, modify_bits<1, 0>, pred_reg, const_bits<5, 0b01000>, unused_bits<2>, reg_dst<reg_type::PR>, unused_bits<1>, reg_src<reg_type::PR>, unused_bits<1>, reg_src<reg_type::PR>, const_bits<3, 0b100>, instr_dep_bits<4>>
	{
	public:
		ALUp_format(std::string name, int32_t func, std::function<bool(bool, bool)> funcOp);
	};
	class ALUb_format : public uni_format<true, modify_bits<1, 0>, pred_reg, const_bits<5, 0b01000>, reg_dst<reg_type::GPR>, reg_src<reg_type::GPR>, unsigned_imm<5, int32_t>, const_bits<3, 0b101>, reg_src<reg_type::PR>>
	{
	public:
		ALUb_format(std::string name, std::function<int32_t(int32_t, int32_t, bool)> funcOp);
	};
	class SPCt_format : public uni_format<true, modify_bits<1, 0>, pred_reg, const_bits<5, 0b01001>, unused_bits<5>, reg_src<reg_type::GPR>, unused_bits<5>, const_bits<3, 0b010>, reg_dst<reg_type::SPR>>
	{
	public:
		SPCt_format(std::string name);
	};
	class SPCf_format : public uni_format<true, modify_bits<1, 0>, pred_reg, const_bits<5, 0b01001>, reg_dst<reg_type::GPR>, unused_bits<10>, const_bits<3, 0b011>, reg_src<reg_type::SPR>>
	{
	public:
		SPCf_format(std::string name);
	};






	static const std::vector<rounding::x86_round_mode> x86_and_patmos_compat_rounding_modes = {
		rounding::x86_round_mode::RNE,
		rounding::x86_round_mode::RTZ,
		rounding::x86_round_mode::RDN,
		rounding::x86_round_mode::RUP,
	};
	static const std::vector<std::function<void(isaTest&)>> no_special_tests;


	class FPUr_format : public uni_float_format<true, modify_bits<1, 0>, pred_reg, const_bits<5, 0b01101>, reg_dst<reg_type::FPR>, reg_src<reg_type::FPR>, reg_src<reg_type::FPR>, const_bits<3, 0b000>, instr_dep_bits<4>>
	{
	public:
		FPUr_format(std::string name, int32_t func, std::function<float(float, float)> funcOp);
	};
	class FPUl_format : public uni_float_format<true, const_bits<1, 1>, pred_reg, const_bits<5, 0b01101>, reg_dst<reg_type::FPR>, reg_src<reg_type::FPR>, unused_bits<5>, const_bits<3, 0b010>, instr_dep_bits<4>, signed_imm<32, float>>
	{
	public:
		FPUl_format(std::string name, int32_t func, std::function<float(float, float)> funcOp);
	};
	class FPUrs_format : public uni_float_format<true, modify_bits<1, 0>, pred_reg, const_bits<5, 0b01101>, reg_dst<reg_type::FPR>, reg_src<reg_type::FPR>, unused_bits<5>, const_bits<3, 0b011>, instr_dep_bits<4>>
	{
	public:
		FPUrs_format(std::string name, int32_t func, std::function<float(float)> funcOp);
	};
	class FPUc_format : public uni_float_format<true, modify_bits<1, 0>, pred_reg, const_bits<5, 0b01101>, unused_bits<2>, reg_dst<reg_type::PR>, reg_src<reg_type::FPR>, reg_src<reg_type::FPR>, const_bits<3, 0b001>, instr_dep_bits<4>>
	{
	public:
		FPUc_format(std::string name, int32_t func, std::function<bool(float, float)> funcOp);
	};
	class FPCt_format : public uni_float_format<true, modify_bits<1, 0>, pred_reg, const_bits<5, 0b01110>, reg_dst<reg_type::FPR>, reg_src<reg_type::GPR>, unused_bits<5>, const_bits<3, 0b000>, instr_dep_bits<4>>
	{
	public:
		FPCt_format(std::string name, int32_t func, std::function<float(int32_t)> funcOp);
	};
	class FPCf_format : public uni_float_format<true, modify_bits<1, 0>, pred_reg, const_bits<5, 0b01110>, reg_dst<reg_type::GPR>, reg_src<reg_type::FPR>, unused_bits<5>, const_bits<3, 0b001>, instr_dep_bits<4>>
	{
		const std::vector<std::function<void(isaTest&)>> special_tests;

	public:
		FPCf_format(std::string name, int32_t func, std::function<int32_t(float)> funcOp, const std::vector<std::function<void(isaTest&)>>& special_tests = no_special_tests, const std::vector<rounding::x86_round_mode>& rounding_modes = x86_and_patmos_compat_rounding_modes);

		void make_single_op_tests(std::string asmfilepath, std::string expfilepath, std::mt19937& rngGen, int32_t testCount) const override;
	};
}