#include <boost/fusion/include/map.hpp>
#include <boost/fusion/include/at_key.hpp>
#include <boost/range/algorithm/set_algorithm.hpp>
#include <boost/range/algorithm/transform.hpp>
#include <scorum/protocol/betting/invariants_validation.hpp>
#include <scorum/protocol/betting/wincase_serialization.hpp>
#include <scorum/protocol/betting/wincase_comparison.hpp>

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

void validate_game(const game_type& game, const fc::flat_set<market_type>& markets)
{
    const auto& expected_markets
        = game.visit([&](const auto& g) { return bf::at_key<std::decay_t<decltype(g)>>(game_markets); });

    std::vector<market_kind> market_kinds;
    boost::transform(markets, std::back_inserter(market_kinds), [](const market_type& m) { return m.kind; });

    std::vector<market_kind> diff;
    boost::set_difference(market_kinds, expected_markets, std::back_inserter(diff));

    FC_ASSERT(diff.empty(), "Markets [$(m)] cannot be used with specified game", ("m", diff));
}

void validate_markets(const fc::flat_set<market_type>& markets)
{
    FC_ASSERT((!markets.empty()), "Markets list cannot be empty");

    for (const auto& market : markets)
        validate_market(market);
}

void validate_market(const market_type& market)
{
    FC_ASSERT((!market.wincases.empty()), "Wincases list cannot be empty (market '${m}')", ("m", market.kind));

    for (const auto& pair : market.wincases)
    {
        validate_wincase(pair.first, market.kind);
        validate_wincase(pair.second, market.kind);

        auto is_valid_pair = pair.first.visit([&](const auto& fst) { return fst.create_opposite() == pair.second; });

        FC_ASSERT(is_valid_pair, "Wincases '${1}' and '${2}' cannot be paired", ("1", pair.first)("2", pair.second));
    }
}

void validate_wincase(const wincase_type& wincase, market_kind market)
{
    auto market_from_wincase = wincase.visit([](const auto& w) { return std::decay_t<decltype(w)>::kind_v; });
    FC_ASSERT(market_from_wincase == market, "Market '${wm}' from wincase doesn't equal specified market '${m}'",
              ("wm", market_from_wincase)("m", market));

    auto check_threshold = [](auto threshold) { return threshold.value % (threshold_type::factor / 2) == 0; };
    auto check_positive_threshold = [&](auto threshold) { return check_threshold(threshold) && threshold > 0; };

    auto is_valid
        = wincase.weak_visit([&](const total_over& w) { return check_positive_threshold(w.threshold); },
                             [&](const total_under& w) { return check_positive_threshold(w.threshold); },
                             [&](const handicap_home_over& w) { return check_threshold(w.threshold); },
                             [&](const handicap_home_under& w) { return check_threshold(w.threshold); },
                             [&](const total_goals_home_over& w) { return check_positive_threshold(w.threshold); },
                             [&](const total_goals_home_under& w) { return check_positive_threshold(w.threshold); },
                             [&](const total_goals_away_over& w) { return check_positive_threshold(w.threshold); },
                             [&](const total_goals_away_under& w) { return check_positive_threshold(w.threshold); });

    FC_ASSERT(is_valid.value_or(true), "Wincase '${w}' is invalid", ("w", wincase));
}
}
}
}