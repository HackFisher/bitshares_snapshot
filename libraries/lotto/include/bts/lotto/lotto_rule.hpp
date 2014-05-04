#pragma once
#include <fc/reflect/variant.hpp>

#include <bts/lotto/rule.hpp>

namespace bts { namespace lotto {
    /**
    *  @brief encapsulates an combination number in
    *  integer form.   It can be converted to arrays or bits for matching
    *  winning or input purposes and can also be constructed from an combinatorial representation of integer
    *  according to Combinatorial number system
    *  TODO: decribe data infos and bytes etc.
    *  2. change to struct seems to be better, is one ticket value
    *  3. some function change to inline
    *  4. TODO: replace vectors and binary with BOOST_BINARY( 100 111000 01 1 110 )
    *    http://www.boost.org/doc/libs/1_42_0/libs/utility/utility.htm#BOOST_BINARY
    */
    typedef std::vector<uint16_t>                  combination;
    typedef std::vector<uint16_t>                  group_match;

    struct ball
    {
        uint16_t total_count;
        uint16_t combination_count;
    };

    struct prize
    {
        uint16_t level;
        std::vector<group_match> match_list;
    };
    /* Ranking groups
    *
    */
    typedef std::vector<uint64_t>						c_rankings;

    typedef std::vector<ball>	                        type_ball_group;
    typedef std::vector<prize>				            type_prizes;

namespace detail  { class lotto_rule_impl; }

class lotto_db;
/**
 *  @class lotto_rule
 *  @brief base class used for evaluating rule and calculating jackpots
 *
 *  This lotto_rule can be implemented by inheritance lotto DACs.
 */
class lotto_rule : public bts::lotto::rule
{
   public:
       struct config
       {
           config() :valid(false), version(0), id(0), name(""), asset_type(0){}
           bool											   valid;
           uint16_t                                        version;
           uint32_t                                        id;
           fc::string                                      name;
           bts::blockchain::asset::unit_type               asset_type;
           type_ball_group                                 ball_group;
           type_prizes                                     prizes;
       };

       lotto_rule(lotto_db* db);
       virtual ~lotto_rule();

	  /**
	   * Provided with winning_number, calculated the total jackpots will be in the block
	   * using this to update the following blocks' drawing_record's (total_jackpot, total_paid)
	   * will be used for block validation
	   */
	  virtual uint64_t evaluate_total_jackpot(const uint64_t& winning_number, const uint64_t& target_block_num, const uint64_t& available_funds);
      
      virtual uint64_t jackpot_for_ticket(const uint64_t& winning_number, 
          const bts::lotto::claim_ticket_output& ticket, const uint64_t& amt, const uint64_t& total_jackpots);

   protected:
       std::unique_ptr<detail::lotto_rule_impl> my;
};
typedef std::shared_ptr<lotto_rule> lotto_rule_ptr;

}} // bts::lotto
#include <fc/reflect/reflect.hpp>
FC_REFLECT(bts::lotto::ball, (total_count)(combination_count))
FC_REFLECT(bts::lotto::prize, (level)(match_list))
FC_REFLECT(bts::lotto::lotto_rule::config, (version)(id)(name)(asset_type)(ball_group)(prizes))

