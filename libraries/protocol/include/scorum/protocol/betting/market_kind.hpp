#pragma once
#include <fc/reflect/reflect.hpp>

namespace scorum {
namespace protocol {
namespace betting {
enum class market_kind
{
    result,
    round,
    handicap,
    correct_score,
    goal,
    total
};
}
}
}

FC_REFLECT_ENUM(scorum::protocol::betting::market_kind, (result)(round)(handicap)(correct_score)(goal)(total))

namespace fc {
class variant;
void to_variant(const scorum::protocol::betting::market_kind& m, fc::variant& variant);
}