#include <bts/lotto/lotto_outputs.hpp>
#include <bts/lotto/lotto_db.hpp>
#include <bts/lotto/lotto_rule.hpp>
#include <bts/lotto/lotto_config.hpp>

#include <fc/reflect/variant.hpp>
#include <fc/io/raw.hpp>
#include <fc/io/json.hpp>
#include <fc/log/logger.hpp>
#include <bts/db/level_map.hpp>

#include <algorithm>
#include <bitset>
#include <functional>
#include <vector>
#include <iostream>

namespace bts { namespace lotto {
    namespace detail
    {
        class lotto_rule_impl
        {
            public:
                lotto_rule_impl(){}

                // map drawning number to drawing record
                bts::db::level_map<uint32_t, drawing_record>  _drawing2record;
        };
    }

    namespace helper
    {
        uint64_t Combination(uint16_t N, uint16_t k)
        {
            static uint64_t C[BTS_LOTTO_MAX_BALL_COUNT][BTS_LOTTO_MAX_BALL_COUNT / 2];
            static bool inited = false;

            if (!inited) {
                int i,j;
                for (j = 0; j < (BTS_LOTTO_MAX_BALL_COUNT / 2); ++j)
                {
                    C[0][j] = 0;
                }
                for (i = 0; i < BTS_LOTTO_MAX_BALL_COUNT; ++i)
                {
                    C[i][0] = 1;
                }

                for (i = 1; i < BTS_LOTTO_MAX_BALL_COUNT; ++i)
                {
                    for (j = 1; j < (BTS_LOTTO_MAX_BALL_COUNT / 2); ++j)
                    {
                        C[i][j] = C[i-1][j] + C[i-1][j-1];
                    }
                }
                inited = true;
            }

            if (N < k) return 0;
            if ((N - k) < k) k = N - k;

            FC_ASSERT(N < BTS_LOTTO_MAX_BALL_COUNT, 
                "N should less than maximum ball count.", ("N", N)("MAX", BTS_LOTTO_MAX_BALL_COUNT));
            FC_ASSERT(k < BTS_LOTTO_MAX_BALL_COUNT / 2, "k should less than half of maximum ball count.", ("k", k)("MAX", BTS_LOTTO_MAX_BALL_COUNT));
    
            return C[N][k];
        }

        group_match match_rankings(const c_rankings& l, const c_rankings& r, const type_ball_group& balls)
        {
            FC_ASSERT(l.size() == r.size());
            FC_ASSERT(l.size() == balls.size());
            FC_ASSERT(balls.size() < BTS_LOTTO_MAX_BALL_COUNT);
    
            group_match m;

            for(size_t i = 0; i < l.size(); i ++)
            {
                combination left_combinatioin = unranking(l[i], balls[i].combination_count, balls[i].total_count);
                combination right_combinatioin = unranking(r[i], balls[i].combination_count, balls[i].total_count);

                std::bitset<BTS_LOTTO_MAX_BALL_COUNT> left_bits, right_bits;
                for (size_t i = 0; i < balls.size(); i++){
                    left_bits[left_combinatioin[i]] = 1;
                    right_bits[right_combinatioin[i]] = 1;
                }

                m.push_back( (left_bits & right_bits).count() );
            }

            FC_ASSERT(balls.size() == m.size());

            return m;
        }

        uint64_t ranking(const c_rankings& r, const std::vector<uint64_t>& spaces )
        {
            FC_ASSERT( r.size() == spaces.size() );
            uint64_t res = 0;
            for (uint16_t i = 0; i < spaces.size(); i ++ )
            {
                FC_ASSERT( r[i] < spaces[i] );
                if (i == 0)
                {
                    res += r[i];
                } else {
                    res = res * spaces[i] + r[i];
                }
            }

            return res;
        }

        // unranking to combination rankings
        // TODO: making return value const?
        c_rankings unranking(uint64_t num, const std::vector<uint64_t>& spaces )
        {
            c_rankings rs;
            for (int i = spaces.size() - 1; i >= 0; i --) {
                rs.push_back(num % spaces[i]);
                num = num / spaces[i];
            }

            std::reverse(rs.begin(), rs.end());
            return rs;
        }

        uint64_t ranking(const combination& c)
        {
            std::vector<uint16_t> v(c);
            std::sort(v.begin(), v.end());
            uint64_t n = 0;
            // sum of C(v[k - 1], k)
            for (size_t i = 1; i <= v.size(); i ++) {
                n += Combination(v[i - 1], i);
            }
            return n;
        }

