#pragma once
#include <fc/reflect/reflect.hpp>

namespace scorum {
namespace protocol {
enum class market_kind : uint16_t
{
    result,
    round,
    handicap,
    correct_score,
    goal,
    total,
    total_goals
};
}
}

FC_REFLECT_ENUM(scorum::protocol::market_kind, (result)(round)(handicap)(correct_score)(goal)(total)(total_goals))

namespace fc {
class variant;
void to_variant(const scorum::protocol::market_kind& m, fc::variant& variant);
}
