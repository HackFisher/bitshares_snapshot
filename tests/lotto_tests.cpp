#define BOOST_TEST_MODULE LottoTests
#include <boost/test/unit_test.hpp>
#include <bts/wallet/wallet.hpp>
#include <bts/blockchain/chain_database.hpp>
#include <bts/blockchain/block_miner.hpp>
#include <bts/blockchain/config.hpp>
#include <bts/lotto/combination.hpp>
#include <fc/filesystem.hpp>
#include <fc/log/logger.hpp>
#include <fc/io/raw.hpp>
#include <fc/reflect/variant.hpp>
#include <fc/thread/thread.hpp>
#include <bitset>

#include <iostream>
using namespace bts::wallet;
using namespace bts::blockchain;
using namespace bts::lotto;

/**
 *  Test utility methods
 */
BOOST_AUTO_TEST_CASE( combination_to_int )
{
	uint16_t ticket_combination[5] = {2, 4, 17, 21, 33};
	std::vector<uint16_t> ticket_v(ticket_combination, ticket_combination + 5);
	std::bitset<35> ticket_bits;
	for (int i = 0; i < 5; i++){
		ticket_bits[ticket_combination[i]] = 1;
	}

	BOOST_CHECK(ticket_v.size() == 5);
	uint32_t ticket_num = combination::combination_to_int(ticket_v);

	// TODO: assert(ticket_num = ??);
	//uint64_t bits_ull = combination::int_to_combination_binary<35>(ticket_num);
	std::vector<uint16_t> res_nums = combination::int_to_combination_binary(ticket_num);
	std::bitset<35> res_bits;
	for (int i = 0; i < 5; i++){
		res_bits[res_nums[i]] = 1;
	}

	std::cout << "the inner representing number of ticket is: " << res_bits.to_ullong();

	BOOST_CHECK(ticket_bits == res_bits);

	uint16_t winning_combination[5] = {3, 4, 9, 17, 22};
	std::bitset<35> winning_bits;
	for (int i = 0; i < 5; i++){
		winning_bits[winning_combination[i]] = 1;
	}

	// match ticket number with winning number
	std::bitset<35> prize_bits = res_bits & winning_bits;

	// only two numbers are matched
	BOOST_CHECK(prize_bits.count() == 2);
}