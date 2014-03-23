#include <bts/blockchain/config.hpp>
#include <bts/wallet/wallet.hpp>

#include <bts/lotto/lotto_wallet.hpp>

#include<fc/reflect/variant.hpp>
#include<fc/io/raw.hpp>
#include<fc/io/raw_variant.hpp>

#include <fc/log/logger.hpp>

namespace bts { namespace lotto {

using namespace bts::blockchain;

lotto_wallet::lotto_wallet()
//:my(new detail::lotto_wallet_impl())
{
}

lotto_wallet::~lotto_wallet()
{
}

}} // bts::lotto
