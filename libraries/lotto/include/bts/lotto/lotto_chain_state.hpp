#pragma once
#include <bts/blockchain/pending_chain_state.hpp>
#include <bts/lotto/lotto_db.hpp>

namespace bts {namespace lotto {
    using namespace bts::blockchain;
    class lotto_chain_state : public pending_chain_state
    {
        lotto_chain_state(lotto_db_ptr prev_state);
        // TODO: what is override?
        virtual ~lotto_chain_state() override;

        virtual void                  apply_deterministic_updates() override;

        virtual void                  apply_changes() const override;
    };
} } // bts::lotto