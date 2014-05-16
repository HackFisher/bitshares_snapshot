#pragma once
#include <bts/client/client.hpp>

namespace bts { namespace lotto {

    namespace detail { class lotto_delegate_impl; }

    class lotto_delegate
    {
        public:
            lotto_delegate();

            ~lotto_delegate();

            void set_lotto_db(const bts::lotto::lotto_db_ptr& ptr);

            void set_lotto_wallet(const bts::lotto::lotto_wallet_ptr& wall);

            void set_client(const bts::client::client_ptr client);

            void run_secret_broadcastor(const fc::ecc::private_key& k, const std::string& wallet_pass, const fc::path& datadir);
        private:
            std::unique_ptr<detail::lotto_delegate_impl> my;

    };

    typedef std::shared_ptr<lotto_delegate> lotto_delegate_ptr;
}} //bts::lotto