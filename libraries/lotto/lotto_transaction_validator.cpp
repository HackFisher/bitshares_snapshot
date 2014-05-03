#include <algorithm>

#include <fc/io/raw.hpp>
#include <fc/log/logger.hpp>

#include <bts/lotto/lotto_transaction_validator.hpp>
#include <bts/lotto/lotto_outputs.hpp>
#include <bts/lotto/lotto_db.hpp>
#include <bts/blockchain/config.hpp>
#include <bts/lotto/lotto_config.hpp>

namespace bts { namespace lotto {

lotto_transaction_validator::lotto_transaction_validator(lotto_db* db)
:transaction_validator(db)
{
    _lotto_db = db;
}

lotto_transaction_validator::~lotto_transaction_validator()
{
}

/*
 * If one of tx.inputs are claim ticket, then all its inputs should be claim tickets, and all output should be claim jackpot.
 * And all the mature_day of the outputs must allocate in different days.
 */
void validate_ticket_jackpot_transactions(const lotto_trx_evaluation_state& state)
{
    for ( auto in : state.inputs )
    {
        FC_ASSERT(in.output.claim_func = claim_ticket);
    }

    std::vector<uint16_t> mature_days;
    for ( auto out : state.trx.outputs )
    {
        FC_ASSERT(out.claim_func = claim_jackpot);
        auto jackpot_out = out.as<claim_jackpot_output>();
        mature_days.push_back(jackpot_out.mature_day);
    }

    std::sort(mature_days.begin(), mature_days.end());
    for ( size_t i = 0; i < mature_days.size(); i ++ )
    {
        if ( i == 0 )
        {
            continue;
        }
        else
        {
            FC_ASSERT((mature_days[i] - mature_days[i-1]) >= 1)
        }
    }
}

transaction_summary lotto_transaction_validator::evaluate( const signed_transaction& tx, const block_evaluation_state_ptr& block_state )
{
    lotto_trx_evaluation_state state(tx);

    auto inputs = _lotto_db->fetch_inputs(state.trx.inputs);
    // TODO: state.inputs is fetch on upstream
    for (auto in : inputs)
    {
        if (in.output.claim_func == claim_ticket)
        {
            validate_ticket_jackpot_transactions(state);
            // TODO: hacking deterministic transactions do not need fees, votes... etc...
            // DO not need on_evaluate, with assumption that all claim_tickets input are in deterministic trxs 
            return transaction_summary();
            //break;
        }
    }

    transaction_summary sum = on_evaluate( state, block_state );

    for (auto out : tx.outputs)
    {
        // TODO: hacking, generate_next_block need fees, or it will ignore the transaction.
        if (out.claim_func == claim_secret)
        {
            sum.fees = 1000;
            break;
        }
    }

    return sum;
}


void lotto_transaction_validator::validate_input( const meta_trx_input& in, transaction_evaluation_state& state, const block_evaluation_state_ptr& block_state )
{
     switch( in.output.claim_func )
     {
        case claim_secret:
		   // pass, validation is done in block
		   break;
        case claim_ticket:
           validate_ticket_input(in, state, block_state);
           break;
        case claim_jackpot:
           validate_jackpot_input(in, state, block_state);
           break;
        default:
           transaction_validator::validate_input( in, state, block_state );
     }
}

void lotto_transaction_validator::validate_output( const trx_output& out, transaction_evaluation_state& state, const block_evaluation_state_ptr& block_state )
{
     switch( out.claim_func )
     {
        case claim_secret:
           FC_ASSERT(out.amount.get_rounded_amount() == 0, "Amount for secret output is required to be zero");
		   // pass, validation is done in block
		   break;
        case claim_ticket:
           validate_ticket_output(out, state, block_state);
           break;
		case claim_jackpot:
           validate_jackpot_output(out, state, block_state);
           break;
        default:
           transaction_validator::validate_output( out, state, block_state);
     }
}

void lotto_transaction_validator::validate_ticket_input(const meta_trx_input& in, transaction_evaluation_state& state, const block_evaluation_state_ptr& block_state)
{
	try {
		auto lotto_state = dynamic_cast<lotto_trx_evaluation_state&>(state);
		auto claim_ticket = in.output.as<claim_ticket_output>();

        

		auto trx_loc = in.source;
		auto headnum = _db->head_block_num();

		// ticket must have been purchased in the past 7 days
		FC_ASSERT( headnum - trx_loc.block_num < (BTS_BLOCKCHAIN_BLOCKS_PER_DAY*7) );
		// ticket must be before the last drawing... 
		FC_ASSERT( trx_loc.block_num < (headnum/BTS_BLOCKCHAIN_BLOCKS_PER_DAY)*BTS_BLOCKCHAIN_BLOCKS_PER_DAY );
        FC_ASSERT(headnum - trx_loc.block_num > BTS_LOTTO_BLOCKS_BEFORE_JACKPOTS_DRAW);
		// TODO: For current, the ticket draw trx must be created by the owner.
        FC_ASSERT( lotto_state.has_signature( claim_ticket.owner ), "", ("owner",claim_ticket.owner)("sigs",state.sigs) );
    
		lotto_db* _lotto_db = dynamic_cast<lotto_db*>(_db);
        FC_ASSERT(_lotto_db != nullptr);

		// returns the jackpot based upon which lottery the ticket was for.
		// throws an exception if the jackpot was already claimed.
        auto jackpot = _lotto_db->draw_jackpot_for_ticket(trx_loc.block_num, claim_ticket.lucky_number, claim_ticket.odds, in.output.amount);
        
        

		if( jackpot > 0 ) // we have a winner!
		{
            // For assert jackpot equals output.amount, by add asset to input in state
            lotto_state.add_input_asset(jackpot);
            // TODO: improve ticket_winnings to support multi assets
			lotto_state.ticket_winnings += jackpot.get_rounded_amount();
		} else {
			// throws or invalid when they did not win, is there necessary to do this?

		}
} FC_RETHROW_EXCEPTIONS( warn, "" ) }

void lotto_transaction_validator::validate_ticket_output(const trx_output& out, transaction_evaluation_state& state, const block_evaluation_state_ptr& block_state)
{
	try {
    auto lotto_state = dynamic_cast<lotto_trx_evaluation_state&>(state);
    lotto_state.total_ticket_sales += out.amount.get_rounded_amount();
    lotto_state.add_output_asset( out.amount );

	// TODO: later we might going to support customize the winner address
} FC_RETHROW_EXCEPTIONS( warn, "" ) }


void lotto_transaction_validator::validate_jackpot_input(const meta_trx_input& in, transaction_evaluation_state& state, const block_evaluation_state_ptr& block_state)
{
    try {
       auto claim = in.output.as<claim_jackpot_output>(); 
       FC_ASSERT( state.has_signature( claim.owner ), "", ("owner",claim.owner)("sigs",state.sigs) );
       
       FC_ASSERT( (_db->head_block_num() - in.source.block_num) >= (claim.mature_day * BTS_BLOCKCHAIN_BLOCKS_PER_DAY),
           "The jackpot input should be after mature day");

       state.add_input_asset( in.output.amount );

       // TODO: https://github.com/HackFisher/bitshares_toolkit/issues/24
       if( in.output.amount.unit == 0 )
       {
          accumulate_votes( in.output.amount.get_rounded_amount(), in.source.block_num, state );
          block_state->add_input_delegate_votes( in.delegate_id, in.output.amount );
          block_state->add_output_delegate_votes( state.trx.vote, in.output.amount );
       }

} FC_RETHROW_EXCEPTIONS( warn, "" ) }

/*
 * The jackpot output amount should be less than the maximum limitation
 */
void lotto_transaction_validator::validate_jackpot_output(const trx_output& out, transaction_evaluation_state& state, const block_evaluation_state_ptr& block_state)
{
	try {
        FC_ASSERT(out.amount.get_rounded_amount() <= BTS_LOTTO_RULE_MAXIMUM_REWARDS_EACH_JACKPOT_OUTPUT);

        state.add_output_asset(out.amount);

} FC_RETHROW_EXCEPTIONS( warn, "" ) }

}} // bts::lotto
