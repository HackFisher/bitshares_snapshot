#pragma once
#include <bts/blockchain/asset.hpp>
#include <bts/blockchain/transaction.hpp>
#include <bts/blockchain/chain_database.hpp>
#include <bts/wallet/wallet.hpp>

#include <bts/lotto/lotto_transaction_evaluation_state.hpp>
#include <bts/lotto/rule.hpp>
#include <bts/lotto/ticket.hpp>

namespace bts { namespace lotto {
using namespace bts::wallet;

namespace detail  { class lotto_db_impl; }

struct block_summary
{
   block_summary()
   :random_number(0){}

   uint64_t random_number; // the previous block uses this value to calculate jackpot, this value is generated according current block's info
};

class lotto_db : public bts::blockchain::chain_database
{
    public:
        lotto_db();
        ~lotto_db();

        void            open( const fc::path& dir, fc::path genesis_file );
        void            close();

        rule_ptr        get_rule_ptr(const ticket_type& type);

        void            validate_secret_transactions(const signed_transactions& deterministic_trxs, const full_block& blk);

        uint64_t        fetch_blk_random_number( const uint32_t& blk_index );

        bool            is_new_delegate(const uint32_t& delegate_id);
        
        std::vector<uint32_t>   fetch_blocks_idxs(const uint32_t& delegate_id);

        secret_operation     fetch_secret(const uint32_t& blk_index);

        /**
        *  Evaluate the transaction and return the results.
        */
        virtual transaction_evaluation_state_ptr evaluate_transaction(const signed_transaction& trx) override;

        /**
         * Performs global validation of a block to make sure that no two transactions conflict. In
         * the case of the lotto only one transaction can claim the jackpot.
         */
        // virtual block_evaluation_state_ptr validate( const full_block& blk, const signed_transactions& deterministic_trxs );

        /**
         *  Called after a block has been validated and appends
         *  it to the block chain storing all relevant transactions and updating the
         *  winning database.
         */
        // virtual void store( const full_block& blk, const signed_transactions& deterministic_trxs, const block_evaluation_state_ptr& state );
    private:
         std::unique_ptr<detail::lotto_db_impl> my;

};

typedef std::shared_ptr<lotto_db> lotto_db_ptr;

}} // bts::lotto

FC_REFLECT( bts::lotto::block_summary, (random_number))
FC_REFLECT_TYPENAME(std::vector<uint32_t>);
