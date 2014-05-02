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
                // map drawning number to drawing record
                bts::db::level_map<uint32_t, drawing_record>  _drawing2record;
                bts::db::level_map<uint32_t, block_summary>   _block2summary;
                bts::db::level_map<uint32_t, std::vector<uint32_t>>   _delegate2blocks;
				bts::db::level_map<uint32_t, claim_secret_output> _block2secret;
				rule_validator_ptr                           _rule_validator;
            
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
        FC_ASSERT(head_block_num() - out_idx.block_idx > BTS_LOTTO_BLOCKS_BEFORE_JACKPOTS_DRAW);
		// fc::sha256 winning_number;
		// using the next block generated block number
        uint64_t winning_number = my->_block2summary.fetch(out_idx.block_idx + BTS_LOTTO_BLOCKS_BEFORE_JACKPOTS_DRAW).winning_number;
		
        // TODO: what's global_odds, ignore currenly.
		uint64_t global_odds = 0;

		auto dr = my->_drawing2record.fetch(out_idx.block_idx + BTS_LOTTO_BLOCKS_BEFORE_JACKPOTS_DRAW);
        
        uint64_t jackpot =  my->_rule_validator->evaluate_jackpot(winning_number, lucky_number, dr.total_jackpot);

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
        for (auto trx : ticket_blk.trxs)
        {
            auto inputs = fetch_inputs(trx.inputs);
            for (auto in : inputs)
            {
                // TODO: spent should not happen, e.g. in current/before block's trxs
                // All ticket are drawn in deterministic way
                if (in.output.claim_func == claim_ticket && !in.meta_output.is_spent())
                {
                    signed_transaction draw_trx;
                    auto ticket_out = in.output.as<claim_ticket_output>();
                    auto trx_num = fetch_trx_num(trx.id());
                    // TODO: why not directly send in.output in to 
                    auto jackpot = draw_jackpot_for_ticket(output_index(trx_num.block_num, trx_num.trx_idx, in.output_num),
                        ticket_out.lucky_number, ticket_out.odds, in.output.amount);
                    
                    uint16_t mature = 0;
                    while (jackpot.get_rounded_amount() > BTS_LOTTO_RULE_MAXIMUM_REWARDS_EACH_JACKPOT_OUTPUT)
                    {
                        claim_jackpot_output jackpot_out(ticket_out.owner, mature);
                        asset amt(BTS_LOTTO_RULE_MAXIMUM_REWARDS_EACH_JACKPOT_OUTPUT, jackpot.unit);
                        draw_trx.outputs.push_back( trx_output( jackpot_out, amt ) );
                        jackpot -= amt;
                        ++ mature;
                    }

                    draw_trx.outputs.push_back(trx_output(claim_jackpot_output(ticket_out.owner, mature), jackpot));

                    signed_trxs.push_back(draw_trx);
                }
            }

        }

        return signed_trxs;
    }

    /**
     * Performs global validation of a block to make sure that no two transactions conflict. In
     * the case of the lotto only one transaction can claim the jackpot.
     */
    block_evaluation_state_ptr lotto_db::validate( const trx_block& blk, const signed_transactions& deterministic_trxs )
    {
		block_evaluation_state_ptr block_state = chain_database::validate(blk, deterministic_trxs);

		if (blk.block_num == 0) { return block_state; } // don't check anything for the genesis block;

		// At least contain the claim_secret trx
		FC_ASSERT(deterministic_trxs.size() > 0);

		validate_secret_transactions(deterministic_trxs, blk.trxs);

		auto secret_out = deterministic_trxs[0].outputs[0].as<claim_secret_output>();

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
        
        for (const signed_transaction& trx : blk.trxs)
        {
            for ( auto i : trx.inputs)
            {
                // TODO: need to remove fees?
				auto o = fetch_output(i.output_ref);
                auto draw_block_num = fetch_trx_num(i.output_ref.trx_hash).block_num + BTS_LOTTO_BLOCKS_BEFORE_JACKPOTS_DRAW;
                uint64_t trx_paid = 0;
                if (o.claim_func == claim_ticket) {
                    // all the tickets drawing in this trx should belong to the same blocks.
                    for ( auto out : trx.outputs)
                    {
						if (out.claim_func == claim_by_signature) {
                            trx_paid += out.amount.get_rounded_amount();
                        }
                    }
					// The in.output should all be tickets, and the out should all be jackpots
                    auto draw_record = my->_drawing2record.fetch(draw_block_num);
                    FC_ASSERT(draw_record.total_paid + trx_paid <= draw_record.total_jackpot, "The paid jackpots is out of the total jackpot.");
					break;
                }
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

		claim_secret_output head_secret;

		if (blk.block_num > 0)
		{
			head_secret = deterministic_trxs[0].outputs[0].as<claim_secret_output>();
		}

		// TODO: the default delegate id of first block is 0, this could influnce delgete2blocks.....
		my->_block2secret.store(blk.block_num, head_secret);

        auto random = fc::sha256::hash(head_secret.revealed_secret.str());
        for (uint32_t i = 1; i < 100 && (blk.block_num >= i); ++i)
        {
            auto history_secret = my->_block2secret.fetch(blk.block_num - i);
            random = fc::sha256::hash(history_secret.revealed_secret.str() + random.str()); // where + is concat
        }
        // TODO: ToFix: Should block's delegate id be retrieved this way? Then, how to achieve this before store?
        // TODO: the default delegate id of first block is 0, this could influnce delgete2blocks.....
        if (blk.block_num > 0)
        {
            auto itr = my->_delegate2blocks.find(head_secret.delegate_id);
            if (itr.valid())
            {
                auto block_ids = itr.value();
                block_ids.push_back(blk.block_num);
                my->_delegate2blocks.store(head_secret.delegate_id, block_ids);
            }
        }

        block_summary bs;
        uint64_t ticket_sales = 0;
        uint64_t amout_won = 0;
        for (const signed_transaction& trx : blk.trxs)
        {
            for ( auto o : trx.outputs)
            {
                if (o.claim_func == claim_ticket) {
                    ticket_sales += o.amount.get_rounded_amount();
                }
            }

            for ( auto i : trx.inputs)
            {
                // TODO: need to remove fees?
				auto o = fetch_output(i.output_ref);
                auto draw_block_num = fetch_trx_num(i.output_ref.trx_hash).block_num + BTS_LOTTO_BLOCKS_BEFORE_JACKPOTS_DRAW;
                uint64_t trx_paid = 0;
                if (o.claim_func == claim_ticket) {
                    // all the tickets drawing in this trx should belong to the same blocks.
                    for ( auto out : trx.outputs)
                    {
						if (out.claim_func == claim_by_signature) {
                            amout_won += out.amount.get_rounded_amount();
                            trx_paid += out.amount.get_rounded_amount();
                        }
                    }
					// The in.output should all be tickets, and the out should all be jackpots
                    auto draw_record = my->_drawing2record.fetch(draw_block_num);
                    draw_record.total_paid = draw_record.total_paid + trx_paid;
                    my->_drawing2record.store(draw_block_num, draw_record);
					break;
                }
            }
        }
        bs.ticket_sales = ticket_sales;
        bs.amount_won = amout_won;

        // TODO: change wining_number to sha256, and recheck whether sha356 is suitable for hashing.
        bs.winning_number = ((uint64_t)random._hash[0]) <<32 & ((uint64_t)random._hash[0]);
        my->_block2summary.store(blk.block_num, bs);
        // the drawing record in this block, corresponding to previous related ticket purchase block (blk.block_num - BTS_LOTTO_BLOCKS_BEFORE_JACKPOTS_DRAW)
        uint64_t last_jackpot_pool = 0;
        if (blk.block_num > 0)
        {
            last_jackpot_pool = my->_drawing2record.fetch(blk.block_num - 1).jackpot_pool;
        }
        drawing_record dr;
        dr.total_jackpot = my->_rule_validator->evaluate_total_jackpot(bs.winning_number, blk.block_num, last_jackpot_pool + bs.ticket_sales);
        // just the begin, still not paid
        dr.total_paid = 0;
        dr.jackpot_pool = last_jackpot_pool + bs.ticket_sales - dr.total_jackpot;
        // TODO: how to move to validate()
        FC_ASSERT(dr.jackpot_pool >= 0, "jackpot is out ...");
        // Assert that the jackpot_pool is large than the sum ticket sales of blk.block_num - 99 , blk.block_num - 98 ... blk.block_num.
        my->_drawing2record.store(blk.block_num, dr);
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
