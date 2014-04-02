#define BOOST_TEST_MODULE LottoTests
#include <boost/test/unit_test.hpp>
#include <bts/wallet/wallet.hpp>
#include <bts/blockchain/chain_database.hpp>
#include <bts/blockchain/block_miner.hpp>
#include <bts/blockchain/config.hpp>
#include <bts/lotto/rule.hpp>
#include <fc/filesystem.hpp>
#include <fc/log/logger.hpp>
#include <fc/io/raw.hpp>
#include <fc/reflect/variant.hpp>
#include <fc/thread/thread.hpp>
#include <bitset>
#include <boost/shared_ptr.hpp>
#include <boost/make_shared.hpp>

#include <iostream>
using namespace bts::wallet;
using namespace bts::blockchain;
using namespace bts::lotto;

/**
 *  Test utility methods
 */
BOOST_AUTO_TEST_CASE( combination_to_int )
{
	uint8_t ticket_combination[5] = {2, 4, 17, 21, 33};
    combination ticket_v(ticket_combination, ticket_combination + 5);
	std::bitset<35> ticket_bits;
	for (int i = 0; i < 5; i++){
		ticket_bits[ticket_v[i]] = 1;
	}

	BOOST_CHECK(ticket_v.size() == 5);
    uint64_t ticket_num = ranking(ticket_v);

	// TODO: assert(ticket_num = ??);
	//uint64_t bits_ull = combination::int_to_combination_binary<35>(ticket_num);
    auto res_nums = unranking(ticket_num, 5, 35);
	std::bitset<35> res_bits;
	for (int i = 0; i < 5; i++){
		std::cout << (uint16_t)res_nums->at(i) << "\n";
		res_bits[res_nums->at(i)] = 1;
	}

	std::cout << "the rankinging number of ticket is: " << ticket_num << "\n";

    std::cout << "ticket bits is: "<< ticket_bits.to_string() << "\n";
    std::cout << "result bits is: "<< res_bits.to_string() << "\n";

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

BOOST_AUTO_TEST_CASE( test_generate_rule_config )
{
    rule_config config;
    config.version = 1;
    config.id = 1;
    config.name = fc::string("Double color ball lottey");
    config.balls.push_back(std::pair<uint16_t, uint16_t>(35, 5));
    config.balls.push_back(std::pair<uint16_t, uint16_t>(12, 2));

    // level 1:
    uint16_t group_1_match_1[2] = {5, 2};
    match g1v1(group_1_match_1, group_1_match_1+2);
    std::vector< match> g1;
    g1.push_back(g1v1);
    config.prizes.push_back(std::pair<uint16_t, std::vector<match>>(1, g1));


    // level 2:
    uint16_t group_2_match_1[2] = {5, 1};
    match g2v1(group_2_match_1, group_2_match_1+2);
    std::vector< match> g2;
    g2.push_back(g2v1);
    config.prizes.push_back(std::pair<uint16_t, std::vector<match>>(2, g2));

    // level 3:
    uint16_t group_3_match_1[2] = {5, 0};
    match g3v1(group_3_match_1, group_3_match_1+2);
    std::vector< match> g3;
    g3.push_back(g3v1);
    config.prizes.push_back(std::pair<uint16_t, std::vector<match>>(3, g3));

    // level 4:
    uint16_t group_4_match_1[2] = {4, 2};
    match g4v1(group_4_match_1, group_4_match_1+2);
    std::vector< match> g4;
    g4.push_back(g4v1);
    config.prizes.push_back(std::pair<uint16_t, std::vector<match>>(4, g4));

    // level 5:
    uint16_t group_5_match_1[2] = {4, 1};
    match g5v1(group_5_match_1, group_5_match_1+2);
    std::vector< match> g5;
    g5.push_back(g5v1);
    config.prizes.push_back(std::pair<uint16_t, std::vector<match>>(5, g5));

    // level 6:
    uint16_t group_6_match_1[2] = {4, 0};
    match g6v1(group_6_match_1, group_6_match_1+2);
    uint16_t group_6_match_2[2] = {3, 2};
    match g6v2(group_6_match_2, group_6_match_2+2);
    std::vector< match> g6;
    g6.push_back(g6v1);
    g6.push_back(g6v2);
    config.prizes.push_back(std::pair<uint16_t, std::vector<match>>(6, g6));

    fc::variant var(config);
    auto str = fc::json::to_string(var);
	
    ilog( "block: \n${b}", ("b", str ) );

	const fc::path& p = "rule.json";
	fc::json::save_to_file(var, p);
}

BOOST_AUTO_TEST_CASE( test_load_rule_config )
{
	const rule_config& config = global_rule_config();
	BOOST_CHECK(config.balls.size() == 2);

	BOOST_CHECK(GROUP_COUNT() == 2);

	BOOST_CHECK(TOTAL_SPACE() == Combination(35, 5) * Combination(12, 2));

	BOOST_CHECK(GROUP_SPACES()[0] == Combination(35, 5));

	BOOST_CHECK(GROUP_SPACES()[1] == Combination(12, 2));
}

BOOST_AUTO_TEST_CASE( test_combination )
{
	// TODO
}

BOOST_AUTO_TEST_CASE( test_rule_validator )
{
	// TODO
}

BOOST_AUTO_TEST_CASE( test_hash_and )
{
	uint32_t left = 1;
	uint32_t right = 2;
	uint64_t result = ((1 << 32) & 2);
	BOOST_CHECK((((uint64_t)left << 32) & (uint64_t)right) == result);
}