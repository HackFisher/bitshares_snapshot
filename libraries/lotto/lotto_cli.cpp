#include <bts/client/client.hpp>
#include <bts/cli/cli.hpp>
#include <bts/lotto/lotto_cli.hpp>
#include <bts/lotto/lotto_asset.hpp>
#include <bts/lotto/common.hpp>

#include <iostream>
#include <sstream>
#include <iomanip>

namespace bts { namespace lotto {

lotto_cli::lotto_cli( const client_ptr& client, const bts::rpc::rpc_server_ptr& rpc_server ) : cli(client, rpc_server)
{
}

lotto_cli::~lotto_cli()
{
}

fc::variants lotto_cli::parse_interactive_command(fc::buffered_istream& argument_stream, const std::string& command)
{
    if (command == "lucky_number")
    {
        fc::variants arguments;
        uint16_t group_count = GROUP_COUNT();
        c_rankings c_r;
        for (size_t k = 0; k < group_count; k ++)
        {
            if ( k != 0)
            {
                auto seperator_var = fc::json::from_stream(argument_stream);
                std::string seperator = seperator_var.as_string();
                FC_ASSERT(seperator == "|");
            }
            
            combination combination;
            uint16_t ball_select_count = global_rule_config().ball_group[k].combination_count;
            uint16_t ball_count = global_rule_config().ball_group[k].total_count;
            
            for (size_t i = 0; i < ball_select_count; i ++)
            {
                auto number_var = fc::json::from_stream(argument_stream);
                uint16_t    number = (uint16_t)number_var.as_uint64();
                
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
        arguments.push_back(fc::variant(lucky_number));
        return arguments;
    }
    else
    {
        return cli::parse_interactive_command(argument_stream, command);
    }
}

fc::variant lotto_cli::execute_interactive_command(const std::string& command, const fc::variants& arguments)
{
    if (command == "buy_ticket")
    {
        FC_ASSERT(arguments.size() == 3);
        
        
        auto required_input = arguments[2].as<asset>();
        // _create_sendtoaddress_transaction takes the same arguments as sendtoaddress
        auto result = execute_interactive_command("getbalance", fc::variants());
        auto curr_bal = result.as<bts::blockchain::asset>();
            
        std::cout<<"current balance: "<< curr_bal.get_rounded_amount() <<" "<<fc::variant((asset_type)curr_bal.unit).as_string()<<"\n";
        std::cout<<"total price: "<< required_input.get_rounded_amount() <<" "<<fc::variant((asset_type)required_input.unit).as_string()<<"\n";

        if( required_input > curr_bal )
        {
            std::cout<<"Insufficient Funds\n";
        }
        else
        {
            std::string line;
            std::cout<<"submit order? (y|n): ";
            std::getline( std::cin, line );
            if( line == "yes" || line == "y" )
            {
                execute_interactive_command(command, arguments);
                std::cout<<"order submitted\n";
            }
            else
            {
                std::cout<<"order canceled\n";
            }
        }
    }
    else if (command == "lucky_number")
    {
        FC_ASSERT(arguments.size() == 1);
        uint64_t lucky_number = arguments[0].as_uint64();
            
        std::cout << "lucky number is "<< lucky_number << "\n";
        
        std::string line;
        std::cout<<"submit order? (y|n): ";
        std::getline( std::cin, line );
        if( line == "yes" || line == "y" )
        {
            execute_interactive_command(command, arguments);
            std::cout<<"order submitted\n";
        }
        else
        {
            std::cout<<"order canceled\n";
        }
    }
    else
    {
        return cli::execute_interactive_command(command, arguments);
    }
}
void lotto_cli::format_and_print_result(const std::string& command, const fc::variant& result)
{
    if ( command == "buy_ticket")
    {
        std::cout << "\nDone buy ticket\n";
    }
    if ( command == "lucky_number")
    {
        std::cout << "\nDone lucky number\n";
    }
    else if ( command == "print_rule")
    {
        // TODO: to be improved
        auto config = global_rule_config();
        std::cout << "------Start of Rule " << config.name << "-----\n";
        std::cout << "There are " << config.ball_group.size() << " groups of balls:" << "\n";
        
        for (size_t i = 0; i < config.ball_group.size(); i++)
        {
            std::cout << "\n";
            std::cout << "For the " << i << "th group of ball:" << "\n";
            std::cout << "There are " << (uint16_t)config.ball_group[i].total_count << " balls with different numbers in total" << "\n";
            std::cout << "Player should choose " << (uint16_t)config.ball_group[i].combination_count 
                << " balls out of " << (uint16_t)config.ball_group[i].total_count << " total balls" << "\n";
        }
        
        std::cout << "\n";
        std::cout << "The following are the definition of the prizes:\n";
        type_prizes& pz = config.prizes;
        for (size_t i = 0; i < pz.size(); i ++)
        {
            std::cout << "\n";
            std::cout << "The " << pz[i].level << " level prize's definition is:\n" ;
            //std::count << pz[i].desc;
            std::cout << "Match count for each group: [";
            for (size_t j = 0; j < pz[i].match_list.size(); j ++ )
            {
                if( j != 0)
                {
                    std::cout << " OR ";
                }
                std::cout << "( ";
                for (size_t k = 0; k < pz[i].match_list[j].size(); k++)
                {
                    if (k != 0)
                    {
                        std::cout << ", ";
                    }
                    std::cout << "" << (uint16_t)pz[i].match_list[j][k];
                }
                std::cout << " )";
            }
            
            std::cout << "]\n";
        }
        
        std::cout << "------End of Rule " << config.name << "-----\n";
    }
    else
    {
        cli::format_and_print_result(command, result);
    }
}

}}