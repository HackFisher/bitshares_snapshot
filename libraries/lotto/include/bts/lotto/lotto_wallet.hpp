#pragma once
#include <bts/blockchain/address.hpp>
#include <bts/blockchain/transaction.hpp>
#include <bts/wallet/wallet.hpp>
#include <bts/lotto/lotto_db.hpp>

namespace bts { namespace lotto {
    //namespace detail  { class dns_wallet_impl; }
    using namespace bts::blockchain;
    using namespace bts::wallet;
    class lotto_wallet : public bts::wallet::wallet
    {
        public:
            lotto_wallet();
            ~lotto_wallet();
            bts::blockchain::signed_transaction buy_ticket(const uint64_t& luckynumber, const uint16_t& odds,
                                                        asset amount, lotto_db& db);

        protected:
            virtual bool scan_output( transaction_state& state, const trx_output& out, const output_reference& out_ref, const bts::wallet::output_index& oidx );

        private:
             //std::unique_ptr<detail::lotto_wallet_impl> my;
     };

} } // bts::lotto
