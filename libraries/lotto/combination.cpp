#include <bts/lotto/lotto_config.hpp>
#include <bts/lotto/combination.hpp>
#include <algorithm>
#include <functional>
#include <vector>
#include <iostream>

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

    /* convert cominations numbers to int N, algorithm combinatorial number system
     * 0<=C1<=C2.....
     *
     */
	uint32_t combination::ranking(const std::vector<uint16_t>& combination)
	{
        std::vector<uint16_t> v(combination);
        std::sort(v.begin(), v.end());
		uint32_t n = 0;
        // sum of C(v[k - 1], k)
        for (uint32_t i = 1; i <= v.size(); i ++) {
            int32_t a = 1;
            uint32_t b = 1;
            for (uint32_t j = 1; j <= i; j++) {
                a *= v[i - 1] - j + 1;
                b *= j;
            }
            n += (a <= 0 ? 0: a/b);
        }
		return n;
	}

    std::shared_ptr<std::vector<uint16_t>> combination::unranking(uint32_t num, uint16_t k, uint16_t n)
    {
		std::vector<uint16_t> combination;
        uint16_t max = n;
        uint32_t x = 1;
        for (uint16_t i = k; i >=1; i--)
        {
            x*= i;
        }

        for (uint16_t i = k; i >=1; i--)
        {
            if (num <= 0) {
                combination.push_back(i-1);
            } else {
                for (; max >= 1;)
                {
                    int32_t temp = 1;
                    for (uint16_t m = 1; m <= i; m ++)
                    {
                        temp *= (max - m + 1);
                    }

                    if (num * x >= temp) {
                        combination.push_back(max);
                        num -=  temp / x;
                        max --;
                        break;
                    } else {
                        max --;
                    }
                }
            }

            x /= i;
        }

		std::sort(combination.begin(), combination.end());

		return std::make_shared<std::vector<uint16_t>>(combination);
	}
}}	// namespace bts::lotto