#include <scorum/protocol/betting/game_status.hpp>
#include <fc/variant.hpp>

namespace fc {
void to_variant(const scorum::protocol::game_status& m, fc::variant& variant)
{
    variant = fc::reflector<scorum::protocol::game_status>::to_string(m);
}
}
