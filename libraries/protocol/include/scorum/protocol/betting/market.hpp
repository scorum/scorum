#pragma once
#include <scorum/protocol/betting/wincase.hpp>
#include <scorum/protocol/betting/wincase_comparison.hpp>

namespace scorum {
namespace protocol {
namespace betting {
struct market_type
{
    market_kind kind;
    fc::flat_set<wincase_pair> wincases;
};
}
}
}

namespace std {
using namespace scorum::protocol::betting;
template <> struct less<market_type>
{
    bool operator()(const market_type& lhs, const market_type& rhs) const
    {
        return lhs.kind < rhs.kind;
    }
};
}

FC_REFLECT(scorum::protocol::betting::market_type, (kind)(wincases))