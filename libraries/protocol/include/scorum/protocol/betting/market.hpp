#pragma once
#include <scorum/protocol/betting/wincase.hpp>

namespace scorum {
namespace protocol {
namespace betting {
struct market_type
{
    market_kind kind;
    std::vector<wincase_type> wincases;
};
}
}
}

FC_REFLECT(scorum::protocol::betting::market_type, (kind)(wincases))