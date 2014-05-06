#pragma once
#include <bts/lotto/rule.hpp>

namespace bts { namespace lotto {
    namespace detail  { class betting_rule_impl; }

    class betting_rule : public bts::lotto::rule
    {
        public:

            betting_rule();
            virtual ~betting_rule();

            /**
            * Provided with winning_number, calculated the total jackpots will be in the block
            * using this to update the following blocks' drawing_record's (total_jackpot, total_paid)
            * will be used for block validation
            */
            virtual uint64_t evaluate_total_jackpot(const uint64_t& winning_number, const uint64_t& ticket_sale, const uint64_t& target_block_num, const uint64_t& jackpot_pool);

            virtual uint64_t jackpot_for_ticket(const uint64_t& winning_number,
                const bts::lotto::claim_ticket_output& ticket, const uint64_t& amt, const output_index& out_idx);

        protected:
            std::unique_ptr<detail::betting_rule_impl> my;
    };
        typedef std::shared_ptr<betting_rule> betting_rule_ptr;

} } // bts::lotto

