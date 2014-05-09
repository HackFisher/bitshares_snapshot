#pragma once
#include <bts/blockchain/asset.hpp>

namespace bts { namespace lotto {
    
    
    enum lotto_asset_type
    {
        /** basic lotto share unit */
        dic         = 0,
        bet         = 1,
        lto         = 2
    };
}} //bts::lotto

FC_REFLECT_ENUM(bts::lotto::lotto_asset_type, (dic)(bet)(lto));