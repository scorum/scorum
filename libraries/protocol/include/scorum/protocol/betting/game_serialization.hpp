#include <scorum/protocol/betting/game.hpp>

namespace fc {
using namespace scorum::protocol::betting;

class variant;

void to_variant(const game_type& game, fc::variant& variant);
void from_variant(const fc::variant& variant, game_type& game);
}