#pragma once
#include <bts/blockchain/transaction.hpp>
#include <bts/lotto/lotto_outputs.hpp>
#include <fc/reflect/variant.hpp>

namespace bts { namespace lotto {
using namespace bts::blockchain;


class lotto_trx_evaluation_state : public bts::blockchain::transaction_evaluation_state
{
    public:
        lotto_trx_evaluation_state(const chain_interface_ptr& blockchain);

        lotto_trx_evaluation_state(){};

        virtual ~lotto_trx_evaluation_state();

        uint64_t total_ticket_sales;
        uint64_t ticket_winnings;

        // TODO: void evaluate_ticket_jackpot_transactions(lotto_trx_evaluation_state& state);
        virtual void evaluate_operation(const operation& op);

        virtual void evaluate_secret(const secret_operation& op);
        virtual void evaluate_ticket(const ticket_operation& op);
        virtual void evaluate_jackpot(const jackpot_operation& op);
};
}} // bts::lotto

