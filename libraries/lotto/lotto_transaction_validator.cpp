#include <bts/lotto/lotto_transaction_validator.hpp>
#include <bts/lotto/lotto_outputs.hpp>
#include <bts/lotto/lotto_db.hpp>
#include <bts/blockchain/config.hpp>
#include <fc/io/raw.hpp>

namespace bts { namespace lotto {

lotto_transaction_validator::lotto_transaction_validator(lotto_db* db)
:transaction_validator(db)
{
}

lotto_transaction_validator::~lotto_transaction_validator()
{
}

transaction_summary lotto_transaction_validator::evaluate( const signed_transaction& tx, const block_evaluation_state_ptr& block_state )
{
    lotto_trx_evaluation_state state(tx);
    return on_evaluate( state, block_state );
}


void lotto_transaction_validator::validate_input( const meta_trx_input& in, transaction_evaluation_state& state, const block_evaluation_state_ptr& block_state )
{
     switch( in.output.claim_func )
     {
        case claim_ticket:
            validate_ticket_input(in, state, block_state);
           break;
        default:
           transaction_validator::validate_input( in, state, block_state );
     }
}

void lotto_transaction_validator::validate_output( const trx_output& out, transaction_evaluation_state& state, const block_evaluation_state_ptr& block_state )
{
     switch( out.claim_func )
     {
        case claim_ticket:
            validate_ticket_output(out, state, block_state);
           break;
        default:
           transaction_validator::validate_output( out, state, block_state);
     }
}

void lotto_transaction_validator::validate_ticket_input(const meta_trx_input& in, transaction_evaluation_state& state, const block_evaluation_state_ptr& block_state)
{
	// 
	try {
		auto lotto_state = dynamic_cast<lotto_trx_evaluation_state&>(state);
		auto claim_ticket = in.output.as<claim_ticket_output>();

		auto trx_loc = in.source;
		auto headnum = _db->head_block_num();

		// ticket must have been purchased in the past 2 days
		FC_ASSERT( headnum - trx_loc.block_num < (BTS_BLOCKCHAIN_BLOCKS_PER_DAY*2) );
		// ticket must be before the last drawing... 
		FC_ASSERT( trx_loc.block_num < (headnum/BTS_BLOCKCHAIN_BLOCKS_PER_DAY)*BTS_BLOCKCHAIN_BLOCKS_PER_DAY );
		// ticket must be signed by owner
		FC_ASSERT( lotto_state.has_signature( claim_ticket.owner ) );
    
		lotto_db* db = dynamic_cast<lotto_db*>(_db);
		FC_ASSERT( db != nullptr );

		// returns the jackpot based upon which lottery the ticket was for.
		// throws an exception if the jackpot was already claimed.
		uint64_t jackpot  = db->get_jackpot_for_ticket( trx_loc.block_num, claim_ticket.lucky_number, claim_ticket.odds, in.output.amount.get_rounded_amount());

		if( jackpot > 0 ) // we have a winner!
		{
			lotto_state.add_input_asset( asset( jackpot ) ); 
			lotto_state.ticket_winnings += jackpot;
		} else {
			// throws or invalid when they did not win, is there necessary to do this?

		}

		/* TODO
		uint64_t winnings = db->get_winnings( in ); // throws if they did not win.
		lotto_state.ticket_winnings += winnings;
		lotto_state.add_input_asset( asset(winnings) );
		*/
	} FC_RETHROW_EXCEPTIONS( warn, "" ) 
}

void lotto_transaction_validator::validate_ticket_output(const trx_output& out, transaction_evaluation_state& state, const block_evaluation_state_ptr& block_state)
{
	try {
    auto lotto_state = dynamic_cast<lotto_trx_evaluation_state&>(state);
    lotto_state.total_ticket_sales += out.amount.get_rounded_amount();
    lotto_state.add_output_asset( out.amount );

	/*
    auto claim_ticket = out.output.as<claim_ticket_output>();
    FC_ASSERT( claim_ticket.odds 
		*/
} FC_RETHROW_EXCEPTIONS( warn, "" ) }


}} // bts::blockchain
