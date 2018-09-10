#include <scorum/protocol/betting/betting_serialization.hpp>
#include <scorum/utils/static_variant_serialization.hpp>

namespace fc {

using namespace scorum;

void to_variant(const wincase_type& wincase, fc::variant& var)
{
    utils::to_variant(wincase, var);
}
void from_variant(const fc::variant& var, wincase_type& wincase)
{
    utils::from_variant(var, wincase);
}

void to_variant(const market_type& market, fc::variant& var)
{
    utils::to_variant(market, var);
}
void from_variant(const fc::variant& var, market_type& market)
{
    utils::from_variant(var, market);
}

void to_variant(const game_type& game, fc::variant& var)
{
    utils::to_variant(game, var);
}
void from_variant(const fc::variant& var, game_type& game)
{
    utils::from_variant(var, game);
}
}
