#include <bts/client/client.hpp>
#include <bts/cli/cli.hpp>
#include <bts/lotto/lotto_cli.hpp>
#include <bts/lotto/lotto_asset.hpp>
#include <bts/lotto/rule.hpp>

#include <iostream>
#include <sstream>
#include <iomanip>

namespace bts { namespace lotto {

void lotto_cli::print_help(){
    std::cout<<"Lotto Commands\n";
    std::cout<<"-------------------------------------------------------------\n";
    std::cout<<"buy_ticket [AMOUNT] [LUCKY_NUMBER] [ODDS] - buy tickets for specific lucky number with some odds\n";
    std::cout<<"lucky_number - but 1 share of lotto ticket. FORMAT: R1 R2 R3 R4 R5 | B1 B2 e.g. 3 6 21 25 31 | 4 7\n";
    std::cout<<"print_rule - print details gaming rule.";
    std::cout<<"draw_ticket - draw tickets\n";
    std::cout<<"-------------------------------------------------------------\n";

    cli::print_help();
}
void lotto_cli::process_command( const std::string& cmd, const std::string& args ){
    std::stringstream ss(args);

    const lotto_db_ptr db = std::dynamic_pointer_cast<lotto_db>(client()->get_chain());
    const lotto_wallet_ptr wallet = std::dynamic_pointer_cast<lotto_wallet>(client()->get_wallet());

    if( cmd == "buy_ticket" )
	{
        if( check_unlock() )
        {
            std::string line;
            // TODO: process buy lotto ticket commands according to rule config
            std::string base_str,at;
            double      amount;
            // TODO: maybe lucky_number and odds should not be optional?
            uint16_t    lucky_number;
            uint16_t    odds;
            ss >> amount >> lucky_number >> odds;
            asset       amnt(amount);
            
            auto required_input = amnt;
            asset curr_bal = wallet->get_balance(0);
            
            std::cout<<"current balance: "<< curr_bal.get_rounded_amount() <<" "<<fc::variant((asset_type)curr_bal.unit).as_string()<<"\n";
            std::cout<<"total price: "<< required_input.get_rounded_amount() <<" "<<fc::variant((asset_type)required_input.unit).as_string()<<"\n";
            
            if( required_input > curr_bal )
            {
                std::cout<<"Insufficient Funds\n";
            }
            else
            {
                std::cout<<"submit order? (y|n): ";
                std::getline( std::cin, line );
                if( line == "yes" || line == "y" )
                {
                    wallet->buy_ticket(lucky_number, odds, amnt);
                    std::cout<<"order submitted\n";
                }
                else
                {
                    std::cout<<"order canceled\n";
                }
            }
        }
	} else if (cmd == "lucky_number")
    {
        if( check_unlock() )
        {
            std::string line;
            uint16_t group_count = GROUP_COUNT();
            c_rankings c_r;
            for (size_t k = 0; k < group_count; k ++)
            {
                if ( k != 0)
                {
                    char seperator;
                    ss >> seperator;
                    FC_ASSERT(seperator == '|');
                }
                
                combination combination;
                uint16_t ball_select_count = global_rule_config().balls[k].second;
                uint16_t ball_count = global_rule_config().balls[k].first;
                
                for (size_t i = 0; i < ball_select_count; i ++)
                {
                    uint16_t    number;
                    ss >> number;
                    
                    FC_ASSERT(number > 0 && number <= ball_count);
                    
                    
                    combination.push_back(number - 1);
                }
                
                // assert unique of number
                std::sort(combination.begin(), combination.end());
                for ( size_t j = 1; j < combination.size(); j ++ )
                {
                    FC_ASSERT(combination[j - 1] != combination[j]);
                }
                
                uint64_t r = ranking(combination);
                c_r.push_back(r);
            }
            
            uint64_t lucky_number = ranking(c_r, GROUP_SPACES());
            
            std::cout << "lucky number is "<< lucky_number << "\n";
            
            std::cout<<"submit order? (y|n): ";
            std::getline( std::cin, line );
            if( line == "yes" || line == "y" )
            {
                asset amt(1.0);
                wallet->buy_ticket(lucky_number, 1, amt);
                std::cout<<"order submitted\n";
            }
            else
            {
                std::cout<<"order canceled\n";
            }
        }
    }
    else if ( cmd == "draw_ticket")
	{
		wallet->draw_ticket();
	} else if ( cmd == "query_jackpots")
    {
        // TODO
    } else if ( cmd == "print_rule")
    {
        // TODO: to be improved
        auto config = global_rule_config();
        std::cout << "------Start of Rule " << config.name << "-----\n";
        std::cout << "There are " << config.balls.size() << " groups of balls:" << "\n";
        
        for (size_t i = 0; i < config.balls.size(); i ++)
        {
            std::cout << "\n";
            std::cout << "For the " << i << "th group of ball:" << "\n";
            std::cout << "There are " << (uint16_t)config.balls[i].first << " balls with different numbers in total" << "\n";
            std::cout << "Player should choose " << (uint16_t)config.balls[i].second << " balls out of " << (uint16_t)config.balls[i].first << " total balls" << "\n";
        }
        
        std::cout << "\n";
        std::cout << "The following are the definition of the prizes:\n";
        type_prizes& pz = config.prizes;
        for (size_t i = 0; i < pz.size(); i ++)
        {
            std::cout << "\n";
            std::cout << "The " << pz[i].first << " level prize's definition is:\n" ;
            //std::count << pz[i].desc;
            std::cout << "Match count for each group: [";
            for (size_t j = 0; j < pz[i].second.size(); j ++ )
            {
                if( j != 0)
                {
                    std::cout << " OR ";
                }
                std::cout << "( ";
                for (size_t k = 0; k < pz[i].second[j].size(); k ++ )
                {
                    if (k != 0)
                    {
                        std::cout << ", ";
                    }
                    std::cout << "" << (uint16_t)pz[i].second[j][k];
                }
                std::cout << " )";
            }
            
            std::cout << "]\n";
        }
        
        std::cout << "------End of Rule " << config.name << "-----\n";
    }
    else
    {
        cli::process_command(cmd, args);
    }
}
            
void lotto_cli::list_transactions( uint32_t count ){
    // TODO: list lotto related transactions
    cli::list_transactions(count);
}
void lotto_cli::get_balance( uint32_t min_conf, uint16_t unit ){
    // TODO: list lotto related balances
    cli::get_balance(min_conf, unit);
}

}}