
#include <bts/db/level_map.hpp>
#include <bts/lotto/lotto_db.hpp>
#include <fc/reflect/variant.hpp>
#include <bts/lotto/lotto_rule_validator.hpp>
#include <bts/lotto/lotto_block.hpp>
#include <bts/lotto/lotto_config.hpp>

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
				rule_validator_ptr                           _rule_validator;
            
        };
    }

    lotto_db::lotto_db()
    :my( new detail::lotto_db_impl() )
    {
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
        } FC_RETHROW_EXCEPTIONS( warn, "Error loading domain database ${dir}", ("dir", dir)("create", create) );
    }

    void lotto_db::close() 
    {
        my->_drawing2record.close();
        my->_block2summary.close();
    }

	void lotto_db::set_rule_validator( const rule_validator_ptr& v )
    {
       my->_rule_validator = v;
    }

	uint64_t lotto_db::get_jackpot_for_ticket( uint64_t ticket_block_num, uint64_t lucky_number, uint16_t odds, uint16_t amount)
    {
		/* TODO: generate winning_number according to future blocks, maybe with prove of work
		 * winning_number should be validate by block validation. or generated during block mining, so we can get directly from here.
		 */
		// fc::sha256 winning_number;
		// using the next block generated block number
        uint64_t winning_number = my->_block2summary.fetch(ticket_block_num + BTS_LOTTO_BLOCKS_BEFORE_JACKPOTS_DRAW).winning_number;
		// TODO: what's global_odds, ignore currenly.
		uint64_t global_odds = 0;

		drawing_record dr = my->_drawing2record.fetch(ticket_block_num);
		// TODO: what's the total jackpot and total paid meaning?
		block_summary summary = my->_block2summary.fetch(ticket_block_num);
		uint16_t available_pool_prize = summary.ticket_sales - summary.amount_won;

		// 3. jackpot should not be calculated here, 
		/*
		fc::sha256::encoder enc;
		enc.write( (char*)&lucky_number, sizeof(lucky_number) );
		enc.write( (char*)&winning_number, sizeof(winning_number) );
		enc.result();
		fc::bigint  result_bigint( enc.result() );

		// the ticket number must be below the winning threshold to claim the jackpot
		auto winning_threshold = result_bigint.to_int64 % fc::bigint( global_odds * odds ).to_int64();
		auto ticket_threshold = amount / odds;
		if (winning_threshold < ticket_threshold)	// we have a winners
		{
			return jackpots;
		}
		*/

		return 0;
    }

    /**
     * Performs global validation of a block to make sure that no two transactions conflict. In
     * the case of the lotto only one transaction can claim the jackpot.
     */
    block_evaluation_state_ptr lotto_db::validate( const trx_block& blk, const signed_transactions& determinsitc_trxs )
    {
        block_evaluation_state_ptr blockstate = chain_database::validate(blk, determinsitc_trxs);

        // TODO:
        auto head_blk = static_cast<const bts::lotto::lotto_block&>(blk);
        auto delegate_id = lookup_delegate(blk.block_num)->delegate_id;
        auto itr = my->_delegate2blocks.find(delegate_id);
        if( itr.valid() )
        {
            auto block_ids = itr.value();
            auto trx_blk = fetch_block(block_ids[block_ids.size() - 1]);    // GetLastBlockProducedByDelegate
            auto lotto_blk = static_cast<bts::lotto::lotto_block&>(trx_blk);
            
            // TODO: reviewing the hash.
            FC_ASSERT(fc::sha256::hash(head_blk.revealed_secret) == lotto_blk.secret);
        } else {
            FC_ASSERT(head_blk.revealed_secret == fc::sha256());   //  this is the first block produced by delegate
        }

        return blockstate;
    }

    /** 
     *  Called after a block has been validated and appends
     *  it to the block chain storing all relevant transactions and updating the
     *  winning database.
     */
    void lotto_db::store( const trx_block& blk, const signed_transactions& deterministic_trxs, const block_evaluation_state_ptr& state )
    {
        chain_database::store(blk, deterministic_trxs, state);

        // update drawingrecord and blocksummary, and winning number(used as random)
        drawing_record dr;
        // TODO:

        my->_drawing2record.store(blk.block_num, dr);
        block_summary bs;
        uint64_t ticket_sales = 0;
        uint64_t amout_won = 0;
        for( const signed_transaction& trx : deterministic_trxs )
        {
            for ( auto o : trx.outputs)
            {
                if (o.claim_func == claim_ticket) {
                    ticket_sales += o.amount.get_rounded_amount();
                }
            }

            for ( auto i : trx.inputs)
            {
				auto o = fetch_output(i.output_ref);
                if (o.claim_func == claim_ticket) {
                    for ( auto out : trx.outputs)
                    {
						if (out.claim_func == claim_by_signature) {
                            amout_won += out.amount.get_rounded_amount();
                        }
                    }
					// The in.output should all be tickets, and the out should all be jackpots
					break;
                }
            }
        }
        bs.ticket_sales = ticket_sales;
        bs.amount_won = amout_won;
        // TODO: hash according to block info, move to block summary?
        auto head_blk = static_cast<const bts::lotto::lotto_block&>(blk);
        auto random = fc::sha256::hash(head_blk.revealed_secret.str());
        for( uint32_t i = 1; i < 100; ++i )
        {
            auto h_blk = fetch_block(head_blk.block_num - i);
            auto lotto_blk = static_cast<bts::lotto::lotto_block&>(h_blk);
            random = fc::sha256::hash(lotto_blk.revealed_secret.str() + random.str()); // where + is concat
        }

        // TODO: change wining_number to sha256, and recheck whether sha356 is suitable for hashing.
        bs.winning_number = ((uint64_t)random._hash[0]) <<32 & ((uint64_t)random._hash[0]);
        my->_block2summary.store(blk.block_num, bs);

        // TODO: Should block's delegate id be retrieved this way? Then, how to achieve this before store?
        auto delegate_id = lookup_delegate(blk.block_num)->delegate_id;
        auto block_ids = my->_delegate2blocks.fetch(delegate_id);
        block_ids.push_back(blk.block_num);
        my->_delegate2blocks.store(delegate_id, block_ids);
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
