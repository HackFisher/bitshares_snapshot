#include <bts/client/client.hpp>
#include <bts/cli/cli.hpp>
#include <bts/lotto/lotto_cli.hpp>
#include <bts/lotto/lotto_asset.hpp>

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
    return cli::parse_interactive_command(argument_stream, command);
}

fc::variant lotto_cli::execute_interactive_command(const std::string& command, const fc::variants& arguments)
{
    if (command == "buy_ticket")
    {
        FC_ASSERT(arguments.size() == 3);
        
        auto required_input = arguments[2].as<asset>();
        // _create_sendtoaddress_transaction takes the same arguments as sendtoaddress
        auto result = cli::execute_interactive_command("getbalance", fc::variants());
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
                std::cout << "order submitted\n";
                return cli::execute_interactive_command(command, arguments);
                
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
            std::cout << "order submitted\n";
            return cli::execute_interactive_command(command, arguments);
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

    return fc::variant();
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
        lotto_rule::config_instance().print_rule();
    }
    else
    {
        cli::format_and_print_result(command, result);
    }
}

}}