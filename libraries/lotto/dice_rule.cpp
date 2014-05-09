#pragma once
#include <bts/lotto/dice_rule.hpp>
#include <bts/lotto/lotto_config.hpp>

#include <bts/lotto/lotto_db.hpp>

#include <bts/db/level_map.hpp>

namespace bts { namespace lotto {
    namespace detail
    { 
        class dice_rule_impl        {        public:            dice_rule_impl(){}        };
    }

    dice_rule::dice_rule(lotto_db* db, ticket_type t, asset::unit_type u)
        :rule(db, t, u), my(new detail::dice_rule_impl())
    {
    }

    dice_rule::~dice_rule()
    {
    }

    uint64_t dice_rule::calculate_payout(uint64_t block_random, uint64_t ticket_random, uint16_t odds, uint64_t amt)
    {
        uint64_t range = 100000000;
        double house_edge = 0.01;
        double payout = 0;

        if ( ( ( (block_random % range  + ticket_random % range ) % range ) * odds) < range )
        {
            payout = amt * odds * (1 - house_edge);
        }

        return (uint64_t)payout;
    }

    // https://bitsharestalk.org/index.php?topic=4505.0
    asset dice_rule::jackpot_payout(const meta_ticket_output& meta_ticket_out)
    {
        try {
            auto headnum = _lotto_db->head_block_num();
            FC_ASSERT(meta_ticket_out.ticket_out.ticket.ticket_func == get_ticket_type());
            
            FC_ASSERT(headnum >= BTS_LOTTO_BLOCKS_BEFORE_JACKPOTS_DRAW  + meta_ticket_out.out_idx.block_idx);

            uint64_t random_number = _lotto_db->fetch_blk_random_number(meta_ticket_out.out_idx.block_idx + BTS_LOTTO_BLOCKS_BEFORE_JACKPOTS_DRAW);

            auto trx = _lotto_db->fetch_trx(trx_num(meta_ticket_out.out_idx.block_idx, meta_ticket_out.out_idx.trx_idx));
            auto trx_hash = trx.id();
            uint64_t trx_random = fc::sha256::hash((char*)&trx_hash, sizeof(trx_hash))._hash[0];

            auto ticket = meta_ticket_out.ticket_out.ticket.as<dice_ticket>();

            uint64_t payout = calculate_payout(random_number, trx_random, ticket.odds, meta_ticket_out.amount.get_rounded_amount());

            return asset(payout, meta_ticket_out.amount.unit);

        } FC_RETHROW_EXCEPTIONS(warn, "Error calculating jackpots for dice ticket ${m}", ("m", meta_ticket_out));
    }

} } // bts::lotto