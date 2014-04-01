#pragma once
#include <fc/reflect/variant.hpp>

namespace bts { namespace lotto {

class lotto_db;
/**
 *  @class rule_validator
 *  @brief base class used for evaluating rule and calculating jackpots
 *
 *  This rule_validator can be implemented by inheritance lotto DACs.
 */
class rule_validator
{
   public:
      rule_validator( lotto_db* db );
      virtual ~rule_validator();

	  /**
	   * Provided with winning_number, calculated the total jackpots will be in the block
	   * using this to update the following blocks' drawing_record's (total_jackpot, total_paid)
	   * will be used for block validation
	   */
	  virtual uint64_t evaluate_total_jackpot(uint64_t winning_number, uint64_t target_block_num);
      
      virtual uint64_t evaluate_jackpot(uint64_t winning_number, uint64_t lucky_number, uint16_t amount);

   protected:
      lotto_db* _db;
};
typedef std::shared_ptr<rule_validator> rule_validator_ptr;
}} // bts::lotto

