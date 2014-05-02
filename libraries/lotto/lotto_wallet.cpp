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

        trx.outputs.push_back(trx_output(ticket_output, amount));

        return collect_inputs_and_sign(trx, amount);
    } FC_RETHROW_EXCEPTIONS(warn, "buy_ticket ${luckynumber} with ${odds}, amount {amt}", ("name", luckynumber)("odds", odds)("amt", amount))
}

std::map<output_index, trx_output> lotto_wallet::list_tickets(lotto_db& db)
{
    try {
        std::map<output_index, trx_output> tickets_map;
        for (auto out : get_unspent_outputs())
        {
            if (out.second.claim_func = claim_ticket)
            {
                tickets_map[out.first] = out.second;
            }
        }

        return tickets_map;
    } FC_RETHROW_EXCEPTIONS(warn, "list tickets")
}

std::map<output_index, trx_output> lotto_wallet::list_jackpots(lotto_db& db)
{
    try {
        std::map<output_index, trx_output> jackpots_map;
        for (auto out : get_unspent_outputs())
        {
            if (out.second.claim_func = claim_jackpot)
            {
                jackpots_map[out.first] = out.second;
            }
        }

        return jackpots_map;
    } FC_RETHROW_EXCEPTIONS(warn, "list jackpots")
}

bts::blockchain::signed_transaction lotto_wallet::cash_jackpot(const output_index& jackpot_idx)
{
    try {
        auto unspent_outs = get_unspent_outputs();
        FC_ASSERT(unspent_outs.find(jackpot_idx) != unspent_outs.end(),
            "This jackpot does not exsit, or you do not own this jackpot!");

        auto out = unspent_outs[jackpot_idx];
        FC_ASSERT(out.claim_func == claim_jackpot, "This is not a jackpot!");

        auto jackpot_out = out.as<claim_jackpot_output>();

        signed_transaction trx;
        signature_set          required_sigs;

        FC_ASSERT(is_my_address(jackpot_out.owner));
        required_sigs.insert(jackpot_out.owner);

        auto inputs = std::vector<trx_input>();

        auto signature_output = claim_by_signature_output();
        signature_output.owner = jackpot_out.owner;

        trx.outputs.push_back(trx_output(signature_output, out.amount));

        return collect_inputs_and_sign(trx, asset(), required_sigs, 
            "cash out jackpot: " + std::string(jackpot_idx) + "; jackpot amount is: " + std::string(out.amount));
    } FC_RETHROW_EXCEPTIONS(warn, "cash_jackpot ${jackpot_idx}", ("jackpot_idx", jackpot_idx))
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
                    // TODO: then we are buying ticket for someone
                    state.to[receive_address] = get_send_address_label(receive_address);
                }
                return false;
            }
            case claim_jackpot:
            {
                auto receive_address = out.as<claim_jackpot_output>().owner;
                if (is_my_address(receive_address))
                {
                    cache_output(state.trx.vote, out, out_ref, oidx);
                    state.from[receive_address] = get_receive_address_label(receive_address);
                    state.adjust_balance(out.amount, 1);
                    return true;
                }
                else if (state.delta_balance.size())
                {
                    // then we are sending funds to someone... 
                    state.to[receive_address] = get_send_address_label(receive_address);
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
