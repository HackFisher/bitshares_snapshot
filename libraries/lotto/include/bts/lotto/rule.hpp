#pragma once
#include <fc/reflect/variant.hpp>

#include <bts/blockchain/asset.hpp>
#include <bts/blockchain/transaction.hpp>
#include <bts/blockchain/chain_database.hpp>
#include <bts/wallet/wallet.hpp>
#include <bts/lotto/lotto_outputs.hpp>

namespace bts { namespace lotto {
    using namespace bts::wallet;

    namespace detail
    {
        class rule_impl;
    }
    /**
    *  @class rule
    *  @brief base class used for evaluating rule and calculating jackpots
    *
    *  TODO: This lotto_rule can be implemented by inheritance lotto DACs.
    */
    class rule
    {
        public:
            rule();
            virtual ~rule();

            virtual void                  open( const fc::path& dir, bool create = true );
            virtual void                  close();

            /**
            * Provided with winning_number, calculated the total jackpots will be in the block
            * using this to update the following blocks' drawing_record's (total_jackpot, total_paid)
            * will be used for block validation
            */
            virtual uint64_t evaluate_total_jackpot(const uint64_t& winning_number, const uint64_t& ticket_sale, const uint64_t& target_block_num, const uint64_t& jackpot_pool) = 0;

            virtual uint64_t jackpot_for_ticket(const uint64_t& winning_number,
                const bts::lotto::claim_ticket_output& ticket, const uint64_t& amt, const output_index& out_idx) = 0;

            virtual void validate( const trx_block& blk, const signed_transactions& deterministic_trxs );

            virtual void store( const trx_block& blk, const signed_transactions& deterministic_trxs, const block_evaluation_state_ptr& state );

        private:
            std::unique_ptr<detail::rule_impl> my;
    };
    typedef std::shared_ptr<rule> rule_ptr;
} } // bts::lotto

