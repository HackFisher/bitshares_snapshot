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

bts::blockchain::signed_transaction lotto_wallet::draw_ticket( lotto_db& lotto_db, const uint32_t& ticket_block_num )
{
	try {
        // TODO: more validations?
        FC_ASSERT(lotto_db.head_block_num() - ticket_block_num > BTS_LOTTO_BLOCKS_BEFORE_JACKPOTS_DRAW);
        
		// For each ticket draw transaction, the input.out_ref must have the same block number.
		auto unspent_outputs = get_unspent_outputs();
		signed_transaction trx;
		auto req_sigs = std::unordered_set<bts::blockchain::address>();
        
		for (auto out : unspent_outputs)
		{
			if (out.first.block_idx == ticket_block_num && out.second.claim_func == claim_ticket)
			{
                auto out_ref = get_ref_from_output_idx(out.first);
                auto output = lotto_db.fetch_output(out_ref);
                
                auto claim_ticket = output.as<claim_ticket_output>();
                
				uint64_t jackpot = lotto_db.get_jackpot_for_ticket(ticket_block_num, claim_ticket.lucky_number, claim_ticket.odds, output.amount.get_rounded_amount());
				trx.inputs.push_back( trx_input( claim_ticket_input(),  out_ref) );
				trx.outputs.push_back( trx_output( 
					claim_by_signature_output( out.second.as<claim_ticket_output>().owner), asset(jackpot, out.second.amount.unit) ) );
				req_sigs.insert( out.second.as<claim_ticket_output>().owner );
			}
		}

        return collect_inputs_and_sign(trx, asset(), req_sigs);
		
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
                    cache_output( state.trx.vote, out, out_ref, oidx );
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
