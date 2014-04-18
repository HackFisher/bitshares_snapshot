#pragma once
#include <bts/cli/cli.hpp>
#include <bts/client/client.hpp>
#include <bts/lotto/lotto_db.hpp>
#include <bts/lotto/lotto_wallet.hpp>

namespace bts { namespace lotto {
    using namespace client;
    class lotto_cli : public bts::cli::cli
    {
        public:
            lotto_cli (const client_ptr &c) : cli(c)
            {
            }

            virtual void print_help();
            virtual void process_command( const std::string& cmd, const std::string& args );
            
            virtual void list_transactions( uint32_t count = 0 );
            virtual void get_balance( uint32_t min_conf, uint16_t unit = 0 );

        private:
            // std::unique_ptr<detail::cli_impl> my;
     };

} } // bts::lotto