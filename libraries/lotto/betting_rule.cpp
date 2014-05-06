#pragma once
#include <bts/lotto/betting_rule.hpp>
#include <bts/lotto/lotto_config.hpp>

#include <bts/lotto/lotto_db.hpp>

#include <bts/db/level_map.hpp>

namespace bts {
    namespace lotto {
        namespace detail
        { 
            class betting_rule_impl            {            public:                betting_rule_impl(){}                bts::db::level_map<uint32_t, uint64_t>  _block2ticket_sale;                lotto_db* _db;            };
        }

        betting_rule::betting_rule(lotto_db* db)
            :my(new detail::betting_rule_impl())
        {
            my->_db = db;
        }

        betting_rule::~betting_rule()
        {

        }

        void betting_rule::open(const fc::path& dir, bool create)
        {
            try {
                my->_block2ticket_sale.open(dir / "betting_rule" / "_block2ticket_sale", create);
            } FC_RETHROW_EXCEPTIONS(warn, "Error loading betting rule database ${dir}", ("dir", dir)("create", create));
        }

        void betting_rule::close()
        {
            my->_block2ticket_sale.close();
        }

        asset betting_rule::jackpot_for_ticket(
            const bts::lotto::claim_ticket_output& ticket, const asset& amt, const output_index& out_idx)
        {
            // https://bitsharestalk.org/index.php?topic=4502.0
            uint64_t random_number = my->_db->fetch_blk_random_number(out_idx.block_idx + BTS_LOTTO_BLOCKS_BEFORE_JACKPOTS_DRAW);
            
            uint64_t ticket_sale = my->_block2ticket_sale.fetch(out_idx.block_idx);
            uint64_t ticket_lucky_number = ticket.lucky_number;
            uint16_t ticket_odds = ticket.odds;
            uint64_t ticket_amount = amt.get_rounded_amount();
            uint64_t winners_count = /*Total ticket count*/ 100 / 3;    // global odds?

            uint64_t jackpot = 0;

            double house_edge = 0.01;

            // calcualte hash
            fc::sha256::encoder enc;
            enc.write((char*)&random_number, sizeof(random_number));
            enc.write((char*)&ticket.lucky_number, sizeof(ticket.lucky_number));
            uint64_t hash = enc.result()._hash[0];

            uint64_t range = (ticket_odds * ticket_sale) / (ticket_amount * winners_count);

            // HASH(RANDOM_NUMBER+TICKET_LUCKY_NUMBER) % {TICKET_ODD * TICKET_SALE/ (TICKET_AMOUNT * X)} < 1 
            if (hash % range < 1)
            {
                // ((1 - E) * Ticket_Odd * TICKET_SALE) / X
                jackpot = (uint64_t)(((1 - house_edge) * ticket_odds * ticket_sale) / winners_count);
            }
            

            return asset(jackpot, amt.unit);
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

    }
} // bts::lotto