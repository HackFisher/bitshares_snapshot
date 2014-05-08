#pragma once
#include <bts/lotto/betting_rule.hpp>
#include <bts/lotto/lotto_config.hpp>

#include <bts/lotto/lotto_db.hpp>

#include <bts/db/level_map.hpp>

namespace bts { namespace lotto {
    namespace detail
    { 
        class betting_rule_impl        {        public:            betting_rule_impl(){}            bts::db::level_map<uint32_t, uint64_t>  _block2ticket_sale;        };
    }

    betting_rule::betting_rule(lotto_db* db, ticket_type t, asset::unit_type u)
        :rule(db, t, u), my(new detail::betting_rule_impl())
    {
    }

    betting_rule::~betting_rule()
    {
    }

    void betting_rule::open(const fc::path& dir, bool create)
    {
        try {
            my->_block2ticket_sale.open(dir / "betting_rule" / "block2ticket_sale", create);
        } FC_RETHROW_EXCEPTIONS(warn, "Error loading betting rule database ${dir}", ("dir", dir)("create", create));
    }

    void betting_rule::close()
    {
        my->_block2ticket_sale.close();
    }

    asset betting_rule::jackpot_for_ticket(const meta_ticket_output& meta_ticket_out)
    {
        try {
            FC_ASSERT(meta_ticket_out.ticket_out.ticket.ticket_func == get_ticket_type());
            auto headnum = _lotto_db->head_block_num();

            FC_ASSERT(headnum >= BTS_LOTTO_BLOCKS_BEFORE_JACKPOTS_DRAW  + meta_ticket_out.out_idx.block_idx);

            // https://bitsharestalk.org/index.php?topic=4502.0
            uint64_t random_number = _lotto_db->fetch_blk_random_number(meta_ticket_out.out_idx.block_idx + BTS_LOTTO_BLOCKS_BEFORE_JACKPOTS_DRAW);

            uint64_t ticket_sale = my->_block2ticket_sale.fetch(meta_ticket_out.out_idx.block_idx);
            auto ticket = meta_ticket_out.ticket_out.ticket.as<betting_ticket>();
            
            FC_ASSERT(ticket.unit == get_asset_unit());
            FC_ASSERT(ticket.unit == meta_ticket_out.amount.unit);

            uint64_t ticket_lucky_number = ticket.lucky_number;
            uint16_t ticket_odds = ticket.odds;
            uint64_t ticket_amount = meta_ticket_out.amount.get_rounded_amount();
            uint64_t winners_count = /*Total ticket count*/ 10 / 3;    // global odds?

            uint64_t jackpot = 0;

            double house_edge = 0.01;

            // calcualte hash
            fc::sha256::encoder enc;
            enc.write((char*)&random_number, sizeof(random_number));
            enc.write((char*)&ticket.lucky_number, sizeof(ticket.lucky_number));
            uint64_t hash = enc.result()._hash[0];

            if (ticket_odds == 0)
            {
                return asset(0, meta_ticket_out.amount.unit);
            }

            // uint64_t range = (ticket_odds * ticket_sale) / (ticket_amount * winners_count);
            uint64_t range = (ticket_odds * ticket_sale);
            uint64_t scope = (ticket_amount * winners_count);

            // HASH(RANDOM_NUMBER+TICKET_LUCKY_NUMBER) % {TICKET_ODD * TICKET_SALE/ (TICKET_AMOUNT * X)} < 1 
            if (hash % range < scope)
            {
                // ((1 - E) * Ticket_Odd * TICKET_SALE) / X
                jackpot = (uint64_t)(((1 - house_edge) * ticket_odds * ticket_sale) / winners_count);
            }


            return asset(jackpot, meta_ticket_out.amount.unit);

        } FC_RETHROW_EXCEPTIONS(warn, "Error calculating jackpots for betting ticket ${m}", ("m", meta_ticket_out));
    }

    void betting_rule::store(const trx_block& blk, const signed_transactions& deterministic_trxs, const block_evaluation_state_ptr& state)
    {
        uint64_t ticket_sale = 0;

        for (auto trx : blk.trxs)
        {
            for (auto o : trx.outputs)
            {
                if (o.claim_func == claim_ticket) {
                    ticket_sale += o.amount.get_rounded_amount();
                }
            }
        }
            
        my->_block2ticket_sale.store(blk.block_num, ticket_sale);
    }

} } // bts::lotto