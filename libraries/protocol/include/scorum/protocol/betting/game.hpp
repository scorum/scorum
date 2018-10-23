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

namespace fc {
using scorum::protocol::game_type;

template <> void to_variant(const game_type& game, fc::variant& variant);
template <> void from_variant(const fc::variant& variant, game_type& game);
}

FC_REFLECT_EMPTY(scorum::protocol::soccer_game)
FC_REFLECT_EMPTY(scorum::protocol::hockey_game)
