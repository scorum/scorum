#include <scorum/protocol/betting/game.hpp>
#include <scorum/protocol/betting/game_serialization.hpp>
#include <scorum/utils/static_variant_serialization.hpp>

namespace fc {

using namespace scorum;
using namespace scorum::protocol::betting;

void to_variant(const game_type& game, fc::variant& var)
{
    utils::to_variant(game, var);
}
void from_variant(const fc::variant& var, game_type& game)
{
    utils::from_variant(var, game);
}
}