#pragma once
#include <fc/static_variant.hpp>

namespace scorum {
namespace protocol {
struct soccer_game
{
};

struct hockey_game
{
};

using game_type = fc::static_variant<soccer_game, hockey_game>;
}
}

FC_REFLECT_EMPTY(scorum::protocol::soccer_game)
FC_REFLECT_EMPTY(scorum::protocol::hockey_game)
