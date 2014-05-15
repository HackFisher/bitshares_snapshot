#pragma once

#include <fc/reflect/variant.hpp>
#include <fc/reflect/reflect.hpp>

#include <bts/blockchain/asset.hpp>
#include <bts/blockchain/transaction.hpp>
#include <bts/blockchain/chain_database.hpp>
#include <bts/wallet/wallet.hpp>
#include <bts/lotto/lotto_operations.hpp>

namespace bts { namespace lotto {
    using namespace bts::wallet;

    class lotto_db;

    struct ticket_index
    {
        ticket_index(){}

        ticket_index(const transaction_id_type& t, const uint32_t& k)
            :trx_id(t), ticket_num(k){}

        transaction_id_type trx_id;
        uint32_t ticket_num;
    };

    /**
     *  @class meta_trx_input
     *
     *  Caches ticket output information used by rules while
     *  evaluating a ticket.
     */
    struct meta_ticket_output
    {
       meta_ticket_output(){}

       meta_ticket_output(transaction_id_type t, uint16_t output_id, ticket_operation o, asset a)
           : out_idx(std::move(t), output_id), ticket_op(std::move(o)), amount(std::move(a))
       {}

       ticket_index         out_idx;
       ticket_operation     ticket_op;
       asset                amount;
    };

    namespace detail
    {
        class rule_impl;
    }
    /**
    *  @class rule
    *  @brief base class of rule layer, to define rule specific things.
    */
    class rule
    {
        public:
            rule(lotto_db* db, ticket_type t, asset_id_type u);
            virtual ~rule();

            virtual void                  open( const fc::path& dir, bool create = true );
            virtual void                  close();

            virtual asset jackpot_payout(const meta_ticket_output& meta_ticket_out) = 0;

            //virtual void validate(const full_block& blk, const signed_transactions& deterministic_trxs);

            //virtual void store(const full_block& blk, const signed_transactions& deterministic_trxs, const block_evaluation_state_ptr& state);

            ticket_type get_ticket_type()
            {
                return _ticket_type;
            }

            asset_id_type get_asset_unit()
            {
                return _unit_type;
            }

        protected:
            lotto_db*           _lotto_db;
            ticket_type         _ticket_type;
            asset_id_type       _unit_type;

        private:
            std::unique_ptr<detail::rule_impl> my;
    };
    typedef std::shared_ptr<rule> rule_ptr;
} } // bts::lotto

FC_REFLECT(bts::lotto::ticket_index, (trx_id)(ticket_num))
FC_REFLECT(bts::lotto::meta_ticket_output, (out_idx)(ticket_op)(amount))

