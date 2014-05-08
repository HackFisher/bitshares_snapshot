#define BOOST_TEST_MODULE LottoTests
#include <boost/test/unit_test.hpp>
#include <bts/lotto/lotto_wallet.hpp>
#include <bts/lotto/lotto_outputs.hpp>
#include <bts/lotto/lotto_rule.hpp>
#include <fc/filesystem.hpp>
#include <fc/log/logger.hpp>
#include <fc/io/raw.hpp>
#include <fc/reflect/variant.hpp>
#include <fc/thread/thread.hpp>
#include <fc/io/json.hpp>
#include <bitset>
#include <boost/shared_ptr.hpp>
#include <boost/make_shared.hpp>

#include <iostream>
using namespace bts::lotto;
using namespace bts::lotto::helper;

/**
 *  Test utility methods
 */
BOOST_AUTO_TEST_CASE( util_combination_to_int )
{
    combination ticket_v{ 2, 4, 17, 21, 33 };
	std::bitset<35> ticket_bits;
	for (int i = 0; i < 5; i++){
		ticket_bits[ticket_v[i]] = 1;
	}

	BOOST_CHECK(ticket_v.size() == 5);

    uint64_t ticket_num = ranking(ticket_v);

	// TODO: assert(ticket_num = ??);
	//uint64_t bits_ull = combination::int_to_combination_binary<35>(ticket_num);
    auto res_nums = unranking(ticket_num, 5, 35);
    BOOST_CHECK(res_nums.size() == 5);
	std::bitset<35> res_bits;
	for (int i = 0; i < 5; i++){
		std::cout << (uint16_t)res_nums[i] << "\n";
		res_bits[res_nums[i]] = 1;
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

BOOST_AUTO_TEST_CASE( util_generate_rule_config )
{
    lotto_rule::config config;
    config.version = 1;
    config.id = 1;
    config.name = fc::string("Double color ball lottey");
    config.asset_type = 0;
    config.ball_group.push_back(ball{ 35, 5 });
    config.ball_group.push_back(ball{ 12, 2 });

    prize prize_1;
    prize_1.level = 1;
    group_match match_1;
    match_1.push_back(5);
    match_1.push_back(2);
    prize_1.match_list.push_back(match_1);
    // level 1:
    config.prizes.push_back(prize_1);


    // level 2:
    prize prize_2;
    prize_2.level = 2;
    group_match match_2;
    match_2.push_back(5);
    match_2.push_back(1);
    prize_2.match_list.push_back(match_2);
    config.prizes.push_back(prize_2);

    // level 3:
    prize prize_3;
    prize_3.level = 3;
    group_match match_3;
    match_3.push_back(5);
    match_3.push_back(0);
    prize_3.match_list.push_back(match_3);
    config.prizes.push_back(prize_3);

    // level 4:
    prize prize_4;
    prize_4.level = 4;
    group_match match_4;
    match_4.push_back(4);
    match_4.push_back(2);
    prize_4.match_list.push_back(match_4);
    config.prizes.push_back(prize_4);

    // level 5:
    prize prize_5;
    prize_5.level = 5;
    group_match match_5;
    match_5.push_back(4);
    match_5.push_back(1);
    prize_5.match_list.push_back(match_5);
    config.prizes.push_back(prize_5);

    // level 6:
    prize prize_6;
    prize_6.level = 2;
    group_match match_6_1;
    match_6_1.push_back(4);
    match_6_1.push_back(0);
    prize_6.match_list.push_back(match_6_1);
    group_match match_6_2;
    match_6_2.push_back(3);
    match_6_2.push_back(2);
    prize_6.match_list.push_back(match_6_2);
    config.prizes.push_back(prize_6);

    fc::variant var(config);
    auto str = fc::json::to_string(var);
	
    ilog( "block: \n${b}", ("b", str ) );

	const fc::path& p = "lotto_rule.json";
	fc::json::save_to_file(var, p, true);
}

BOOST_AUTO_TEST_CASE( util_load_rule_config )
{
    BOOST_CHECK(lotto_rule::config_instance().ball_group.size() == 2);

	BOOST_CHECK(lotto_rule::config_instance().group_count() == 2);

	BOOST_CHECK(lotto_rule::total_space() == Combination(35, 5) * Combination(12, 2));

	BOOST_CHECK(lotto_rule::group_spaces()[0] == Combination(35, 5));

	BOOST_CHECK(lotto_rule::group_spaces()[1] == Combination(12, 2));
}

BOOST_AUTO_TEST_CASE( util_combination )
{
    BOOST_CHECK(Combination(156, 1) == 156);

    BOOST_CHECK(Combination(254, 30) == Combination(254, 254 - 30));

    BOOST_CHECK(Combination(133, 4) == (133 * 132 * 131 * 130) / (4 * 3 * 2 * 1));

    auto no_exception = false;
    try
    {
        // N should not be larger than 256
        Combination(256, 3);
        no_exception = true;
    }
    catch (const fc::exception& e)
    {
        no_exception = false;
        elog("${e}", ("e", e.to_detail_string()));
    }
    catch (...)
    {
        no_exception = false;
    }

    BOOST_CHECK(no_exception == false);
}

BOOST_AUTO_TEST_CASE(util_rule_validator)
{
	// TODO
}

BOOST_AUTO_TEST_CASE(util_hash_and)
{
	uint32_t left = 1;
	uint32_t right = 2;
    uint64_t result = ((uint64_t)((uint64_t)1 << 32) & 2);
	BOOST_CHECK((((uint64_t)left << 32) & (uint64_t)right) == result);
}

BOOST_AUTO_TEST_CASE(util_c_ranking)
{
    c_rankings cr;
    cr.push_back(6);
    cr.push_back(34);
    cr.push_back(17);

    c_rankings spaces;
    spaces.push_back(21);
    spaces.push_back(50);
    spaces.push_back(100);
    BOOST_CHECK( ranking(cr, spaces) == ( 6 * 50 * 100 + 34 * 100 + 17 ) );

    BOOST_CHECK( unranking( 6 * 50 * 100 + 34 * 100 + 17, spaces) == cr );
}

BOOST_AUTO_TEST_CASE(util_random_hash)
{
    fc::sha256 revealed_secret("FFF01234ABCDDDE");
    std::cout << revealed_secret.str().c_str() << "\n";
    fc::sha256 secret = fc::sha256::hash(revealed_secret);
    std::cout << secret.str().c_str() << "\n";

    BOOST_CHECK(revealed_secret != secret);
    BOOST_CHECK(secret._hash[0] == fc::hash64(revealed_secret.data(), 32));

    try
    {
        fc::sha256 revealed_secret("I'm not hex string");
    }
    catch (const fc::exception& e)
    {
        std::cout << e.to_detail_string();
    }
}