#pragma once
#include <fc/static_variant.hpp>
#include <scorum/protocol/betting/market_kind.hpp>
#include <scorum/protocol/betting/threshold.hpp>

namespace scorum {
namespace protocol {
namespace betting {

struct home_tag;
struct away_tag;
struct draw_tag;
struct both_tag;

template <bool side, market_kind kind, typename tag = void> struct over_under
{
    static constexpr market_kind kind_v = kind;
    using opposite_type = over_under<!side, kind, tag>;

    threshold_type threshold = 0;

    over_under() = default;
    over_under(const threshold_type::value_type threshold_)
        : threshold(threshold_)
    {
    }

    opposite_type create_opposite() const
    {
        return opposite_type{ threshold.value };
    }

    bool has_trd_state() const
    {
        return threshold.value % threshold_type::factor == 0;
    }
};

template <bool side, market_kind kind, typename tag = void> struct yes_no
{
    static constexpr market_kind kind_v = kind;
    using opposite_type = yes_no<!side, kind, tag>;

    opposite_type create_opposite() const
    {
        return opposite_type{};
    }

    bool has_trd_state() const
    {
        return false;
    }
};

template <bool side, market_kind kind, typename tag = void> struct score_yes_no
{
    static constexpr market_kind kind_v = kind;
    using opposite_type = score_yes_no<!side, kind, tag>;

    uint16_t home;
    uint16_t away;

    opposite_type create_opposite() const
    {
        return opposite_type{ home, away };
    }

    bool has_trd_state() const
    {
        return false;
    }
};

template <market_kind kind, typename tag = void> using over = over_under<true, kind, tag>;
template <market_kind kind, typename tag = void> using under = over_under<false, kind, tag>;
template <market_kind kind, typename tag = void> using yes = yes_no<true, kind, tag>;
template <market_kind kind, typename tag = void> using no = yes_no<false, kind, tag>;
template <market_kind kind, typename tag = void> using score_yes = score_yes_no<true, kind, tag>;
template <market_kind kind, typename tag = void> using score_no = score_yes_no<false, kind, tag>;

using result_home = yes<market_kind::result, home_tag>;
using result_draw_away = no<market_kind::result, home_tag>;
using result_draw = yes<market_kind::result, draw_tag>;
using result_home_away = no<market_kind::result, draw_tag>;
using result_away = yes<market_kind::result, away_tag>;
using result_home_draw = no<market_kind::result, away_tag>;

using round_home = yes<market_kind::round, home_tag>;
using round_away = no<market_kind::round, home_tag>;

using handicap_home_over = over<market_kind::handicap>;
using handicap_home_under = under<market_kind::handicap>;

using correct_score_yes = score_yes<market_kind::correct_score>;
using correct_score_no = score_no<market_kind::correct_score>;
using correct_score_home_yes = yes<market_kind::correct_score, home_tag>;
using correct_score_home_no = no<market_kind::correct_score, home_tag>;
using correct_score_draw_yes = yes<market_kind::correct_score, draw_tag>;
using correct_score_draw_no = no<market_kind::correct_score, draw_tag>;
using correct_score_away_yes = yes<market_kind::correct_score, away_tag>;
using correct_score_away_no = no<market_kind::correct_score, away_tag>;

using goal_home_yes = yes<market_kind::goal, home_tag>;
using goal_home_no = no<market_kind::goal, home_tag>;
using goal_both_yes = yes<market_kind::goal, both_tag>;
using goal_both_no = no<market_kind::goal, both_tag>;
using goal_away_yes = yes<market_kind::goal, away_tag>;
using goal_away_no = no<market_kind::goal, away_tag>;

using total_over = over<market_kind::total>;
using total_under = under<market_kind::total>;

using total_goals_home_over = over<market_kind::total_goals, home_tag>;
using total_goals_home_under = under<market_kind::total_goals, home_tag>;
using total_goals_away_over = over<market_kind::total_goals, away_tag>;
using total_goals_away_under = under<market_kind::total_goals, away_tag>;

using wincase_type = fc::static_variant<result_home,
                                        result_draw,
                                        result_away,
                                        result_draw_away,
                                        result_home_away,
                                        result_home_draw,
                                        round_home,
                                        round_away,
                                        handicap_home_over,
                                        handicap_home_under,
                                        correct_score_yes,
                                        correct_score_no,
                                        correct_score_home_yes,
                                        correct_score_home_no,
                                        correct_score_draw_yes,
                                        correct_score_draw_no,
                                        correct_score_away_yes,
                                        correct_score_away_no,
                                        goal_home_yes,
                                        goal_home_no,
                                        goal_both_yes,
                                        goal_both_no,
                                        goal_away_yes,
                                        goal_away_no,
                                        total_over,
                                        total_under,
                                        // v2.0
                                        total_goals_home_over,
                                        total_goals_home_under,
                                        total_goals_away_over,
                                        total_goals_away_under>;

using wincase_pair = std::pair<wincase_type, wincase_type>;
}
}
}

FC_REFLECT_EMPTY(scorum::protocol::betting::result_home)
FC_REFLECT_EMPTY(scorum::protocol::betting::result_draw)
FC_REFLECT_EMPTY(scorum::protocol::betting::result_away)
FC_REFLECT_EMPTY(scorum::protocol::betting::result_draw_away)
FC_REFLECT_EMPTY(scorum::protocol::betting::result_home_away)
FC_REFLECT_EMPTY(scorum::protocol::betting::result_home_draw)
FC_REFLECT_EMPTY(scorum::protocol::betting::round_home)
FC_REFLECT_EMPTY(scorum::protocol::betting::round_away)
FC_REFLECT(scorum::protocol::betting::handicap_home_over, (threshold))
FC_REFLECT(scorum::protocol::betting::handicap_home_under, (threshold))
FC_REFLECT(scorum::protocol::betting::correct_score_yes, (home)(away))
FC_REFLECT(scorum::protocol::betting::correct_score_no, (home)(away))
FC_REFLECT_EMPTY(scorum::protocol::betting::correct_score_home_yes)
FC_REFLECT_EMPTY(scorum::protocol::betting::correct_score_home_no)
FC_REFLECT_EMPTY(scorum::protocol::betting::correct_score_draw_yes)
FC_REFLECT_EMPTY(scorum::protocol::betting::correct_score_draw_no)
FC_REFLECT_EMPTY(scorum::protocol::betting::correct_score_away_yes)
FC_REFLECT_EMPTY(scorum::protocol::betting::correct_score_away_no)
FC_REFLECT_EMPTY(scorum::protocol::betting::goal_home_yes)
FC_REFLECT_EMPTY(scorum::protocol::betting::goal_home_no)
FC_REFLECT_EMPTY(scorum::protocol::betting::goal_both_yes)
FC_REFLECT_EMPTY(scorum::protocol::betting::goal_both_no)
FC_REFLECT_EMPTY(scorum::protocol::betting::goal_away_yes)
FC_REFLECT_EMPTY(scorum::protocol::betting::goal_away_no)
FC_REFLECT(scorum::protocol::betting::total_over, (threshold))
FC_REFLECT(scorum::protocol::betting::total_under, (threshold))
FC_REFLECT(scorum::protocol::betting::total_goals_home_over, (threshold))
FC_REFLECT(scorum::protocol::betting::total_goals_home_under, (threshold))
FC_REFLECT(scorum::protocol::betting::total_goals_away_over, (threshold))
FC_REFLECT(scorum::protocol::betting::total_goals_away_under, (threshold))
FC_REFLECT_TYPENAME(scorum::protocol::betting::wincase_pair)
