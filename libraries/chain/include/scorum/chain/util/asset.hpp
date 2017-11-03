#pragma once

#include <scorum/protocol/asset.hpp>

namespace scorum {
namespace chain {
namespace util {

using scorum::protocol::asset;
using scorum::protocol::price;

inline asset to_sbd(const price& p, const asset& scorum)
{
    FC_ASSERT(scorum.symbol == SCORUM_SYMBOL);
    if (p.is_null())
        return asset(0, SBD_SYMBOL);
    return scorum * p;
}

inline asset to_scorum(const price& p, const asset& sbd)
{
    FC_ASSERT(sbd.symbol == SBD_SYMBOL);
    if (p.is_null())
        return asset(0, SCORUM_SYMBOL);
    return sbd * p;
}
}
}
}
