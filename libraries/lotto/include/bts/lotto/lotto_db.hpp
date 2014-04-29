
#pragma once
#include <bts/blockchain/asset.hpp>
#include <bts/blockchain/transaction.hpp>
#include <bts/blockchain/chain_database.hpp>
#include <bts/wallet/wallet.hpp>

#include <bts/lotto/lotto_transaction_validator.hpp>
#include <bts/lotto/lotto_rule_validator.hpp>

namespace bts { namespace lotto {
using namespace bts::wallet;

namespace detail  { class lotto_db_impl; }

struct drawing_record
{
   drawing_record()
   :total_jackpot(0),total_paid(0), jackpot_pool(0){}

   uint64_t total_jackpot;	// total jackpot of this block
   uint64_t total_paid;		// total jackpot which have been paid in future block.
   uint64_t jackpot_pool;
};
struct block_summary
{
   block_summary()
   :ticket_sales(0),amount_won(0), winning_number(0){}

   uint64_t ticket_sales;	// total ticket sales in the blocks, from 0 to .... current
   uint64_t amount_won;		// total pay out amount from 0 to previous block
   uint64_t winning_number;	// the previous block uses this value to calculate jackpot, this value is generated according current block's info
};

class lotto_db : public bts::blockchain::chain_database
{
    public:
        lotto_db();
        ~lotto_db();
    
        void             open( const fc::path& dir, bool create );
        void             close();

		void set_rule_validator( const rule_validator_ptr& v );

        asset get_jackpot_for_ticket( output_index out_idx, uint64_t lucky_number, uint16_t odds, uint16_t amount );

        /**
         * Performs global validation of a block to make sure that no two transactions conflict. In
         * the case of the lotto only one transaction can claim the jackpot.
         */
        virtual block_evaluation_state_ptr validate( const trx_block& blk, const signed_transactions& deterministic_trxs );

        /** 
         *  Called after a block has been validated and appends
         *  it to the block chain storing all relevant transactions and updating the
         *  winning database.
         */
        virtual void store( const trx_block& blk, const signed_transactions& deterministic_trxs, const block_evaluation_state_ptr& state );

        /**
         * When a block is popped from the chain, this method implements the necessary code
         * to revert the blockchain database to the proper state.
         */
        virtual trx_block pop_block();
    private:
         std::unique_ptr<detail::lotto_db_impl> my;

};

typedef std::shared_ptr<lotto_db> lotto_db_ptr;

}} // bts::lotto


FC_REFLECT( bts::lotto::drawing_record, (total_jackpot)(total_paid)(jackpot_pool) )
FC_REFLECT( bts::lotto::block_summary, (ticket_sales)(amount_won)(winning_number))
