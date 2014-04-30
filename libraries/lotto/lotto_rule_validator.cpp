#include <bts/lotto/lotto_outputs.hpp>
#include <fc/reflect/variant.hpp>
#include <bts/lotto/lotto_db.hpp>
#include <bts/lotto/lotto_rule_validator.hpp>
#include <bts/lotto/rule.hpp>
#include <bts/lotto/lotto_config.hpp>

namespace bts { namespace lotto {

	// Default rule validator implement, TODO: may be move default to another class
	rule_validator::rule_validator( lotto_db* db )
	:_db(db)
	{
	}
	rule_validator::~rule_validator()
	{
	}
    
	uint64_t rule_validator::evaluate_total_jackpot(const uint64_t& winning_number, const uint64_t& target_block_num, const uint64_t& available_funds)
    {
        if (target_block_num < BTS_LOTTO_BLOCKS_BEFORE_JACKPOTS_DRAW) {
            return 0;
        }
        
        // make sure that the jackpot_pool is large than the sum ticket sales of blk.block_num - 99 , blk.block_num - 98 ... blk.block_num.
        
        // available funds - all 100 tickets sales / 100 + (block_num - 100)th ticket sale
        // as the total jackpots.
        
        // uint16_t available_pool_prize = summary.ticket_sales - summary.amount_won;
        // auto summary = my->_block2summary.fetch(ticket_block_num);
		return 0;
	}

    uint64_t rule_validator::evaluate_jackpot(const uint64_t& winning_number, const uint64_t& lucky_number, const uint64_t& total_jackpots)
	{
		// This is only one kind of implementation, we call also implement it as dice.
		uint64_t total_space = TOTAL_SPACE();
		uint64_t rule_winning_number = winning_number % total_space;
		uint64_t rule_lucky_number = lucky_number % total_space;
		c_rankings winning_rs = unranking(rule_winning_number, GROUP_SPACES());
		c_rankings lucky_rs = unranking(rule_lucky_number, GROUP_SPACES());
		match m = match_rankings(winning_rs, lucky_rs, global_rule_config().balls);

		const type_prizes& prizes = global_rule_config().prizes;

		// get prize_level, TODO: extract/refactor as utility method
		uint8_t level;
		for (size_t i = 0; i < prizes.size(); i ++)
		{
			const std::vector<match>& matches = prizes[i].second;
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
				level = prizes[i].first;
			}
		}

		// TODO switch case... level to find jackpots
		uint16_t jackpot = 1;

		// ...

        // 3. jackpot should not be calculated here, 
		/*
		fc::sha256::encoder enc;
		enc.write( (char*)&lucky_number, sizeof(lucky_number) );
		enc.write( (char*)&winning_number, sizeof(winning_number) );
		enc.result();
		fc::bigint  result_bigint( enc.result() );

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

