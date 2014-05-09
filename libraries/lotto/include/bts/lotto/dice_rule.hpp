#pragma once
#include <bts/lotto/rule.hpp>

namespace bts { namespace lotto {
    namespace detail  { class dice_rule_impl; }

    class dice_rule : public rule
    {
        public:

            dice_rule(lotto_db* db, ticket_type t, asset::unit_type u);
            virtual ~dice_rule();

            uint64_t calculate_payout(uint64_t block_random, uint64_t ticket_random, uint16_t odds, uint64_t amt);

            virtual asset jackpot_payout(const meta_ticket_output& meta_ticket_out);

        protected:
            std::unique_ptr<detail::dice_rule_impl> my;
    };

    typedef std::shared_ptr<dice_rule> dice_rule_ptr;

} } // bts::lotto

