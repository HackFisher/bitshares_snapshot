#include <bts/blockchain/config.hpp>
#include <bts/wallet/wallet.hpp>

#include <bts/lotto/lotto_wallet.hpp>
#include <bts/lotto/lotto_outputs.hpp>
#include <bts/lotto/lotto_config.hpp>

#include<fc/reflect/variant.hpp>
#include<fc/io/raw.hpp>
#include<fc/io/raw_variant.hpp>

#include <fc/log/logger.hpp>

namespace bts { namespace lotto {

using namespace bts::blockchain;

namespace detail 
{
		class lotto_wallet_impl
		{
			public:
				// TODO cache _un_drawed tickets
				// Temp to store _un_drawed ticket_output in _unspent_output
		};
} // namespace detail

lotto_wallet::lotto_wallet()
:my(new detail::lotto_wallet_impl())
{
}

lotto_wallet::~lotto_wallet()
{
}

bts::blockchain::signed_transaction lotto_wallet::buy_ticket(const uint64_t& luckynumber, const uint16_t& odds,
                                                        asset amount)
{
    try {
        signed_transaction trx;
        // TODO: validate lucknumber, odds, and amount
   
		auto jackpot_addr = new_receive_address("Owner address for jackpot");
        auto inputs = std::vector<trx_input>();

        auto ticket_output = claim_ticket_output();
        ticket_output.lucky_number = luckynumber;
        ticket_output.odds = odds;
        ticket_output.owner = jackpot_addr;

        // TODO: what's the meaning of amount here?
        trx.outputs.push_back(trx_output(ticket_output, amount));

        return collect_inputs_and_sign(trx, amount);
    } FC_RETHROW_EXCEPTIONS(warn, "buy_ticket ${luckynumber} with ${odds}", ("name", luckynumber)("amt", odds))
}

bts::blockchain::signed_transaction lotto_wallet::draw_ticket( lotto_db& lotto_db, const ticket_number& ticket_num )
{
	try {
        // TODO: using ticket_num.to_output_index() instead
        auto out_idx = ticket_num;

        FC_ASSERT(lotto_db.head_block_num() - out_idx.block_idx > BTS_LOTTO_BLOCKS_BEFORE_JACKPOTS_DRAW);

        auto unspent_outputs = get_unspent_outputs();
        FC_ASSERT(unspent_outputs.find(out_idx) != unspent_outputs.end());

        auto ticket_out = unspent_outputs[out_idx];
        FC_ASSERT(ticket_out.claim_func == claim_ticket);

        auto claim_ticket = ticket_out.as<claim_ticket_output>();

        signed_transaction trx;
		auto req_sigs = std::unordered_set<bts::blockchain::address>();

        auto jackpot = lotto_db.get_jackpot_for_ticket(out_idx, claim_ticket.lucky_number, claim_ticket.odds, ticket_out.amount);
        
        // Only support jackpot with same unit, but anyway it will be validate in lotto_state in trx validator
        FC_ASSERT(ticket_out.amount.unit == jackpot.unit);

        trx.inputs.push_back( trx_input( claim_ticket_input(),  get_ref_from_output_idx(out_idx)) );

        auto draw_output = claim_by_signature_output( ticket_out.as<claim_ticket_output>().owner );
		trx.outputs.push_back( trx_output( draw_output, jackpot ) );
		
        // Require the owner to draw the ticket, maybe, ticket draw trx could be deterministric trx, so could be draw automatic.
        req_sigs.insert( ticket_out.as<claim_ticket_output>().owner );

        return collect_inputs_and_sign(trx, asset(), req_sigs);
		
    } FC_RETHROW_EXCEPTIONS(warn, "draw_ticket with ${ticket_number}", ("ticket_number", ticket_num))
}

bool lotto_wallet::scan_output( transaction_state& state, const trx_output& out, const output_reference& out_ref, const bts::wallet::output_index& oidx )
{
    try {
        switch ( out.claim_func )
        {
            case claim_secret:
            {
                // pass, no need to cache
                return true;
            }
            case claim_ticket:
            {
                auto receive_address = out.as<claim_ticket_output>().owner; 
                if (is_my_address( receive_address ))
                {
					// Is my ticket do not ajust balance.
                    cache_output( state.trx.vote, out, out_ref, oidx );
                    state.from[ receive_address ] = get_receive_address_label( receive_address );
                    state.adjust_balance( out.amount, 1 );
                    return true;
                }
                else if( state.delta_balance.size() )
                {
                    // TODO: what is this?
                }
                return false;
            }
            default:
                return wallet::scan_output(state, out, out_ref, oidx );
        }
    } FC_RETHROW_EXCEPTIONS( warn, "" )
}

void lotto_wallet::scan_input( transaction_state& state, const output_reference& ref, const output_index& idx  ){
    // For claim secret output, there is no need to scan.
    // Because secret output is required to must have zero amount asset, so will not affect ajust balance
    // And, claim_secret is not cached, so the input is not ownered by the wallet.

    wallet::scan_input(state, ref, idx);
}

}} // bts::lotto
