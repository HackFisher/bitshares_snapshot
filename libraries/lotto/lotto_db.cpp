#include <fc/reflect/variant.hpp>

#include <bts/db/level_map.hpp>
#include <bts/blockchain/output_factory.hpp>
#include <bts/lotto/lotto_db.hpp>
#include <bts/lotto/lotto_rule_validator.hpp>
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

                // map drawning number to drawing record
                bts::db::level_map<uint32_t, drawing_record>  _drawing2record;
                bts::db::level_map<uint32_t, block_summary>   _block2summary;
                bts::db::level_map<uint32_t, std::vector<uint32_t>>   _delegate2blocks;
				bts::db::level_map<uint32_t, claim_secret_output> _block2secret;
                rule_validator_ptr                           _rule_validator;

                /**
                * @return <ticket transaction number, paid_jackpot for that ticket>
                */
                std::pair<trx_num, uint64_t> jackpot_paid_in_transaction(const signed_transaction& trx)
                {
                    uint64_t trx_paid = 0;
                    trx_num trx_n;

                    for (auto i : trx.inputs)
                    {
                        // TODO: Fee? There is no fee in deterministic trxs
                        auto o = _self->fetch_output(i.output_ref);
                        trx_n = _self->fetch_trx_num(i.output_ref.trx_hash);
                        if (o.claim_func == claim_ticket) {
                            for (auto out : trx.outputs)
                            {
                                if (out.claim_func == claim_jackpot) {

                                    trx_paid += out.amount.get_rounded_amount();
                                }
                            }

                            // TODO: Many ticket's inputs? Jackpots should draw by each ticket. 
                            break;
                        }
                    }

                    return std::pair<trx_num, uint64_t>(trx_n, trx_paid);
                }

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

                void store(const trx_block& blk,
                    const signed_transactions& deterministic_trxs,
                    const block_evaluation_state_ptr& state)
                {
                    claim_secret_output head_secret;

                    if (blk.block_num > 0)
                    {
                        head_secret = blk.trxs[0].outputs[0].as<claim_secret_output>();
                    }

                    // TODO: the default delegate id of first block is 0, this could influnce delgete2blocks.....
                    _block2secret.store(blk.block_num, head_secret);

                    // TODO: ToFix: Should block's delegate id be retrieved this way? Then, how to achieve this before store?
                    // TODO: the default delegate id of first block is 0, this should influnce delgete2blocks.....
                    // real delegate is are larger than 0
                    if (blk.block_num > 0)
                    {
                        auto itr = _delegate2blocks.find(head_secret.delegate_id);
                        std::vector<uint32_t> block_ids;
                        if (itr.valid())
                        {
                            block_ids = itr.value();
                        }

                        block_ids.push_back(blk.block_num);
                        _delegate2blocks.store(head_secret.delegate_id, block_ids);
                    }

                    uint64_t amout_won = 0;

                    // TODO: ticket->jackpot is only allowed in deterministic_trxs
                    for (auto trx : deterministic_trxs)
                    {
                        auto trx_num_paid = jackpot_paid_in_transaction(trx);
                        if (trx_num_paid.second > 0)
                        {
                            auto winning_block_num = trx_num_paid.first.block_num + BTS_LOTTO_BLOCKS_BEFORE_JACKPOTS_DRAW;
                            amout_won += trx_num_paid.second;
                            // The in.output should all be tickets, and the out should all be jackpots
                            auto draw_record = _drawing2record.fetch(winning_block_num);
                            draw_record.total_paid = draw_record.total_paid + trx_num_paid.second;
                            _drawing2record.store(winning_block_num, draw_record);
                        }
                    }

                    // update block summary
                    block_summary bs;
                    uint64_t ticket_sales = 0;

                    for (auto trx : blk.trxs)
                    {
                        for (auto o : trx.outputs)
                        {
                            if (o.claim_func == claim_ticket) {
                                ticket_sales += o.amount.get_rounded_amount();
                            }
                        }
                    }
                    bs.ticket_sales = ticket_sales;
                    bs.amount_won = amout_won;

                    // TODO: change wining_number to sha256, and recheck whether sha356 is suitable for hashing.
                    uint64_t random_number = generate_random_number(blk.block_num, head_secret.revealed_secret.str());
                    bs.winning_number = random_number;
                    wlog("winning number is ${r}", ("r", random_number));

                    _block2summary.store(blk.block_num, bs);

                    // update jackpots.....
                    // the drawing record in this block, corresponding to previous related ticket purchase block (blk.block_num - BTS_LOTTO_BLOCKS_BEFORE_JACKPOTS_DRAW)
                    uint64_t last_jackpot_pool = 0;
                    if (blk.block_num > 0)
                    {
                        last_jackpot_pool = _drawing2record.fetch(blk.block_num - 1).jackpot_pool;
                    }
                    drawing_record dr;
                    dr.total_jackpot = _rule_validator->evaluate_total_jackpot(bs.winning_number, blk.block_num, last_jackpot_pool + bs.ticket_sales);
                    // just the begin, still not paid
                    dr.total_paid = 0;
                    dr.jackpot_pool = last_jackpot_pool + bs.ticket_sales - dr.total_jackpot;
                    // TODO: how to move to validate()
                    FC_ASSERT(dr.jackpot_pool >= 0, "jackpot is out ...");
                    // Assert that the jackpot_pool is large than the sum ticket sales of blk.block_num - 99 , blk.block_num - 98 ... blk.block_num.
                    _drawing2record.store(blk.block_num, dr);
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
		set_rule_validator(std::make_shared<rule_validator>(this));
        my->_self = this;
    }

    lotto_db::~lotto_db()
    {
    }

    void lotto_db::open( const fc::path& dir, bool create )
    {
        try {
            chain_database::open( dir, create );
            my->_drawing2record.open( dir / "drawing2record", create );
            my->_block2summary.open( dir / "block2summary", create );
            my->_delegate2blocks.open(dir / "delegate2blocks", create);
            my->_block2secret.open( dir / "block2secret", create );
        } FC_RETHROW_EXCEPTIONS( warn, "Error loading domain database ${dir}", ("dir", dir)("create", create) );
    }

    void lotto_db::close() 
    {
        my->_drawing2record.close();
        my->_block2summary.close();
        my->_delegate2blocks.close();
        my->_block2secret.close();

        chain_database::close();
    }

	void lotto_db::set_rule_validator( const rule_validator_ptr& v )
    {
       my->_rule_validator = v;
    }

	void validate_secret_transactions(const signed_transactions& deterministic_trxs, const signed_transactions& trxs)
	{
		std::unordered_set<output_reference> ref_outs;
		for (const signed_transaction& trx : deterministic_trxs)
		{
			for (auto out : trx.outputs)
			{
				FC_ASSERT(out.claim_func != claim_secret, "secret output is no allowed in deterministic transactions");
			}
		}
		for (size_t i = 0; i < trxs.size(); i++)
		{
			if (i == 0)
			{
				FC_ASSERT(trxs[0].inputs.size() == 0, "The size of claim secret inputs should be zero.");
				FC_ASSERT(trxs[0].outputs.size() == 1, "The size of claim secret outputs should be one.");

				FC_ASSERT(trxs[0].outputs[0].claim_func == claim_secret, "The first transaction in this block should be claim secret.");
			}
			else
			{
				for (auto out : trxs[i].outputs)
				{
					FC_ASSERT(out.claim_func != claim_secret, "secret output is no allowed in transactions except the first one");
				}
			}
		}
	}

    /**
     * TODO: this method should be const, return should be always same with same params
     */
	asset lotto_db::draw_jackpot_for_ticket( output_index out_idx, uint64_t lucky_number, uint16_t odds, asset amount )
    {
        FC_ASSERT(head_block_num() >= out_idx.block_idx + BTS_LOTTO_BLOCKS_BEFORE_JACKPOTS_DRAW);
		// fc::sha256 winning_number;
		// using the next block generated block number
        uint64_t winning_number = my->_block2summary.fetch(out_idx.block_idx + BTS_LOTTO_BLOCKS_BEFORE_JACKPOTS_DRAW).winning_number;
		
        // TODO: what's global_odds, ignore currenly.
		uint64_t global_odds = 0;

		auto dr = my->_drawing2record.fetch(out_idx.block_idx + BTS_LOTTO_BLOCKS_BEFORE_JACKPOTS_DRAW);
        
        // TODO: pass asset inside?
        uint64_t jackpot =  my->_rule_validator->evaluate_jackpot(winning_number, lucky_number, odds, amount.get_rounded_amount(), dr.total_jackpot);

        return asset(jackpot, amount.unit);
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
        wlog("ticket_blk is: ${c} ", ("c", ticket_blk));
        for (auto trx : ticket_blk.trxs)
        {
            for (size_t i = 0; i < trx.outputs.size(); i ++ )
            {
                auto out = trx.outputs[i];
                // TODO: spent should not happen, e.g. in current/before block's trxs
                // All ticket are drawn in deterministic way
                wlog("ticket out is: ${c} ", ("c", out));
                if (out.claim_func == claim_ticket /*TODO: && !in.meta_output.is_spent()*/)
                {
                    signed_transaction draw_trx;
                    draw_trx.vote = 0; // TODO: no vote
                    auto ticket_out = out.as<claim_ticket_output>();
                    auto trx_num = fetch_trx_num(trx.id());
                    wlog("ticket trx_num is: ${c} ", ("c", trx_num));
                    // TODO: why not directly send in.output in to 
                    auto jackpot = draw_jackpot_for_ticket(output_index(trx_num.block_num, trx_num.trx_idx, i),
                        ticket_out.lucky_number, ticket_out.odds, out.amount);
                    
                    wlog("jackpot drawn is: ${c} ", ("c", jackpot));
                    uint16_t mature = 0;
                    while (jackpot.get_rounded_amount() > BTS_LOTTO_RULE_MAXIMUM_REWARDS_EACH_JACKPOT_OUTPUT)
                    {
                        wlog("mature day is: ${c} ", ("c", mature));
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
        // TODO: In lotto deterministic_trxs do not validate again
        block_evaluation_state_ptr block_state = chain_database::validate(blk, deterministic_trxs);

		if (blk.block_num == 0) { return block_state; } // don't check anything for the genesis block;

		// At least contain the claim_secret trx
        FC_ASSERT(blk.trxs.size() > 0);
		validate_secret_transactions(deterministic_trxs, blk.trxs);

        auto secret_out = blk.trxs[0].outputs[0].as<claim_secret_output>();

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

        for (const signed_transaction& trx : deterministic_trxs)
        {
            auto trx_num_paid = my->jackpot_paid_in_transaction(trx);

            if (trx_num_paid.second > 0)
            {
                auto winning_block_num = trx_num_paid.first.block_num + BTS_LOTTO_BLOCKS_BEFORE_JACKPOTS_DRAW;
                // The in.output should all be tickets, and the out should all be jackpots
                auto draw_record = my->_drawing2record.fetch(winning_block_num);
                FC_ASSERT(draw_record.total_paid + trx_num_paid.second <= draw_record.total_jackpot, "The paid jackpots is out of the total jackpot.");
            }
        }

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

}} // bts::lotto
