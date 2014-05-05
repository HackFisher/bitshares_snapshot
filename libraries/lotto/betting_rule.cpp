#pragma once
#include <bts/lotto/betting_rule.hpp>

namespace bts {
    namespace lotto {
        namespace detail
        { 
            class betting_rule_impl            {            public:                betting_rule_impl(){}            };
        }

        betting_rule::betting_rule()
        {

        }

        betting_rule::~betting_rule()
        {

        }

        uint64_t betting_rule::evaluate_total_jackpot(const uint64_t& winning_number, const uint64_t& ticket_sale, const uint64_t& target_block_num, const uint64_t& jackpot_pool)
        {
            return (ticket_sale + jackpot_pool) * 99 / 100;
        }

        uint64_t betting_rule::jackpot_for_ticket(const uint64_t& winning_number,
            const bts::lotto::claim_ticket_output& ticket, const uint64_t& amt, const uint64_t& total_jackpots)
        {
            // https://bitsharestalk.org/index.php?topic=4502.0
            uint64_t ticket_sale = total_jackpots; // TODO: Fix it.
            uint64_t ticket_lucky_number = ticket.lucky_number;
            uint16_t ticket_odds = ticket.odds;
            uint64_t ticket_amount = amt;
            uint64_t random_number = winning_number;
            uint64_t winners_count = /*Total ticket count*/ 100 / 3;    // global odds?

            uint64_t jackpot = 0;

            double house_edge = 0.01;

            // calcualte hash
            fc::sha256::encoder enc;
            enc.write((char*)&winning_number, sizeof(winning_number));
            enc.write((char*)&ticket.lucky_number, sizeof(ticket.lucky_number));
            uint64_t hash = enc.result()._hash[0];

            uint64_t range = (ticket_odds * ticket_sale) / (ticket_amount * winners_count);

            // HASH(RANDOM_NUMBER+TICKET_LUCKY_NUMBER) % {TICKET_ODD * TICKET_SALE/ (TICKET_AMOUNT * X)} < 1 
            if (hash % range < 1)
            {
                // ((1 - E) * Ticket_Odd * TICKET_SALE) / X
                jackpot = (uint64_t)(((1 - house_edge) * ticket_odds * ticket_sale) / winners_count);
            }
            

            return jackpot;
        }

    }
} // bts::lotto