#pragma once
#include <bts/lotto/rule.hpp>

namespace bts { namespace lotto {
    namespace detail  { class betting_rule_impl; }

    class lotto_db;

    class betting_rule : public rule
    {
        public:

            betting_rule(lotto_db* db);
            virtual ~betting_rule();

            virtual void                  open(const fc::path& dir, bool create = true);
            virtual void                  close();

            virtual asset jackpot_for_ticket(const meta_ticket_output& meta_ticket_out);

            virtual void store(const trx_block& blk, const signed_transactions& deterministic_trxs, const block_evaluation_state_ptr& state);

        protected:
            std::unique_ptr<detail::betting_rule_impl> my;
    };
        typedef std::shared_ptr<betting_rule> betting_rule_ptr;

} } // bts::lotto

