#include <bts/lotto/lotto_config.hpp>
#include <bts/lotto/rule.hpp>
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

const rule_config& global_rule_config()
{
	static rule_config config;
	if (config.valid) return config;
	
	try {
		FC_ASSERT( fc::exists( "rule.json" ) );
		config = fc::json::from_file( "rule.json" ).as<rule_config>();

		// TODO: validation and assert
		config.valid = true;
		return config;
	} 
	catch ( const fc::exception& e )
	{
		ilog( "caught exception!: ${e}", ("e", e.to_detail_string()) );
		throw;
	}
}

uint8_t GROUP_COUNT()
{
    const rule_config& config = global_rule_config();
	if (! config.valid) return 0;

    return config.balls.size();
}

const std::vector<uint64_t>& GROUP_SPACES()
{
	static std::vector<uint64_t> spaces;
	if (spaces.size() > 0)	{
		FC_ASSERT(spaces.size() == GROUP_COUNT());

		return spaces;
	}

	// init group spaces, C(N,k)
	const rule_config& config = global_rule_config();
	uint8_t group_count = GROUP_COUNT();
	for (int i = 0; i < group_count; i ++)
	{
		uint8_t N = config.balls[i].first;
		uint8_t k = config.balls[i].second;
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

	for(size_t i = 0; i < spaces.size(); i ++)
	{
		FC_ASSERT(spaces[i] >= 0);
		total *= spaces[i];
	}

	return total;
}

namespace bts { namespace lotto {

uint64_t Combination(uint8_t N, uint8_t k)
{
	static uint64_t C[256][128];
	static bool inited = false;

	if (!inited) {
		int i,j;
		for(j = 0; j < 128; ++j)
		{
			C[0][j] = 0;
		}
		for(i = 0; i < 256; ++i)
		{
			C[i][0] = 1;
		}

		for(i = 1; i < 256; ++i)
		{
			for(j = 1; j < 128; ++j)
			{
				C[i][j] = C[i-1][j] + C[i-1][j-1];
			}
		}
		inited = true;
	}

	if (N < k) return 0;
	if ((N - k) < k) k = N -k;
	
	return C[N][k];
}

match match_rankings(const c_rankings& l, const c_rankings& r, const type_balls& balls)
{
	FC_ASSERT(l.size() == r.size());
	FC_ASSERT(l.size() == balls.size());
	
	match m;

	for(size_t i = 0; i < l.size(); i ++)
	{
		std::shared_ptr<combination> left_combinatioin = unranking(l[i], balls[i].second, balls[i].first);
		std::shared_ptr<combination> right_combinatioin = unranking(r[i], balls[i].second, balls[i].first);

		std::bitset<256> left_bits, right_bits;	// TODO: is size of 256 too big?
		for (size_t i = 0; i < balls.size(); i++){
			left_bits[left_combinatioin->at(i)] = 1;
			right_bits[right_combinatioin->at(i)] = 1;
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
std::shared_ptr<c_rankings> unranking(uint64_t num, const std::vector<uint64_t>& spaces )
{
	c_rankings rs;
	for (int i = spaces.size() - 1; i >= 0; i --) {
		rs.push_back(num % spaces[i]);
		num = num / spaces[i];
	}

	std::reverse(rs.begin(), rs.end());
	return std::make_shared<c_rankings>(rs);
}


/* convert cominations numbers to int N, algorithm combinatorial number system
    * 0<=C1<=C2.....
    *
    */
uint64_t ranking(const combination& c)
{
    std::vector<uint8_t> v(c);
    std::sort(v.begin(), v.end());
	uint64_t n = 0;
    // sum of C(v[k - 1], k)
    for (uint32_t i = 1; i <= v.size(); i ++) {
		n += Combination(v[i - 1], i);
    }
	return n;
}

std::shared_ptr<combination> unranking(uint64_t num, uint8_t k, uint8_t n)
{
	std::vector<uint8_t> c;
    uint8_t max = n;

    for (uint8_t i = k; i >=1; i--)
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

	return std::make_shared<combination>(c);
}
}}	// namespace bts::lotto