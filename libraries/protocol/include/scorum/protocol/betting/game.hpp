#pragma once
#include <fc/static_variant.hpp>

namespace scorum {
namespace protocol {
namespace betting {
struct soccer_game
{
};

struct hockey_game
{
};

using game_type = fc::static_variant<soccer_game, hockey_game>;
}
}
}

FC_REFLECT_EMPTY(scorum::protocol::betting::soccer_game)
FC_REFLECT_EMPTY(scorum::protocol::betting::hockey_game)