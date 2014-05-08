#pragma once
#include <bts/blockchain/asset.hpp>

namespace bts { namespace lotto {
    
    
    enum lotto_asset_type
    {
        /** basic lotto share unit */
        bet         = 0,
        lto         = 1
    };
}} //bts::lotto

FC_REFLECT_ENUM(bts::lotto::lotto_asset_type, (bet)(lto));