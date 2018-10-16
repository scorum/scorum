#pragma once
#include <fc/reflect/reflect.hpp>

namespace scorum {
namespace protocol {
enum class game_status : uint8_t
{
    created = 0b0001,
    started = 0b0010,
    finished = 0b0100,
    resolved = 0b1000,
    expired = 0b10000
};
}
}

FC_REFLECT_ENUM(scorum::protocol::game_status, (created)(started)(finished)(resolved)(expired))

namespace fc {
class variant;
void to_variant(const scorum::protocol::game_status& m, fc::variant& variant);
}
