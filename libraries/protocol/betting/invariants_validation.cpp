#include <boost/fusion/include/map.hpp>
#include <boost/fusion/include/at_key.hpp>
#include <boost/range/algorithm/set_algorithm.hpp>
#include <boost/range/algorithm/transform.hpp>
#include <scorum/protocol/betting/invariants_validation.hpp>

namespace scorum {
namespace protocol {
namespace betting {

namespace bf = boost::fusion;

const bf::map<bf::pair<soccer_game, std::set<market_kind>>, bf::pair<hockey_game, std::set<market_kind>>>
    game_markets(bf::make_pair<soccer_game>(std::set<market_kind>{ market_kind::result, market_kind::round,
                                                                   market_kind::handicap, market_kind::correct_score,
                                                                   market_kind::goal, market_kind::total }),
                 bf::make_pair<hockey_game>(std::set<market_kind>{ market_kind::result, market_kind::round,
                                                                   market_kind::handicap, market_kind::correct_score,
                                                                   market_kind::goal, market_kind::total }));

void validate_game(const game_type& game, const std::vector<market_type>& markets)
{
    const auto& expected_markets
        = game.visit([&](const auto& g) { return bf::at_key<std::decay_t<decltype(g)>>(game_markets); });

    std::set<market_kind> market_kinds;
    boost::transform(markets, std::inserter(market_kinds, market_kinds.begin()),
                     [](const market_type& m) { return m.kind; });

    std::vector<market_kind> diff;
    boost::set_difference(market_kinds, expected_markets, std::back_inserter(diff));

    FC_ASSERT(diff.empty(), "Markets [$(m)] cannot be used with specified game", ("m", diff));
}

void validate_markets(const std::vector<market_type>& markets)
{
    FC_ASSERT((!markets.empty()), "Markets list cannot be empty");

    for (const auto& market : markets)
        validate_market(market);
}

void validate_market(const market_type& market)
{
    FC_ASSERT((!market.wincases.empty()), "Wincases list cannot be empty (market ${m})", ("m", market.kind));

    for (const auto& wincase : market.wincases)
    {
        auto market_from_wincase = wincase.visit([](auto w) { return decltype(w)::kind_v; });
        FC_ASSERT(market_from_wincase == market.kind,
                  "Market ${wm} from wincase ${w} doesn't equal specified market ${m}",
                  ("w", wincase)("wm", market_from_wincase)("m", market.kind));
    }
}
}
}
}