#include <bts/blockchain/config.hpp>
#include <bts/wallet/wallet.hpp>

#include <bts/lotto/lotto_wallet.hpp>
#include <bts/lotto/lotto_outputs.hpp>

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
   
        auto jackpot_addr = new_recv_address("Owner address for jackpot, lucky number is " + luckynumber);
        auto change_addr = new_recv_address("Change address");
        auto req_sigs = std::unordered_set<bts::blockchain::address>();
        auto inputs = std::vector<trx_input>();
        auto total_in = bts::blockchain::asset(); // set by collect_inputs

        auto ticket_output = claim_ticket_output();
        ticket_output.lucky_number = luckynumber;
        ticket_output.odds = odds;
        ticket_output.owner = jackpot_addr;

        // TODO: what's the asset/coin here for lotto?
        auto required_in = amount;
        trx.inputs = collect_inputs( required_in, total_in, req_sigs);
        auto change_amt = total_in - required_in;

        // TODO: what's the meaning of amount here?
        trx.outputs.push_back(trx_output(ticket_output, amount));
        trx.outputs.push_back(trx_output(claim_by_signature_output(change_addr), change_amt));
        // what the next step doing?
        trx.sigs.clear();
        sign_transaction(trx, req_sigs, false);

        // TODO
        //trx = add_fee_and_sign(trx, amount, total_in, req_sigs);

        return trx;
    } FC_RETHROW_EXCEPTIONS(warn, "buy_ticket ${luckynumber} with ${odds}", ("name", luckynumber)("amt", odds))
}

bts::blockchain::signed_transaction lotto_wallet::draw_ticket()
{
	try {
		// TODO: if draw transaction, should be one in one out, or X in X out?
		auto unspent_outputs = get_unspent_outputs();
		signed_transaction trx;
		auto req_sigs = std::unordered_set<bts::blockchain::address>();
		for (auto out : unspent_outputs)
		{
			if (out.second.claim_func == claim_ticket)
			{
				uint64_t jackpot = 0;	// TODO: Check output valid block, and calcute the jackpots for this output.
				trx.inputs.push_back( trx_input( claim_ticket_input(), get_ref_from_output_idx(out.first) ) ); 
				trx.outputs.push_back( trx_output( 
					claim_by_signature_output( out.second.as<claim_ticket_output>().owner), asset(jackpot, out.second.amount.unit) ) );
				req_sigs.insert( out.second.as<claim_ticket_output>().owner );
			}
		}

		trx.sigs.clear();
        sign_transaction(trx, req_sigs, false);

		return trx;
		
	} FC_RETHROW_EXCEPTIONS(warn, "draw_ticket ")
}

bool lotto_wallet::scan_output( transaction_state& state, const trx_output& out, const output_reference& out_ref, const bts::wallet::output_index& oidx )
{
    try {
        switch ( out.claim_func )
        {
            case claim_ticket:
            {
                if (is_my_address( out.as<claim_ticket_output>().owner ))
                {
					// Is my ticket do not ajust balance.
                    cache_output( out, out_ref, oidx );
                    return true;
                }
                return false;
            }
            default:
                return wallet::scan_output(state, out, out_ref, oidx );
        }
    } FC_RETHROW_EXCEPTIONS( warn, "" )
}

void lotto_wallet::scan_input( transaction_state& state, const output_reference& ref, const output_index& idx  ){
    auto itr = get_unspent_outputs().find(idx);
    if( itr != get_unspent_outputs().end() )
    {
        if (itr->second.claim_func == claim_ticket) {
            return;
        } else {
            wallet::scan_input(state, ref, idx);
            return;
        }
    }

    // TODO: scan from spent outputs?
    wallet::scan_input(state, ref, idx);
}

}} // bts::lotto
