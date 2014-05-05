#include <bts/lotto/lotto_outputs.hpp>
#include <bts/lotto/lotto_db.hpp>
#include <bts/lotto/lotto_rule.hpp>
#include <bts/lotto/lotto_config.hpp>

#include <fc/reflect/variant.hpp>
#include <fc/io/raw.hpp>
#include <fc/io/json.hpp>
#include <fc/log/logger.hpp>

#include <algorithm>
#include <bitset>
#include <functional>
#include <vector>
#include <iostream>

namespace bts { namespace lotto {
    namespace detail
    {
        class lotto_rule_impl
        {
            public:
                lotto_rule_impl(){}

                lotto_db* _db;
        };
    }

    namespace helper
    {
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

        uint64_t lucky_number_from_variants(const fc::variants& params)
        {
            uint16_t group_count = lotto_rule::config_instance().group_count();
            FC_ASSERT(group_count > 0, "group size is 0.");
            size_t expected_param_size = 0;
            for (size_t k = 0; k < group_count; k ++)
            {
                if ( k != 0)
                {
                    expected_param_size ++;
                }
                
                expected_param_size += lotto_rule::config_instance().ball_group[k].combination_count;
            }

            FC_ASSERT(params.size() >= expected_param_size, "No enough parameters.", ("size", params.size()));

            c_rankings c_r;
            size_t i = 0;
            for (size_t k = 0; k < group_count; k ++)
            {
                if ( k != 0)
                {
                    std::string seperator = params[i].as_string();
                    FC_ASSERT(seperator == "|", "The seperator is not right, should be '|' .", ("seperator", seperator));
                    i ++;
                }
            
                combination combination;
                uint16_t ball_select_count = lotto_rule::config_instance().ball_group[k].combination_count;
                uint16_t ball_count = lotto_rule::config_instance().ball_group[k].total_count;
            
                for (size_t i = 0; i < ball_select_count; i ++)
                {
                    uint16_t    number = (uint16_t)params[i].as_uint64();
                
                    FC_ASSERT(number > 0 && number <= ball_count);
                    i ++;
                
                    combination.push_back(number - 1);
                }
            
                // assert unique of number
                std::sort(combination.begin(), combination.end());
                for ( size_t j = 1; j < combination.size(); j ++ )
                {
                    FC_ASSERT(combination[j - 1] != combination[j], "There are repeating numbers.");
                }
            
                uint64_t r = ranking(combination);
                c_r.push_back(r);
            }
        
            uint64_t lucky_number = ranking(c_r, lotto_rule::group_spaces());

            return lucky_number;
        }
    }

	// Default rule validator implement, TODO: may be move default to another class
    lotto_rule::lotto_rule(lotto_db* db)
        :my(new detail::lotto_rule_impl())
	{
        my->_db = db;
	}
    lotto_rule::~lotto_rule()
	{
	}

    void lotto_rule::config::print_rule() const
    {
        std::cout << "------Start of Rule " << name << "-----\n";
        std::cout << "There are " << ball_group.size() << " groups of balls:" << "\n";
        
        for (size_t i = 0; i < ball_group.size(); i++)
        {
            std::cout << "\n";
            std::cout << "For the " << i << "th group of ball:" << "\n";
            std::cout << "There are " << (uint16_t)ball_group[i].total_count << " balls with different numbers in total" << "\n";
            std::cout << "Player should choose " << (uint16_t)ball_group[i].combination_count 
                << " balls out of " << (uint16_t)ball_group[i].total_count << " total balls" << "\n";
        }

        std::cout << "\n";
        std::cout << "The following are the definition of the prizes:\n";
        for (size_t i = 0; i < prizes.size(); i ++)
        {
            std::cout << "\n";
            std::cout << "The " << prizes[i].level << " level prize's definition is:\n" ;
            //std::count << pz[i].desc;
            std::cout << "Match count for each group: [";
            for (size_t j = 0; j < prizes[i].match_list.size(); j ++ )
            {
                if( j != 0)
                {
                    std::cout << " OR ";
                }
                std::cout << "( ";
                for (size_t k = 0; k < prizes[i].match_list[j].size(); k++)
                {
                    if (k != 0)
                    {
                        std::cout << ", ";
                    }
                    std::cout << "" << (uint16_t)prizes[i].match_list[j][k];
                }
                std::cout << " )";
            }
            
            std::cout << "]\n";
        }

        std::cout << "------End of Rule " << name << "-----\n";
    }

