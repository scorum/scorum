#pragma once
#include <scorum/protocol/betting/wincase.hpp>
#include <scorum/protocol/betting/wincase_comparison.hpp>

namespace scorum {
namespace protocol {
namespace betting {

/// This check if strict_wincase_pair_type has valid market kind
template <typename WincasePair> constexpr bool check_market_kind(market_kind kind, WincasePair&& w)
{
    // Only one check for left_wincase is necessary because strict_wincase_pair_type type
    // guarantees that left_wincase and right_wincase have the same market kind
    return kind == std::decay_t<decltype(w)>::left_wincase::kind_v;
}

template <typename WincasePair, typename... WincasePairs>
constexpr bool check_market_kind(market_kind kind, WincasePair&& first, WincasePairs&&... ws)
{
    return check_market_kind(kind, std::forward<WincasePair>(first))
        && check_market_kind(kind, std::forward<WincasePairs>(ws)...);
}

using wincase_pairs_type = fc::flat_set<wincase_pair>;

template <class Implementation, market_kind Kind, typename... Wincases> struct base_market_type
{
    static_assert(check_market_kind(Kind, Wincases{}...), "All wincases should have same market kind");

    static constexpr market_kind kind = Kind;

    virtual ~base_market_type()
    {
    }

    wincase_pairs_type create_wincase_pairs() const
    {
        wincase_pairs_type result;
        boost::fusion::set<Wincases...> wincase_ps;
        boost::fusion::for_each(wincase_ps, [&](auto wp) {
            auto wl = dynamic_cast<const Implementation*>(this)->create_wincase(decltype(wp){});
            result.emplace(std::make_pair(wincase_type(wl), wincase_type(wl.create_opposite())));
        });
        return result;
    }

protected:
    base_market_type() = default;
};

template <market_kind Kind, typename... Wincases>
struct base_market_without_parameters_type
    : public base_market_type<base_market_without_parameters_type<Kind, Wincases...>, Kind, Wincases...>
{
    base_market_without_parameters_type() = default;

    template <typename WincasePair> auto create_wincase(WincasePair&&) const
    {
        typename WincasePair::left_wincase result{};
        return result;
    }
};

template <market_kind Kind, typename... Wincases>
struct base_market_with_threshold_type
    : public base_market_type<base_market_with_threshold_type<Kind, Wincases...>, Kind, Wincases...>
{
    threshold_type::value_type threshold = 0;

    base_market_with_threshold_type() = default;

    base_market_with_threshold_type(const threshold_type::value_type threshold_)
        : threshold(threshold_)
    {
    }

    template <typename WincasePair> auto create_wincase(WincasePair&&) const
    {
        typename WincasePair::left_wincase result{ threshold };
        return result;
    }
};

template <market_kind Kind, typename... Wincases>
struct base_market_with_score_type
    : public base_market_type<base_market_with_score_type<Kind, Wincases...>, Kind, Wincases...>
{
    uint16_t home = 0;
    uint16_t away = 0;

    base_market_with_score_type() = default;

    base_market_with_score_type(const int16_t home_, const int16_t away_)
        : home(home_)
        , away(away_)
    {
    }

    template <typename WincasePair> auto create_wincase(WincasePair&&) const
    {
        typename WincasePair::left_wincase result{ home, away };
        return result;
    }
};

using result_market = base_market_without_parameters_type<market_kind::result,
                                                          WINCASE(result_home, result_draw_away),
                                                          WINCASE(result_draw, result_home_away),
                                                          WINCASE(result_away, result_home_draw)>;

using round_market = base_market_without_parameters_type<market_kind::round, WINCASE(round_home, round_away)>;

using handicap_market
    = base_market_with_threshold_type<market_kind::handicap, WINCASE(handicap_home_over, handicap_home_under)>;

using correct_score_market
    = base_market_without_parameters_type<market_kind::correct_score,
                                          WINCASE(correct_score_home_yes, correct_score_home_no),
                                          WINCASE(correct_score_draw_yes, correct_score_draw_no),
                                          WINCASE(correct_score_away_yes, correct_score_away_no)>;

using correct_score_parametrized_market = base_market_with_score_type<market_kind::correct_score_parametrized,
                                                                      WINCASE(correct_score_yes, correct_score_no)>;

using goal_market = base_market_without_parameters_type<market_kind::goal,
                                                        WINCASE(goal_home_yes, goal_home_no),
                                                        WINCASE(goal_both_yes, goal_both_no),
                                                        WINCASE(goal_away_yes, goal_away_no)>;

using total_market = base_market_with_threshold_type<market_kind::total, WINCASE(total_over, total_under)>;

using total_goals_market = base_market_with_threshold_type<market_kind::total_goals,
                                                           WINCASE(total_goals_home_over, total_goals_home_under),
                                                           WINCASE(total_goals_away_over, total_goals_away_under)>;

using market_type = fc::static_variant<result_market,
                                       round_market,
                                       handicap_market,
                                       correct_score_market,
                                       correct_score_parametrized_market,
                                       goal_market,
                                       total_market,
                                       total_goals_market>;
}
}
}

namespace std {
using namespace scorum::protocol::betting;
template <> struct less<market_type>
{
    bool operator()(const market_type& lhs, const market_type& rhs) const
    {
        return lhs.kind < rhs.kind;
    }
};
}

FC_REFLECT_EMPTY(scorum::protocol::betting::result_market)
FC_REFLECT_EMPTY(scorum::protocol::betting::round_market)
FC_REFLECT(scorum::protocol::betting::handicap_market, (threshold))
FC_REFLECT_EMPTY(scorum::protocol::betting::correct_score_market)
FC_REFLECT(scorum::protocol::betting::correct_score_parametrized_market, (home)(away))
FC_REFLECT_EMPTY(scorum::protocol::betting::goal_market)
FC_REFLECT(scorum::protocol::betting::total_market, (threshold))
FC_REFLECT(scorum::protocol::betting::total_goals_market, (threshold))
