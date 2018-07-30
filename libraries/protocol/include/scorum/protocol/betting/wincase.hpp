#pragma once
#include <fc/static_variant.hpp>
#include <scorum/protocol/betting/market_kind.hpp>

namespace scorum {
namespace protocol {
namespace betting {

struct home_tag
{
};
struct away_tag
{
};
struct draw_tag
{
};
struct both_tag
{
};

template <market_kind kind, typename tag = void> struct under;
template <market_kind kind, typename tag = void> struct over
{
    static constexpr market_kind kind_v = kind;
    using opposite_type = under<kind, tag>;

    float threshold;
};

template <market_kind kind, typename tag> struct under
{
    static constexpr market_kind kind_v = kind;
    using opposite_type = over<kind, tag>;

    float threshold;
};

template <market_kind kind, typename tag = void> struct no;
template <market_kind kind, typename tag = void> struct yes
{
    static constexpr market_kind kind_v = kind;
    using opposite_type = no<kind, tag>;
};

template <market_kind kind, typename tag> struct no
{
    static constexpr market_kind kind_v = kind;
    using opposite_type = yes<kind, tag>;
};

template <market_kind kind> struct score_yes;
template <market_kind kind> struct score_no
{
    static constexpr market_kind kind_v = kind;
    using opposite_type = score_yes<kind>;

    int16_t home;
    int16_t away;
};

template <market_kind kind> struct score_yes
{
    static constexpr market_kind kind_v = kind;
    using opposite_type = score_no<kind>;

    int16_t home;
    int16_t away;
};

struct result_draw_away;
struct result_home
{
    static constexpr market_kind kind_v = market_kind::result;
    using opposite_type = result_draw_away;
};

struct result_home_away;
struct result_draw
{
    static constexpr market_kind kind_v = market_kind::result;
    using opposite_type = result_home_away;
};

struct result_home_draw;
struct result_away
{
    static constexpr market_kind kind_v = market_kind::result;
    using opposite_type = result_home_draw;
};

struct result_draw_away
{
    static constexpr market_kind kind_v = market_kind::result;
    using opposite_type = result_home;
};

struct result_home_away
{
    static constexpr market_kind kind_v = market_kind::result;
    using opposite_type = result_draw;
};

struct result_home_draw
{
    static constexpr market_kind kind_v = market_kind::result;
    using opposite_type = result_away;
};

struct round_home;
struct round_away
{
    static constexpr market_kind kind_v = market_kind::round;
    using opposite_type = round_home;
};

struct round_home
{
    static constexpr market_kind kind_v = market_kind::round;
    using opposite_type = round_away;
};

using handicap_over = over<market_kind::handicap>;
using handicap_under = under<market_kind::handicap>;

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
                                        handicap_over,
                                        handicap_under,
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
                                        total_goals_home_over,
                                        total_goals_home_under,
                                        total_goals_away_over,
                                        total_goals_away_under>;
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
FC_REFLECT_EMPTY(scorum::protocol::betting::handicap_over)
FC_REFLECT_EMPTY(scorum::protocol::betting::handicap_under)
FC_REFLECT_EMPTY(scorum::protocol::betting::correct_score_yes)
FC_REFLECT_EMPTY(scorum::protocol::betting::correct_score_no)
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
FC_REFLECT_EMPTY(scorum::protocol::betting::total_over)
FC_REFLECT_EMPTY(scorum::protocol::betting::total_under)
FC_REFLECT_EMPTY(scorum::protocol::betting::total_goals_home_over)
FC_REFLECT_EMPTY(scorum::protocol::betting::total_goals_home_under)
FC_REFLECT_EMPTY(scorum::protocol::betting::total_goals_away_over)
FC_REFLECT_EMPTY(scorum::protocol::betting::total_goals_away_under)

namespace fc {
class variant;
void to_variant(const scorum::protocol::betting::wincase_type& wincase, fc::variant& variant);
}