#include <bts/lotto/lotto_config.hpp>
#include <bts/lotto/combination.hpp>

namespace bts { namespace lotto {
	combination::combination()
		: red(0), blue(0)
	{
	}
	combination::combination(uint32_t r, uint32_t b)
		: red(r), blue(b)
	{
	}
	combination::combination(const std::vector<uint16_t>& nums)
	{
		// TODO: call static method to convert nums combinations to N
		// using Combinatorial number system 
		// http://en.wikipedia.org/wiki/Combinatorial_number_system
	}
	combination::combination(uint32_t num)
	{
		red = num / BTS_LOTTO_BLUE_COMBINATION_COUNT;
		blue = num % BTS_LOTTO_BLUE_COMBINATION_COUNT;
	}

	uint32_t combination::to_num() const
	{
		return red * BTS_LOTTO_BLUE_COMBINATION_COUNT + blue;
	}

	uint32_t combination::combination_to_int(const std::vector<uint16_t>& nums)
	{
		// sorting by decrease
		uint32_t n = 0;
		// TODO: convert cominations numbers to int N, algorithm combinatorial number system
		return n;
	}

	std::vector<uint16_t> combination::int_to_combination_binary(uint32_t num)
	{
		// step1: revert of combination_to_int, converting to vector of nums
		// representing this nums using 2-bit form, e.g. 00001110000000
		std::vector<uint16_t> combine_nums(5);
		combine_nums.push_back(5);
		combine_nums.push_back(4);

		return combine_nums;
	}
}}	// namespace bts::lotto