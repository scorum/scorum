#include <scorum/protocol/betting/game.hpp>
#include <scorum/utils/static_variant_serialization.hpp>

namespace fc {

using scorum::protocol::game_type;

template <> void to_variant(const game_type& game, fc::variant& var)
{
    scorum::utils::to_variant(game, var);
}
template <> void from_variant(const fc::variant& var, game_type& game)
{
    scorum::utils::from_variant(var, game);
}
}
