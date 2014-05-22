#pragma once
#include <bts/client/client.hpp>
#include <bts/lotto/lotto_db.hpp>
#include <bts/lotto/lotto_wallet.hpp>

namespace bts { namespace lotto {

    namespace detail { class lotto_delegate_impl; }

    class lotto_delegate
    {
        public:
            lotto_delegate();

            ~lotto_delegate();

            void set_lotto_db(const lotto_db_ptr& ptr);

            void set_lotto_wallet(const lotto_wallet_ptr& wall);

            void set_client(const bts::client::client_ptr client);

            void run_secret_broadcastor(const fc::path& datadir);
        private:
            std::unique_ptr<detail::lotto_delegate_impl> my;

    };

    typedef std::shared_ptr<lotto_delegate> lotto_delegate_ptr;
}} //bts::lotto