        combination unranking(uint64_t num, uint16_t k, uint16_t n)
        {
            std::vector<uint16_t> c;
            uint16_t max = n;

            for (uint16_t i = k; i >= 1; i--)
            {
                if (num <= 0) {
                    c.push_back(i-1);
                } else {
                    for (; max >= 1;)
                    {
                        uint64_t c_max_i = Combination(max, i);
                        if (num >= c_max_i) {
                            c.push_back(max);

                            num -=  c_max_i;
                            max --;
                            break;
                        } else {
                            max --;
                        }
                    }
                }
            }

            std::sort(c.begin(), c.end());

            return c;
        }

        uint64_t lucky_number_from_variants(const fc::variants& params)
        {
            uint16_t group_count = lotto_rule::config_instance().group_count();
            FC_ASSERT(group_count > 0, "group size is 0.");
            size_t expected_param_size = 0;
            for (size_t k = 0; k < group_count; k ++)
            {
                if ( k != 0)
                {
                    expected_param_size ++;
                }
                
                expected_param_size += lotto_rule::config_instance().ball_group[k].combination_count;
            }

            FC_ASSERT(params.size() >= expected_param_size, "No enough parameters.", ("size", params.size()));

            c_rankings c_r;
            size_t i = 0;
            for (size_t k = 0; k < group_count; k ++)
            {
                if ( k != 0)
                {
                    std::string seperator = params[i].as_string();
                    FC_ASSERT(seperator == "|", "The seperator is not right, should be '|' .", ("seperator", seperator));
                    i ++;
                }
            
                combination combination;
                uint16_t ball_select_count = lotto_rule::config_instance().ball_group[k].combination_count;
                uint16_t ball_count = lotto_rule::config_instance().ball_group[k].total_count;
            
                for (size_t i = 0; i < ball_select_count; i ++)
                {
                    uint16_t    number = (uint16_t)params[i].as_uint64();
                
                    FC_ASSERT(number > 0 && number <= ball_count);
                    i ++;
                
                    combination.push_back(number - 1);
                }
            
                // assert unique of number
                std::sort(combination.begin(), combination.end());
                for ( size_t j = 1; j < combination.size(); j ++ )
                {
                    FC_ASSERT(combination[j - 1] != combination[j], "There are repeating numbers.");
                }
            
                uint64_t r = ranking(combination);
                c_r.push_back(r);
            }
        
            uint64_t lucky_number = ranking(c_r, lotto_rule::group_spaces());

            return lucky_number;
        }
    }

    // Default rule validator implement, TODO: may be move default to another class
    lotto_rule::lotto_rule(lotto_db* db, ticket_type t, asset::unit_type u)
        :rule(db, t, u), my(new detail::lotto_rule_impl())
    {
    }
    lotto_rule::~lotto_rule()
    {
    }

    void lotto_rule::open( const fc::path& dir, bool create )
    {
        try {
            my->_drawing2record.open( dir / "lotto_rule" / "drawing2record", create );
        } FC_RETHROW_EXCEPTIONS( warn, "Error loading lotto rule database ${dir}", ("dir", dir)("create", create) );
    }

    void lotto_rule::close() 
    {
        my->_drawing2record.close();
    }

    void lotto_rule::config::print_rule() const
    {
        std::cout << "------Start of Rule " << name << "-----\n";
        std::cout << "There are " << ball_group.size() << " groups of balls:" << "\n";
        
        for (size_t i = 0; i < ball_group.size(); i++)
        {
            std::cout << "\n";
            std::cout << "For the " << i << "th group of ball:" << "\n";
            std::cout << "There are " << (uint16_t)ball_group[i].total_count << " balls with different numbers in total" << "\n";
            std::cout << "Player should choose " << (uint16_t)ball_group[i].combination_count 
                << " balls out of " << (uint16_t)ball_group[i].total_count << " total balls" << "\n";
        }

        std::cout << "\n";
        std::cout << "The following are the definition of the prizes:\n";
        for (size_t i = 0; i < prizes.size(); i ++)
        {
            std::cout << "\n";
            std::cout << "The " << prizes[i].level << " level prize's definition is:\n" ;
            //std::count << pz[i].desc;
            std::cout << "Match count for each group: [";
            for (size_t j = 0; j < prizes[i].match_list.size(); j ++ )
            {
                if( j != 0)
                {
                    std::cout << " OR ";
                }
                std::cout << "( ";
                for (size_t k = 0; k < prizes[i].match_list[j].size(); k++)
                {
                    if (k != 0)
                    {
                        std::cout << ", ";
                    }
                    std::cout << "" << (uint16_t)prizes[i].match_list[j][k];
                }
                std::cout << " )";
            }
            
            std::cout << "]\n";
        }

        std::cout << "------End of Rule " << name << "-----\n";
    }