    const lotto_rule::config& lotto_rule::config_instance()
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
            //ilog("caught exception!: ${e}", ("e", e.to_detail_string()));
            throw;
        }
    }

    const std::vector<uint64_t>& lotto_rule::group_spaces()
    {
        static std::vector<uint64_t> spaces;
        if (spaces.size() > 0)	{
            FC_ASSERT(spaces.size() == config_instance().group_count());

            return spaces;
        }

        uint8_t group_count = config_instance().group_count();
        for (int i = 0; i < group_count; i++)
        {
            uint8_t N = config_instance().ball_group[i].total_count;
            uint8_t k = config_instance().ball_group[i].combination_count;
            spaces.push_back(helper::Combination(N, k));
        }
               
        return spaces;
    }

    const uint64_t& lotto_rule::total_space()
    {
        static uint64_t total = 0;

        if (total > 0) return total;

        total = 1;
        const std::vector<uint64_t>& spaces = group_spaces();

        for (size_t i = 0; i < spaces.size(); i++)
        {
            FC_ASSERT(spaces[i] >= 0);
            total *= spaces[i];
        }

        return total;
    }
    
    uint64_t lotto_rule::evaluate_total_jackpot(const uint64_t& winning_number, const uint64_t& target_block_num, const uint64_t& available_funds)
    {
        if (target_block_num < BTS_LOTTO_BLOCKS_BEFORE_JACKPOTS_DRAW) {
            return 0;
        }
        
        // TODO:make sure that the jackpot_pool is large than the sum ticket sales of blk.block_num - 99 , blk.block_num - 98 ... blk.block_num.
        
        // available funds - all 100 tickets sales / 100 + (block_num - 100)th ticket sale
        // as the total jackpots.
        
        // uint16_t available_pool_prize = summary.ticket_sales - summary.amount_won;
        // auto summary = my->_block2summary.fetch(ticket_block_num);
		return -1;
	}

    uint64_t lotto_rule::jackpot_for_ticket(const uint64_t& winning_number, 
        const bts::lotto::claim_ticket_output& ticket, const uint64_t& amt, const uint64_t& total_jackpots)
	{
		// This is only one kind of implementation, we call also implement it as dice.
		uint64_t total_space = lotto_rule::total_space();
		uint64_t rule_winning_number = winning_number % total_space;
        uint64_t rule_lucky_number = ticket.lucky_number % total_space;
		c_rankings winning_rs = helper::unranking(rule_winning_number, lotto_rule::group_spaces());
		c_rankings lucky_rs = helper::unranking(rule_lucky_number, lotto_rule::group_spaces());
        group_match m = helper::match_rankings(winning_rs, lucky_rs, config_instance().ball_group);

		const type_prizes& prizes = config_instance().prizes;

		// get prize_level, TODO: extract/refactor as utility method
		uint8_t level;
		for (size_t i = 0; i < prizes.size(); i ++)
		{
			const std::vector<group_match>& matches = prizes[i].match_list;
			bool found = false;
			for (size_t j = 0; j < matches.size(); j ++)
			{
				if (m == matches[j])
				{
					found = true;
					break;
				}
			}
			if (found)
			{
				level = prizes[i].level;
			}
		}

		// TODO switch case... level to find jackpots
        uint16_t jackpot = amt;

        fc::sha256::encoder enc;
		enc.write( (char*)&ticket.lucky_number, sizeof(ticket.lucky_number) );
		enc.write( (char*)&winning_number, sizeof(winning_number) );
		enc.result();
		fc::bigint  result_bigint( enc.result() );


		// ...

        // 3. jackpot should not be calculated here, 
		/*
		

		// the ticket number must be below the winning threshold to claim the jackpot
		auto winning_threshold = result_bigint.to_int64 % fc::bigint( global_odds * odds ).to_int64();
		auto ticket_threshold = amount / odds;
		if (winning_threshold < ticket_threshold)	// we have a winners
		{
			return jackpots;
		}
		*/

		// return 0;

		return jackpot;
	}
}} // bts::lotto

