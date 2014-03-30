#include <bts/lotto/lotto_config.hpp>
#include <bts/lotto/combination.hpp>
#include <algorithm>
#include <functional>
#include <vector>
#include <iostream>
#include <fc/reflect/variant.hpp>
#include <fc/thread/thread.hpp>
#include <fc/thread/future.hpp>
#include <fc/io/raw.hpp>
#include <fc/io/json.hpp>
#include <fc/log/logger.hpp>

rule_config load_rule_config()
{
   try {
      FC_ASSERT( fc::exists( "dac.json" ) );
      static rule_config config = fc::json::from_file( "dac.json" ).as<rule_config>();

      // TODO: validation and assert

      return config;
   } 
   catch ( const fc::exception& e )
   {
      ilog( "caught exception!: ${e}", ("e", e.to_detail_string()) );
      throw;
   }
}

uint16_t rule_group_count()
{
    rule_config config = load_rule_config();
    static uint16_t group_count = config.balls.size();

    return group_count;
}

namespace bts { namespace lotto {



/* convert cominations numbers to int N, algorithm combinatorial number system
    * 0<=C1<=C2.....
    *
    */
uint32_t ranking(const combination& c)
{
    std::vector<uint16_t> v(c);
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

std::shared_ptr<combination> unranking(uint32_t num, uint16_t k, uint16_t n)
{
	std::vector<uint16_t> c;
    uint16_t max = n;
    uint32_t x = 1;
    for (uint16_t i = k; i >=1; i--)
    {
        x*= i;
    }

    for (uint16_t i = k; i >=1; i--)
    {
        if (num <= 0) {
            c.push_back(i-1);
        } else {
            for (; max >= 1;)
            {
                int32_t temp = 1;
                for (uint16_t m = 1; m <= i; m ++)
                {
                    temp *= (max - m + 1);
                }

                if (num * x >= temp) {
                    c.push_back(max);
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

	std::sort(c.begin(), c.end());

	return std::make_shared<combination>(c);
}
}}	// namespace bts::lotto