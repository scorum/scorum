#include <scorum/protocol/betting/market.hpp>
#include <scorum/protocol/betting/game.hpp>

namespace fc {
using namespace scorum::protocol;

class variant;

void to_variant(const wincase_type& wincase, fc::variant& variant);
void from_variant(const fc::variant& variant, wincase_type& wincase);

void to_variant(const market_type& market, fc::variant& var);
void from_variant(const fc::variant& var, market_type& market);

void to_variant(const game_type& game, fc::variant& variant);
void from_variant(const fc::variant& variant, game_type& game);
}
