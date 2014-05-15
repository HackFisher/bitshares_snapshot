#include <bts/lotto/lotto_chain_state.hpp>

namespace bts { namespace lotto {
    lotto_chain_state::lotto_chain_state(lotto_db_ptr prev_state)
        :pending_chain_state(prev_state)
    {
    }


    lotto_chain_state::~lotto_chain_state()
    {
    }

    void lotto_chain_state::apply_deterministic_updates()
    {
        pending_chain_state::apply_deterministic_updates();
        /*
        signed_transactions signed_trxs = chain_database::generate_deterministic_transactions();

        // being in genesis block pushing
        if (head_block_num() == transaction_location::invalid_block_num)
        {
            return signed_trxs;
        }

        if (head_block_num() < BTS_LOTTO_BLOCKS_BEFORE_JACKPOTS_DRAW)
        {
            return signed_trxs;
        }
        auto ticket_block_num = head_block_num() - BTS_LOTTO_BLOCKS_BEFORE_JACKPOTS_DRAW;
        // TODO validate unique of deterministric and common trxs
        // TODO and ticket output should not exsits in deterministric trxs
        auto ticket_blk = fetch_trx_block(ticket_block_num);
        for (auto trx : ticket_blk.trxs)
        {
            for (size_t i = 0; i < trx.outputs.size(); i ++ )
            {
                auto out = trx.outputs[i];
                // TODO: spent should not happen, e.g. in current/before block's trxs
                // All ticket are drawn in deterministic way
                if (out.claim_func == claim_ticket)
                {
                    signed_transaction draw_trx;
                    // TODO: deterministic ticket draw trx is quite different from wallet generated trxs using select_delegate_vote()
                    // For deterministic, now is using input's vote as the vote of this trx. To be synced with toolkit
                    draw_trx.stake = get_stake();
                    draw_trx.vote = trx.vote;
                    auto ticket_out = out.as<claim_ticket_output>();
                    auto trx_num = fetch_trx_num(trx.id());
                    // TODO: why not directly send in.output in to 
                    auto jackpot = my->_rules[ticket_out.ticket.ticket_func]->jackpot_payout(meta_ticket_output(trx_num, i, ticket_out, out.amount));
                    
                    if (jackpot.get_rounded_amount() > 0)   // There is a jackpot for this ticket
                    {
                        uint16_t mature = 0;
                        while (jackpot.get_rounded_amount() > BTS_LOTTO_RULE_MAXIMUM_REWARDS_EACH_JACKPOT_OUTPUT)
                        {
                            claim_jackpot_output jackpot_out(ticket_out.owner, mature);
                            asset amt(BTS_LOTTO_RULE_MAXIMUM_REWARDS_EACH_JACKPOT_OUTPUT, jackpot.unit);
                            draw_trx.outputs.push_back( trx_output( jackpot_out, amt ) );
                            jackpot -= amt;
                            ++ mature;
                        }

                        draw_trx.inputs.push_back( trx_input( claim_ticket, output_reference( trx.id(), i ) ) );
                        draw_trx.outputs.push_back(trx_output(claim_jackpot_output(ticket_out.owner, mature), jackpot));

                        signed_trxs.push_back(draw_trx);
                    }
                    else
                    {
                        // There is no jackpot for this ticket
                        // TODO: Then this tickets without jackpots will be dead in this case.
                        // TODO: To fix this, may need to automatic clean lotto wallet balance after draw.
                    }
                    
                }
            }

        }
        */
    }

    void  lotto_chain_state::apply_changes() const
    {

    }
} } // bts::lotto