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

// clang-format off
const bf::map<bf::pair<soccer_game, std::set<market_kind>>, bf::pair<hockey_game, std::set<market_kind>>> game_markets(
    bf::make_pair<soccer_game>(std::set<market_kind>{
                                        market_kind::result,
                                        market_kind::round,
                                        market_kind::handicap,
                                        market_kind::correct_score,
                                        market_kind::goal,
                                        market_kind::total,
                                        market_kind::total_goals
                                                    }),
    bf::make_pair<hockey_game>(std::set<market_kind>{ //This type for tests purpose only. Now we have markets table for soccer only
                                        market_kind::result,
                                        market_kind::goal
                                                    }));
// clang-format on

void validate_game(const game_type& game, const fc::flat_set<market_type>& markets)
{
    const auto& expected_markets
        = game.visit([&](const auto& g) { return bf::at_key<std::decay_t<decltype(g)>>(game_markets); });

    std::vector<market_kind> actual_markets;
    boost::transform(markets, std::back_inserter(actual_markets), [](const market_type& m) {
        market_kind kind;
        m.visit([&](const auto& market_impl) { kind = market_impl.kind; });
        return kind;
    });

    std::vector<market_kind> diff;
    boost::set_difference(actual_markets, expected_markets, std::back_inserter(diff));

    FC_ASSERT(diff.empty(), "Markets [${m}] cannot be used with specified game", ("m", diff));
}

void validate_wincases(const fc::flat_set<wincase_type>& wincases)
{
    for (const auto& wincase : wincases)
    {
        validate_wincase(wincase);
    }
}

void validate_wincase(const wincase_type& wincase)
{
    auto check_threshold
        = [](auto threshold) { return threshold_type(threshold).value % (threshold_type::factor / 2) == 0; };
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

void validate_if_wincase_in_game(const game_type& game, const wincase_type& wincase)
{
    const auto& expected_markets
        = game.visit([&](const auto& g) { return bf::at_key<std::decay_t<decltype(g)>>(game_markets); });

    auto market_kind = wincase.visit([](const auto& w) { return std::decay_t<decltype(w)>::kind_v; });

    FC_ASSERT(expected_markets.find(market_kind) != expected_markets.end(), "Wincase '${w}' doesn't belong game '${g}'",
              ("w", wincase)("g", game));
}

void validate_bet_ids(const fc::flat_set<int64_t>& bet_ids)
{
    FC_ASSERT(!bet_ids.empty());
    for (const auto& id : bet_ids)
    {
        FC_ASSERT(id >= 0, "Invalid bet Id");
    }
}
}
}
}