    const lotto_rule::config& lotto_rule::config_instance()
    {
        static lotto_rule::config config;
        if (config.valid) return config;

        try {
            FC_ASSERT(fc::exists("lotto_rule.json"));
            config = fc::json::from_file("lotto_rule.json").as<lotto_rule::config>();
            // TODO: validation and assert
            config.valid = true;
            return config;
        }
        catch (const fc::exception& e)
        {
            wlog("caught exception!: ${e}", ("e", e.to_detail_string()));
            throw;
        }
    }

    const std::vector<uint64_t>& lotto_rule::group_spaces()
    {
        static std::vector<uint64_t> spaces;
        if (spaces.size() > 0)    {
            FC_ASSERT(spaces.size() == config_instance().group_count());

            return spaces;
        }

        uint16_t group_count = config_instance().group_count();
        for (int i = 0; i < group_count; i++)
        {
            uint16_t N = config_instance().ball_group[i].total_count;
            uint16_t k = config_instance().ball_group[i].combination_count;
            spaces.push_back(helper::Combination(N, k));
        }
               
        return spaces;
    }

    const uint64_t& lotto_rule::total_space()
    {
        static uint64_t total = 0;

        if (total > 0) return total;

        total = 1;
        const std::vector<uint64_t>& spaces = group_spaces();

        for (size_t i = 0; i < spaces.size(); i++)
        {
            FC_ASSERT(spaces[i] >= 0);
            total *= spaces[i];
        }

        return total;
    }
    
    uint64_t lotto_rule::evaluate_total_jackpot(const uint64_t& block_random_number, const uint64_t& ticket_sale, const uint64_t& target_block_num, const uint64_t& jackpot_pool)
    {
        if (target_block_num < BTS_LOTTO_BLOCKS_BEFORE_JACKPOTS_DRAW) {
            return 0;
        }
        
        // TODO:make sure that the jackpot_pool is large than the sum ticket sales of blk.block_num - 99 , blk.block_num - 98 ... blk.block_num.
        
        // available funds - all 100 tickets sales / 100 + (block_num - 100)th ticket sale
        // as the total jackpots.
        
        // uint16_t available_pool_prize = summary.ticket_sales - summary.amount_won;
        // auto summary = my->_block2summary.fetch(ticket_block_num);
        return -1;
    }

    asset lotto_rule::jackpot_for_ticket(const meta_ticket_output& meta_ticket_out)
    {
        auto ticket = meta_ticket_out.ticket_out.ticket.as<lottery_ticket>();
        FC_ASSERT(meta_ticket_out.ticket_out.ticket.ticket_func == get_ticket_type());
        FC_ASSERT(ticket.unit == get_asset_unit());
        FC_ASSERT(ticket.unit == meta_ticket_out.amount.unit);

        auto headnum = _lotto_db->head_block_num();

        // To be move to rule specific valide api, different rule may have different rule validation
        {
            // TODO: ticket must have been purchased in the past 7 days, do not understand, draw by day? to remove it.
            // FC_ASSERT( headnum - trx_loc.block_num < (BTS_BLOCKCHAIN_BLOCKS_PER_DAY*7) );
            // ticket must be before the last drawing... do not understand, draw by day? to remove it.
            // FC_ASSERT( trx_loc.block_num < (headnum/BTS_BLOCKCHAIN_BLOCKS_PER_DAY)*BTS_BLOCKCHAIN_BLOCKS_PER_DAY );
                
            FC_ASSERT(headnum >= BTS_LOTTO_BLOCKS_BEFORE_JACKPOTS_DRAW  + meta_ticket_out.out_idx.block_idx);
            // TODO: For current, the ticket draw trx must be created by the owner.
            // FC_ASSERT( lotto_state.has_signature( claim_ticket.owner ), "", ("owner",claim_ticket.owner)("sigs",state.sigs) );
        }

        uint64_t total_jackpots = my->_drawing2record.fetch(meta_ticket_out.out_idx.block_idx + BTS_LOTTO_BLOCKS_BEFORE_JACKPOTS_DRAW).total_jackpot;

        FC_ASSERT(_lotto_db->head_block_num() >= meta_ticket_out.out_idx.block_idx + BTS_LOTTO_BLOCKS_BEFORE_JACKPOTS_DRAW);
        // fc::sha256 random_number;
        // using the next block generated block number
        uint64_t random_number = _lotto_db->fetch_blk_random_number(meta_ticket_out.out_idx.block_idx + BTS_LOTTO_BLOCKS_BEFORE_JACKPOTS_DRAW);

        // TODO: what's global_odds, ignore currenly.
        uint64_t global_odds = 0;
        uint64_t lucky_number = ticket.lucky_number;

        // This is only one kind of implementation, we call also implement it as dice.
        uint64_t total_space = lotto_rule::total_space();
        uint64_t rule_winning_number = random_number % total_space;
        uint64_t rule_lucky_number = lucky_number % total_space;
        c_rankings winning_rs = helper::unranking(rule_winning_number, lotto_rule::group_spaces());
        c_rankings lucky_rs = helper::unranking(rule_lucky_number, lotto_rule::group_spaces());
        group_match m = helper::match_rankings(winning_rs, lucky_rs, config_instance().ball_group);

        const type_prizes& prizes = config_instance().prizes;

        // get prize_level, TODO: extract/refactor as utility method
        uint16_t level;
        for (size_t i = 0; i < prizes.size(); i ++)
        {
            const std::vector<group_match>& matches = prizes[i].match_list;
            bool found = false;
            for (size_t j = 0; j < matches.size(); j ++)
            {
                if (m == matches[j])
                {
                    found = true;
                    break;
                }
            }
            if (found)
            {
                level = prizes[i].level;
            }
        }

        // TODO switch case... level to find jackpots
        asset jackpot = meta_ticket_out.amount;

        fc::sha256::encoder enc;
        enc.write( (char*)&lucky_number, sizeof(lucky_number) );
        enc.write( (char*)&random_number, sizeof(random_number) );
        enc.result();
        //fc::bigint  result_bigint( enc.result() );


        // ...

        // 3. jackpot should not be calculated here, 
        /*
        

        // the ticket number must be below the winning threshold to claim the jackpot
        auto winning_threshold = result_bigint.to_int64 % fc::bigint( global_odds * odds ).to_int64();
        auto ticket_threshold = amount / odds;
        if (winning_threshold < ticket_threshold)    // we have a winners
        {
            return jackpots;
        }
        */

        // return 0;

        return jackpot;
    }

