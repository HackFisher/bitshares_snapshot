#pragma once
#include <bts/blockchain/asset.hpp>

namespace bts { namespace lotto {
    
    
    enum asset_type
    {
        /** basic lotto share unit */
        lto         = 0,
    };
}} //bts::lotto

FC_REFLECT_ENUM(bts::lotto::asset_type, (lto));