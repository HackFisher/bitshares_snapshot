#pragma once
#include <bts/lotto/lotto_outputs.hpp>
#include <fc/reflect/variant.hpp>
#include <bts/lotto/lotto_db.hpp>
#include <bts/lotto/lotto_rule_validator.hpp>
#include <bts/lotto/rule.hpp>

namespace bts { namespace lotto {

	// Default rule validator implement, TODO: may be move default to another class
	rule_validator::rule_validator( lotto_db* db )
	:_db(db)
	{
	}
	rule_validator::~rule_validator()
	{
	}
    
	uint64_t rule_validator::evaluate_total_jackpot(uint64_t winning_number, uint64_t target_block_num){
		return 0;
	}

    uint64_t rule_validator::evaluate_jackpot(uint64_t winning_number, uint64_t lucky_number, uint16_t amount)
	{
		// This is only one kind of implementation, we call also implement it as dice.
		uint64_t total_space = TOTAL_SPACE();
		winning_number = winning_number % total_space;
		lucky_number = lucky_number % total_space;
		std::shared_ptr<c_rankings> winning_rs = unranking(winning_number, GROUP_SPACES());
		std::shared_ptr<c_rankings> lucky_rs = unranking(winning_number, GROUP_SPACES());
		match m = match_rankings(*winning_rs.get(), *lucky_rs.get(), global_rule_config().balls);

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
		uint16_t jackpot = 0;

		// ...

		return jackpot;
	}
}} // bts::lotto