    void lotto_rule::validate( const trx_block& blk, const signed_transactions& deterministic_trxs )
    {
        for (const signed_transaction& trx : deterministic_trxs)
        {
            auto trx_num_paid = jackpot_paid_in_transaction(trx);

            if (trx_num_paid.second > 0)
            {
                auto winning_block_num = trx_num_paid.first.block_num + BTS_LOTTO_BLOCKS_BEFORE_JACKPOTS_DRAW;
                // The in.output should all be tickets, and the out should all be jackpots
                auto draw_record = my->_drawing2record.fetch(winning_block_num);
                FC_ASSERT(draw_record.total_paid + trx_num_paid.second <= draw_record.total_jackpot, "The paid jackpots is out of the total jackpot.");
            }
        }
    }

    /*
    
    */

    /**
     * @return <ticket transaction number, paid_jackpot for that ticket>
     */
    std::pair<trx_num, uint64_t> lotto_rule::jackpot_paid_in_transaction(const signed_transaction& trx)
    {
        uint64_t trx_paid = 0;
        trx_num trx_n;

        for (auto i : trx.inputs)
        {
            // TODO: Fee? There is no fee in deterministic trxs
            auto o = _lotto_db->fetch_output(i.output_ref);
            trx_n = _lotto_db->fetch_trx_num(i.output_ref.trx_hash);
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

    void lotto_rule::store( const trx_block& blk, const signed_transactions& deterministic_trxs, const block_evaluation_state_ptr& state )
    {
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
                auto draw_record = my->_drawing2record.fetch(winning_block_num);
                draw_record.total_paid = draw_record.total_paid + trx_num_paid.second;
                my->_drawing2record.store(winning_block_num, draw_record);
            }
        }

                    

        drawing_record dr;
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
        dr.ticket_sales = ticket_sales;
        dr.amount_won = amout_won;

        // update jackpots.....
        // the drawing record in this block, corresponding to previous related ticket purchase block (blk.block_num - BTS_LOTTO_BLOCKS_BEFORE_JACKPOTS_DRAW)
        uint64_t last_jackpot_pool = 0;
        if (blk.block_num > 0)
        {
            last_jackpot_pool = my->_drawing2record.fetch(blk.block_num - 1).jackpot_pool;
        }
                    
        dr.total_jackpot = evaluate_total_jackpot(_lotto_db->fetch_blk_random_number(blk.block_num), dr.ticket_sales, blk.block_num, last_jackpot_pool);
        // just the begin, still not paid
        dr.total_paid = 0;
        dr.jackpot_pool = last_jackpot_pool + dr.ticket_sales - dr.total_jackpot;
        // TODO: how to move to validate()
        FC_ASSERT(dr.jackpot_pool >= 0, "jackpot is out ...");
        // Assert that the jackpot_pool is large than the sum ticket sales of blk.block_num - 99 , blk.block_num - 98 ... blk.block_num.
        my->_drawing2record.store(blk.block_num, dr);
    }
}} // bts::lotto
