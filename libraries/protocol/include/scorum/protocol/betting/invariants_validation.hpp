#pragma once
#include <scorum/protocol/betting/game.hpp>
#include <scorum/protocol/betting/market.hpp>

namespace scorum {
namespace protocol {
namespace betting {
void validate_game(const game_type& game, const std::vector<market_type>& markets);
void validate_market(const market_type& market);
}
}
}