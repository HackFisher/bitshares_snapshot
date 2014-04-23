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
            lotto_cli ( const client_ptr& client, const bts::rpc::rpc_server_ptr& rpc_server );
            virtual ~lotto_cli();

            virtual fc::variants parse_interactive_command(fc::buffered_istream& argument_stream, const std::string& command);
        
            virtual fc::variant execute_interactive_command(const std::string& command, const fc::variants& arguments);
        
            virtual void format_and_print_result(const std::string& command, const fc::variant& result);

        private:
            // std::unique_ptr<detail::cli_impl> my;
     };

} } // bts::lotto