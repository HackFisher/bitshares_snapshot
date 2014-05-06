#include <fc/reflect/variant.hpp>

#include <bts/db/level_map.hpp>
#include <bts/blockchain/output_factory.hpp>
#include <bts/lotto/lotto_db.hpp>
#include <bts/lotto/betting_rule.hpp>
#include <bts/lotto/lotto_rule.hpp>
#include <bts/lotto/lotto_config.hpp>

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
                bts::db::level_map<uint32_t, claim_secret_output> _block2secret;

                rule_ptr                                        _rule_ptr;

                uint64_t generate_random_number(const uint32_t& block_num, const fc::string& revealed_secret)
                {
                    auto random = fc::sha256::hash(revealed_secret);
                    for (uint32_t i = 1; i < 100 && (block_num >= i); ++i)
                    {
                        auto history_secret = _block2secret.fetch(block_num - i);
                        // where + is concat
                        random = fc::sha256::hash(history_secret.revealed_secret.str() + random.str());
                    }

                    return (uint64_t)random._hash[0];
                }

                claim_secret_output get_secret_output(const trx_block& blk)
                {
                    for (size_t i = 0; i < blk.trxs.size(); i++)
                    {
                        for (size_t j = 0; j < blk.trxs[i].outputs.size(); j++)
                        {
                            if (blk.trxs[i].outputs[j].claim_func == claim_secret) {
                                return blk.trxs[i].outputs[j].as<claim_secret_output>();
                            }
                        }
                    }

                    FC_ASSERT(false, "Should not happen, validated");
                    return claim_secret_output();
                }

                void store(const trx_block& blk,
                    const signed_transactions& deterministic_trxs,
                    const block_evaluation_state_ptr& state)
                {
                    claim_secret_output secret_out;

                    if (blk.block_num > 0)
                    {
                        secret_out = get_secret_output(blk);
                    }

                    // TODO: the default delegate id of first block is 0, this could influnce delgete2blocks.....
                    _block2secret.store(blk.block_num, secret_out);

                    // TODO: ToFix: Should block's delegate id be retrieved this way? Then, how to achieve this before store?
                    // TODO: the default delegate id of first block is 0, this should influnce delgete2blocks.....
                    // real delegate is are larger than 0
                    if (blk.block_num > 0)
                    {
                        auto itr = _delegate2blocks.find(secret_out.delegate_id);
                        std::vector<uint32_t> block_ids;
                        if (itr.valid())
                        {
                            block_ids = itr.value();
                        }

                        block_ids.push_back(blk.block_num);
                        _delegate2blocks.store(secret_out.delegate_id, block_ids);
                    }

                    // TODO: change wining_number to sha256, and recheck whether sha356 is suitable for hashing.
                    uint64_t random_number = generate_random_number(blk.block_num, secret_out.revealed_secret.str());
                    // update block summary
                    block_summary bs;
                    bs.random_number = random_number;
                    wlog("block random number is ${r}", ("r", random_number));
                    _block2summary.store(blk.block_num, bs);

                    _rule_ptr->store(blk, deterministic_trxs, state);
                }
            
        };
    }

    lotto_db::lotto_db()
    :my( new detail::lotto_db_impl() )
    {
        output_factory::instance().register_output<claim_secret_output>();
        output_factory::instance().register_output<claim_ticket_output>();
        output_factory::instance().register_output<claim_jackpot_output>();
        set_transaction_validator( std::make_shared<lotto_transaction_validator>(this) );
        // my->_rule_ptr = std::make_shared<lotto_rule>(this);
        my->_rule_ptr = std::make_shared<betting_rule>(this);
        my->_self = this;
    }

    lotto_db::~lotto_db()
    {
    }

    void lotto_db::open( const fc::path& dir, bool create )
    {
        try {
            chain_database::open( dir, create );
            my->_block2summary.open( dir / "block2summary", create );
            my->_delegate2blocks.open(dir / "delegate2blocks", create);
            my->_block2secret.open( dir / "block2secret", create );
            my->_rule_ptr->open(dir, create);
        } FC_RETHROW_EXCEPTIONS( warn, "Error loading lotto database ${dir}", ("dir", dir)("create", create) );
    }

    void lotto_db::close() 
    {
        my->_block2summary.close();
        my->_delegate2blocks.close();
        my->_block2secret.close();
        my->_rule_ptr->close();

        chain_database::close();
    }

    rule_ptr lotto_db::get_rule_ptr()
    {
        return my->_rule_ptr;
    }

    void lotto_db::validate_secret_transactions(const signed_transactions& deterministic_trxs, const trx_block& blk)
    {
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
        }

        FC_ASSERT(found_secret_out == true, "There is no secret out found.");
    }

    signed_transactions lotto_db::generate_deterministic_transactions()
    {
        signed_transactions signed_trxs = chain_database::generate_deterministic_transactions();

        // being in genesis block pushing
        if (head_block_num() == trx_num::invalid_block_num)
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
                    auto jackpot = my->_rule_ptr->jackpot_for_ticket(
                        ticket_out, out.amount, output_index(trx_num.block_num, trx_num.trx_idx, i));
                    
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

    /**
     * Performs global validation of a block to make sure that no two transactions conflict. In
     * the case of the lotto only one transaction can claim the jackpot.
     */
    block_evaluation_state_ptr lotto_db::validate( const trx_block& blk, const signed_transactions& deterministic_trxs )
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

        my->_rule_ptr->validate(blk, deterministic_trxs);

        return block_state;
    }

    /** 
     *  Called after a block has been validated and appends
     *  it to the block chain storing all relevant transactions and updating the
     *  winning database.
     */
    void lotto_db::store( const trx_block& blk, const signed_transactions& deterministic_trxs, const block_evaluation_state_ptr& state )
    {
        chain_database::store(blk, deterministic_trxs, state);

        my->store(blk, deterministic_trxs, state);
    }

    /**
     * When a block is popped from the chain, this method implements the necessary code
     * to revert the blockchain database to the proper state.
     */
    trx_block lotto_db::pop_block()
    {
       auto blk = chain_database::pop_block();
       // TODO: remove block summary from DB
       FC_ASSERT( !"Not Implemented" );
       return blk;
    }

    uint64_t lotto_db::fetch_blk_random_number( const uint32_t& k )
    { try {
        return my->_block2summary.fetch(k).random_number;
    } FC_RETHROW_EXCEPTIONS( warn, "fetching random number, block index ${k}", ("k",k) ) }

}} // bts::lotto
