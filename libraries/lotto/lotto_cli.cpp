#include <bts/client/client.hpp>
#include <bts/cli/cli.hpp>
#include <bts/lotto/lotto_cli.hpp>
#include <bts/lotto/lotto_asset.hpp>

#include <iostream>
#include <sstream>
#include <iomanip>

namespace bts { namespace lotto {

using namespace client;

lotto_cli::lotto_cli( const client_ptr& c, const lotto_wallet_ptr& w)
	: bts::cli::cli(c), my_wallet(w)
{
}

lotto_cli::~lotto_cli()
{
}

void lotto_cli::print_help(){
    std::cout<<"Lotto Commands\n";
    std::cout<<"-------------------------------------------------------------\n";
    std::cout<<"buy_ticket AMOUNT UNIT [LUCKY_NUMBER] [ODDS] - buy tickets for specific lucky number with some odds\n";
	std::cout<<"draw_ticket - draw tickets\n";
    std::cout<<"-------------------------------------------------------------\n";

    cli::print_help();
}
void lotto_cli::process_command( const std::string& cmd, const std::string& args ){
    std::stringstream ss(args);

    if( cmd == "buy_ticket" )
	{
        std::string line;
        // TODO: process buy lotto ticket commands according to rule config
        std::string base_str,at;
        double      amount;
        // TODO: maybe lucky_number and odds should not be optional?
        uint16_t    lucky_number;
        uint16_t    odds;
        std::string unit;
        ss >> amount >> unit >> lucky_number >> odds;
        asset::type base_unit = fc::variant(unit).as<asset_type>();
        asset       amnt = asset(amount,base_unit);

        auto required_input = amnt;
        // TODO: customize lotto::type value for lotto share?
		asset curr_bal = my_wallet->get_balance(base_unit);

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
				my_wallet->buy_ticket(lucky_number, odds, amnt);
                std::cout<<"order submitted\n";
            }
            else
            {
                std::cout<<"order canceled\n";
            }
        }
	} else if ( cmd == "draw_ticket")
	{
		my_wallet->draw_ticket();
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