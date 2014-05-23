#include <fc/reflect/variant.hpp>

#include <bts/db/level_map.hpp>
#include <bts/lotto/lotto_db.hpp>
#include <bts/lotto/dice_rule.hpp>
#include <bts/lotto/betting_rule.hpp>
#include <bts/lotto/lotto_rule.hpp>
#include <bts/lotto/lotto_config.hpp>
#include <bts/lotto/ticket_factory.hpp>

#include <bts/blockchain/operation_factory.hpp>

namespace bts { namespace lotto {

    namespace detail
    {
        class lotto_db_impl
        {
            public:
                lotto_db_impl(){}

                lotto_db*                                     _self;

                bts::db::level_map<uint32_t, block_summary>   _block2summary;

                std::unordered_map< ticket_type, rule_ptr > _rules;

                uint64_t generate_random_number(const uint32_t& block_num)
                {
                    auto random = fc::sha256();
                    for (uint32_t i = 1; i < BTS_LOTTO_BLOCKS_BEFORE_JACKPOTS_DRAW && (block_num >= i); ++i)
                    {
                        fc::sha512::encoder enc;
                        fc::raw::pack( enc, _self->get_block_header(block_num - i).previous_secret );
                        fc::raw::pack( enc, random );
                        random = fc::sha256::hash( enc.result() );
                    }

                    return (uint64_t)random._hash[0];
                }

                // TODO: Move to pending chain state...
                /*
                void store(const full_block& blk,
                    const signed_transactions& deterministic_trxs,
                    const block_evaluation_state_ptr& state)
                {
                    // TODO: ToFix: Should block's delegate id be retrieved this way? Then, how to achieve this before store?
                    // TODO: the default delegate id of first block is 0, this should influnce delgete2blocks.....
                    // real delegate is are larger than 0
                    if (blk.block_num > 0)
                    {
                        auto itr = _delegate2blocks.find(secret_op.delegate_id);
                        std::vector<uint32_t> block_ids;
                        if (itr.valid())
                        {
                            block_ids = itr.value();
                        }

                        block_ids.push_back(blk.block_num);
                        _delegate2blocks.store(secret_op.delegate_id, block_ids);
                    }

                    // TODO: change wining_number to sha256, and recheck whether sha356 is suitable for hashing.
                    uint64_t random_number = generate_random_number(blk.block_num);
                    // update block summary
                    block_summary bs;
                    bs.random_number = random_number;
                    wlog("block random number is ${r}", ("r", random_number));
                    _block2summary.store(blk.block_num, bs);

                    for (auto r : _rules)
                    {
                        r.second->store(blk, deterministic_trxs, state);
                    }
                }
            */
        };
    }

    lotto_db::lotto_db()
    :my( new detail::lotto_db_impl() )
    {
        // TODO: still can not register because bts::blockchain::operation_type_enum can not be extended
        //operation_factory::instance().register_operation<ticket_operation>();
        //operation_factory::instance().register_operation<jackpot_operation>();

        ticket_factory::instance().register_ticket<dice_ticket>();
        ticket_factory::instance().register_ticket<betting_ticket>();
        ticket_factory::instance().register_ticket<lottery_ticket>();
        //set_transaction_validator( std::make_shared<lotto_transaction_validator>(this) );

        my->_rules[dice_ticket::type] = std::make_shared<dice_rule>(this, dice_ticket::type, dice_ticket::unit);
        // betting rule using asset unit 1
        my->_rules[betting_ticket::type] = std::make_shared<betting_rule>(this, betting_ticket::type, betting_ticket::unit);
        // betting rule using asset unit 2
        my->_rules[lottery_ticket::type] = std::make_shared<lotto_rule>(this, lottery_ticket::type, lottery_ticket::unit);
        my->_self = this;
    }

    lotto_db::~lotto_db()
    {
    }

    void lotto_db::open( const fc::path& dir, fc::path genesis_file )
    {
        try {
            chain_database::open(dir, genesis_file);
            my->_block2summary.open( dir / "block2summary", true );
            for(auto r : my->_rules)
            {
                r.second->open(dir, true);
            }
        } FC_RETHROW_EXCEPTIONS( warn, "Error loading lotto database ${dir}", ("dir", dir) );
    }

    void lotto_db::close()
    {
        my->_block2summary.close();
        for(auto r : my->_rules)
        {
            r.second->close();
        }

        chain_database::close();
    }

    rule_ptr lotto_db::get_rule_ptr(const ticket_type& type)
    {
        return my->_rules[type];
    }

    transaction_evaluation_state_ptr lotto_db::evaluate_transaction(const signed_transaction& trx)
    {
        try {
            pending_chain_state_ptr          pend_state = std::make_shared<pending_chain_state>(shared_from_this());
            transaction_evaluation_state_ptr trx_eval_state = std::make_shared<lotto_trx_evaluation_state>(pend_state);

            trx_eval_state->evaluate(trx);

            return trx_eval_state;
        } FC_RETHROW_EXCEPTIONS(warn, "", ("trx", trx))

        // TODO
        /*
        auto inputs = _lotto_db->fetch_inputs(state.trx.inputs);
        for (auto in : inputs)
        {
            if (in.output.claim_func == claim_ticket)
            {
                // with assumption that all claim_tickets input are in deterministic trxs 
                evaluate_ticket_jackpot_transactions(state);
                // TODO: Do deterministic transactions need fees, votes, etc, or not? since after 100 block, is origin vote still available
                // To be synced with upstream: https://github.com/BitShares/bitshares_toolkit/issues/48
                break;
            }
        }*/
    }

    /**
     * Performs global validation of a block to make sure that no two transactions conflict. In
     * the case of the lotto only one transaction can claim the jackpot.
     */
    // Move to pending_chain_state
    /*
    block_evaluation_state_ptr lotto_db::validate(const full_block& blk, const signed_transactions& deterministic_trxs)
    {
        wlog("validate transactions: ${c}", ("c", blk.trxs));
        // TODO: In lotto deterministic_trxs do not validate again
        block_evaluation_state_ptr block_state = chain_database::validate(blk, deterministic_trxs);

        if (blk.block_num == 0) { return block_state; } // don't check anything for the genesis block;

        for (auto r : my->_rules)
        {
            // TODO: the parameters of the validate need to be changed, only validate rule sepecific tickets
            r.second->validate(blk, deterministic_trxs);
        }

        return block_state;
    }
    */

    /**
     *  Called after a block has been validated and appends
     *  it to the block chain storing all relevant transactions and updating the
     *  winning database.
     */
    /*
    void lotto_db::store(const full_block& blk, const signed_transactions& deterministic_trxs, const block_evaluation_state_ptr& state)
    {
        chain_database::store(blk, deterministic_trxs, state);

        my->store(blk, deterministic_trxs, state);
    }*/

    uint64_t lotto_db::fetch_blk_random_number( const uint32_t& k )
    { try {
        return my->_block2summary.fetch(k).random_number;
    } FC_RETHROW_EXCEPTIONS( warn, "fetching random number, block index ${k}", ("k",k) ) }

}} // bts::lotto
