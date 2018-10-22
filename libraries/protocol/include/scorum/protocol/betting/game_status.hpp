#pragma once
#include <fc/reflect/reflect.hpp>

namespace scorum {
namespace protocol {
enum class game_status : uint8_t
{
    created,
    started,
    finished,
    resolved,
    expired,
    cancelled
};
}
}

FC_REFLECT_ENUM(scorum::protocol::game_status, (created)(started)(finished)(resolved)(expired)(cancelled))

namespace fc {
class variant;
void to_variant(const scorum::protocol::game_status& m, fc::variant& variant);
}
