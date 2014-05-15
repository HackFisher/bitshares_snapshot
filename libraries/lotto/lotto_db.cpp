#include <fc/reflect/variant.hpp>

#include <bts/db/level_map.hpp>
#include <bts/lotto/lotto_db.hpp>
#include <bts/lotto/dice_rule.hpp>
#include <bts/lotto/betting_rule.hpp>
#include <bts/lotto/lotto_rule.hpp>
#include <bts/lotto/lotto_config.hpp>
#include <bts/lotto/ticket_factory.hpp>

namespace fc {
    template<> struct get_typename<std::vector<uint32_t>>        { static const char* name()  { return "std::vector<uint32_t>"; } };
} // namespace fc

namespace bts { namespace lotto {

    namespace detail
    {
        class lotto_db_impl
        {
            public:
                lotto_db_impl(){}

                lotto_db*                                     _self;

                bts::db::level_map<uint32_t, block_summary>   _block2summary;
                bts::db::level_map<uint32_t, std::vector<uint32_t>>   _delegate2blocks;
                bts::db::level_map<uint32_t, secret_operation> _block2secret;

                std::unordered_map< ticket_type, rule_ptr > _rules;

                uint64_t generate_random_number(const uint32_t& block_num, const fc::string& revealed_secret)
                {
                    auto random = fc::sha256::hash(revealed_secret);
                    for (uint32_t i = 1; i < BTS_LOTTO_BLOCKS_BEFORE_JACKPOTS_DRAW && (block_num >= i); ++i)
                    {
                        auto history_secret = _block2secret.fetch(block_num - i);
                        // where + is concat
                        random = fc::sha256::hash(history_secret.revealed_secret.str() + random.str());
                    }

                    return (uint64_t)random._hash[0];
                }

                secret_operation get_secret_output(const full_block& blk)
                {
                    for (size_t i = 0; i < blk.user_transactions.size(); i++)
                    {
                        for (size_t j = 0; j < blk.user_transactions[i].operations.size(); j++)
                        {
                            if (blk.user_transactions[i].operations[j].type.value == secret_op_type) {
                                return blk.user_transactions[i].operations[j].as<secret_operation>();
                            }
                        }
                    }

                    FC_ASSERT(false, "Should not happen, validated");
                    return secret_operation();
                }

                // TODO: Move to pending chain state...
                /*
                void store(const full_block& blk,
                    const signed_transactions& deterministic_trxs,
                    const block_evaluation_state_ptr& state)
                {
                    secret_operation secret_op;

                    if (blk.block_num > 0)
                    {
                        secret_op = get_secret_output(blk);
                    }

                    // TODO: the default delegate id of first block is 0, this could influnce delgete2blocks.....
                    _block2secret.store(blk.block_num, secret_op);

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
                    uint64_t random_number = generate_random_number(blk.block_num, secret_op.revealed_secret.str());
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
        /* TODO:
        output_factory::instance().register_output<claim_secret_output>();
        output_factory::instance().register_output<claim_ticket_output>();
        output_factory::instance().register_output<claim_jackpot_output>();
        */

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

    void lotto_db::open( const fc::path& dir )
    {
        try {
            chain_database::open( dir );
            my->_block2summary.open( dir / "block2summary", true );
            my->_delegate2blocks.open(dir / "delegate2blocks", true);
            my->_block2secret.open(dir / "block2secret", true);
            for(auto r : my->_rules)
            {
                r.second->open(dir, true);
            }
        } FC_RETHROW_EXCEPTIONS( warn, "Error loading lotto database ${dir}", ("dir", dir) );
    }

