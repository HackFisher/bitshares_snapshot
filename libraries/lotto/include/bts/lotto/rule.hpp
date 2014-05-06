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
    *  @brief base class of rule layer, to define rule specific things.
    */
    class rule
    {
        public:
            rule();
            virtual ~rule();

            virtual void                  open( const fc::path& dir, bool create = true );
            virtual void                  close();

            virtual asset jackpot_for_ticket(
                const bts::lotto::claim_ticket_output& ticket, const asset& amt, const output_index& out_idx) = 0;

            virtual void validate( const trx_block& blk, const signed_transactions& deterministic_trxs );

            virtual void store( const trx_block& blk, const signed_transactions& deterministic_trxs, const block_evaluation_state_ptr& state );

        private:
            std::unique_ptr<detail::rule_impl> my;
    };
    typedef std::shared_ptr<rule> rule_ptr;
} } // bts::lotto

