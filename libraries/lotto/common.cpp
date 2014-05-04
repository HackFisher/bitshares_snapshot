#include <bts/lotto/lotto_config.hpp>
#include <bts/lotto/common.hpp>
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

namespace bts { namespace lotto {

const lotto_rule::config& global_rule_config()
{
    static lotto_rule::config config;
    if (config.valid) return config;

    try {
        FC_ASSERT(fc::exists("lotto_rule.json"));
        config = fc::json::from_file("lotto_rule.json").as<lotto_rule::config>();

        // TODO: validation and assert
        config.valid = true;
        return config;
    }
    catch (const fc::exception& e)
    {
        ilog("caught exception!: ${e}", ("e", e.to_detail_string()));
        throw;
    }
}

uint8_t GROUP_COUNT()
{
    const lotto_rule::config& config = global_rule_config();
    if (!config.valid) return 0;

    return config.ball_group.size();
}

const std::vector<uint64_t>& GROUP_SPACES()
{
    static std::vector<uint64_t> spaces;
    if (spaces.size() > 0)	{
        FC_ASSERT(spaces.size() == GROUP_COUNT());

        return spaces;
    }

    // init group spaces, C(N,k)
    const lotto_rule::config& config = global_rule_config();
    uint8_t group_count = GROUP_COUNT();
    for (int i = 0; i < group_count; i++)
    {
        uint8_t N = config.ball_group[i].total_count;
        uint8_t k = config.ball_group[i].combination_count;
        spaces.push_back(bts::lotto::Combination(N, k));
    }

    return spaces;
}

uint64_t TOTAL_SPACE()
{
    static uint64_t total = 0;

    if (total > 0) return total;

    total = 1;
    const std::vector<uint64_t>& spaces = GROUP_SPACES();

    for (size_t i = 0; i < spaces.size(); i++)
    {
        FC_ASSERT(spaces[i] >= 0);
        total *= spaces[i];
    }

    return total;
}

uint64_t Combination(uint16_t N, uint16_t k)
{
    static uint64_t C[BTS_LOTTO_MAX_BALL_COUNT][BTS_LOTTO_MAX_BALL_COUNT / 2];
	static bool inited = false;

	if (!inited) {
		int i,j;
        for (j = 0; j < (BTS_LOTTO_MAX_BALL_COUNT / 2); ++j)
		{
			C[0][j] = 0;
		}
        for (i = 0; i < BTS_LOTTO_MAX_BALL_COUNT; ++i)
		{
			C[i][0] = 1;
		}

        for (i = 1; i < BTS_LOTTO_MAX_BALL_COUNT; ++i)
		{
            for (j = 1; j < (BTS_LOTTO_MAX_BALL_COUNT / 2); ++j)
			{
				C[i][j] = C[i-1][j] + C[i-1][j-1];
			}
		}
		inited = true;
	}

    if (N < k) return 0;
    if ((N - k) < k) k = N - k;

    FC_ASSERT(N < BTS_LOTTO_MAX_BALL_COUNT, 
        "N should less than maximum ball count.", ("N", N)("MAX", BTS_LOTTO_MAX_BALL_COUNT));
    FC_ASSERT(k < BTS_LOTTO_MAX_BALL_COUNT / 2, "k should less than half of maximum ball count.", ("k", k)("MAX", BTS_LOTTO_MAX_BALL_COUNT));
	
	return C[N][k];
}

group_match match_rankings(const c_rankings& l, const c_rankings& r, const type_ball_group& balls)
{
	FC_ASSERT(l.size() == r.size());
	FC_ASSERT(l.size() == balls.size());
    FC_ASSERT(balls.size() < BTS_LOTTO_MAX_BALL_COUNT);
	
    group_match m;

	for(size_t i = 0; i < l.size(); i ++)
	{
		combination left_combinatioin = unranking(l[i], balls[i].combination_count, balls[i].total_count);
        combination right_combinatioin = unranking(r[i], balls[i].combination_count, balls[i].total_count);

        std::bitset<BTS_LOTTO_MAX_BALL_COUNT> left_bits, right_bits;
		for (size_t i = 0; i < balls.size(); i++){
			left_bits[left_combinatioin[i]] = 1;
			right_bits[right_combinatioin[i]] = 1;
		}

		m.push_back( (left_bits & right_bits).count() );
	}

	FC_ASSERT(balls.size() == m.size());

    return m;
}

uint64_t ranking(const c_rankings& r, const std::vector<uint64_t>& spaces )
{
	FC_ASSERT( r.size() == spaces.size() );
	uint64_t res = 0;
	for (uint16_t i = 0; i < spaces.size(); i ++ )
	{
		FC_ASSERT( r[i] < spaces[i] );
		if (i == 0)
		{
			res += r[i];
		} else {
			res = res * spaces[i] + r[i];
		}
	}

	return res;
}

// unranking to combination rankings
// TODO: making return value const?
c_rankings unranking(uint64_t num, const std::vector<uint64_t>& spaces )
{
	c_rankings rs;
	for (int i = spaces.size() - 1; i >= 0; i --) {
		rs.push_back(num % spaces[i]);
		num = num / spaces[i];
	}

	std::reverse(rs.begin(), rs.end());
    return rs;
}

uint64_t ranking(const combination& c)
{
    std::vector<uint16_t> v(c);
    std::sort(v.begin(), v.end());
	uint64_t n = 0;
    // sum of C(v[k - 1], k)
    for (size_t i = 1; i <= v.size(); i ++) {
		n += Combination(v[i - 1], i);
    }
	return n;
}

combination unranking(uint64_t num, uint16_t k, uint16_t n)
{
    std::vector<uint16_t> c;
    uint16_t max = n;

    for (uint16_t i = k; i >= 1; i--)
    {
        if (num <= 0) {
            c.push_back(i-1);
        } else {
            for (; max >= 1;)
            {
				uint64_t c_max_i = Combination(max, i);
                if (num >= c_max_i) {
                    c.push_back(max);

                    num -=  c_max_i;
                    max --;
                    break;
                } else {
                    max --;
                }
            }
        }
    }

	std::sort(c.begin(), c.end());

    return c;
}
}}	// namespace bts::lotto