    void lotto_db::close() 
    {
        my->_block2summary.close();
        my->_delegate2blocks.close();
        my->_block2secret.close();
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

    void lotto_db::validate_secret_transactions(const signed_transactions& deterministic_trxs, const full_block& blk)
    {
        /*
        for (const signed_transaction& trx : deterministic_trxs)
        {
            for (auto out : trx.outputs)
            {
                FC_ASSERT(out.claim_func != claim_secret, "secret output is no allowed in deterministic transactions");
            }
        }

        bool found_secret_out = false;
        for (size_t i = 0; i < blk.trxs.size(); i++)
        {
            for (size_t j = 0; j < blk.trxs[i].outputs.size(); j++)
            {
                if (blk.trxs[i].outputs[j].claim_func == claim_secret) {
                    auto secret_out = blk.trxs[i].outputs[j].as<claim_secret_output>();
                    FC_ASSERT(found_secret_out == false, "There can only one secret out be allowe in each block.");
                    lotto_trx_evaluation_state state(blk.trxs[i]);
                    FC_ASSERT(state.has_signature(address(blk.signee())), "", ("owner", address(blk.signee()))("sigs", state.sigs));
                    
                    auto provided_delegate = lookup_delegate(secret_out.delegate_id);
                    FC_ASSERT(!!provided_delegate, "unable to find provided delegate id ${id}", ("id", secret_out.delegate_id));
                    // TODO: validate that the provided delegate is the block signee and scheduled delegate
                    // see chain_database::validate
                    if (blk.signee() == get_trustee())
                    {
                        // TODO: if this block is signed by trusee, then do not need to validate, just to trust, but this will be changed later
                    }
                    else
                    {
                        FC_ASSERT(provided_delegate->owner == blk.signee(),
                            "provided delegate is not same with the signee delegate of the block.", ("provided_del_key", provided_delegate->owner)("block_signee", blk.signee()));
                    }
                    

                    //Add fees, so the following two requirements are removed.
                    //FC_ASSERT(trxs[i].inputs.size() == 0, "The size of claim secret inputs should be zero.");
                    //FC_ASSERT(trxs[0].outputs.size() == 1, "The size of claim secret outputs should be one.");
                    found_secret_out = true;
                }
            }
        }*/

        //FC_ASSERT(found_secret_out == true, "There is no secret out found.");
    }

    // TODO: Move to pending_chain_state
    /*
    signed_transactions lotto_db::generate_deterministic_transactions()
    {
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

        wlog("generate_deterministic_transactions: ${c}", ("c", signed_trxs));
        return signed_trxs;
    }
    */

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

        // At least contain the claim_secret trx
        FC_ASSERT(blk.trxs.size() > 0);
        validate_secret_transactions(deterministic_trxs, blk);

        auto secret_out = my->get_secret_output(blk);

        auto itr = my->_delegate2blocks.find(secret_out.delegate_id);

        if( itr.valid() )
        {
            auto block_ids = itr.value();

            // // Get the last secret produced by this delegate
            auto last_secret = my->_block2secret.fetch(block_ids[block_ids.size() - 1]);

            // TODO: reviewing the hash.
            FC_ASSERT(fc::sha256::hash(secret_out.revealed_secret) == last_secret.secret);
        } else {
            FC_ASSERT(secret_out.revealed_secret == fc::sha256());   //  this is the first block produced by delegate
        }

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

    bool lotto_db::is_new_delegate(const uint32_t& d)
    {
        try {
            return !my->_delegate2blocks.find(d).valid();
        } FC_RETHROW_EXCEPTIONS(warn, "finding block indexes, delegate id ${d}", ("d", d))
    }

    std::vector<uint32_t>   lotto_db::fetch_blocks_idxs(const uint32_t& d)
    {
        try {
            return my->_delegate2blocks.fetch(d);
        } FC_RETHROW_EXCEPTIONS(warn, "fetching block indexes, delegate id ${d}", ("d", d))
    }

    secret_operation     lotto_db::fetch_secret(const uint32_t& b)
    {
        try {
            return my->_block2secret.fetch(b);
        } FC_RETHROW_EXCEPTIONS(warn, "fetching block secret, block index ${b}", ("b", b))
    }

}} // bts::lotto
