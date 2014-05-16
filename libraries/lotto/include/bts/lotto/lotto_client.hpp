#pragma once
#include <bts/client/client.hpp>

namespace bts { namespace lotto {

    namespace detail { class lotto_client_impl; }

class lotto_client : public bts::client::client
{
    public:
        lotto_client();
        ~lotto_client();

        void run_secret_broadcastor(const fc::ecc::private_key& k, const std::string& wallet_pass, const fc::path& datadir);
    private:
        std::unique_ptr<detail::lotto_client_impl> my;

};

typedef std::shared_ptr<lotto_client> lotto_client_ptr;
}} //bts::lotto