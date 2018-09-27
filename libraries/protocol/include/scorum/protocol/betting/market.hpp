#pragma once
#include <fc/static_variant.hpp>
#include <scorum/protocol/betting/wincase.hpp>
#include <scorum/protocol/betting/market_kind.hpp>
#include <scorum/protocol/config.hpp>

namespace scorum {
namespace protocol {

template <market_kind kind, typename tag = void> struct over_under_market
{
    int16_t threshold;

    template <bool site> using wincase = over_under_wincase<site, kind, tag>;
    using over = wincase<true>;
    using under = wincase<false>;
    static constexpr market_kind kind_v = kind;

    bool has_trd_state() const
    {
        return threshold % SCORUM_BETTING_THRESHOLD_FACTOR == 0;
    }
};

template <market_kind kind, typename tag = void> struct score_yes_no_market
{
    uint16_t home;
    uint16_t away;

    template <bool site> using wincase = score_yes_no_wincase<site, kind, tag>;
    using yes = wincase<true>;
    using no = wincase<false>;
    static constexpr market_kind kind_v = kind;

    bool has_trd_state() const
    {
        return false;
    }
};

template <market_kind kind, typename tag = void> struct yes_no_market
{
    template <bool site> using wincase = yes_no_wincase<site, kind, tag>;
    using yes = wincase<true>;
    using no = wincase<false>;
    static constexpr market_kind kind_v = kind;

    bool has_trd_state() const
    {
        return false;
    }
};

struct home_tag;
struct away_tag;
struct draw_tag;
struct both_tag;

using result_home = yes_no_market<market_kind::result, home_tag>;
using result_draw = yes_no_market<market_kind::result, draw_tag>;
using result_away = yes_no_market<market_kind::result, away_tag>;

using round_home = yes_no_market<market_kind::round>;

using handicap = over_under_market<market_kind::handicap>;

using correct_score_home = yes_no_market<market_kind::correct_score, home_tag>;
using correct_score_draw = yes_no_market<market_kind::correct_score, draw_tag>;
using correct_score_away = yes_no_market<market_kind::correct_score, away_tag>;
using correct_score = score_yes_no_market<market_kind::correct_score>;

using goal_home = yes_no_market<market_kind::goal, home_tag>;
using goal_both = yes_no_market<market_kind::goal, both_tag>;
using goal_away = yes_no_market<market_kind::goal, away_tag>;

using total = over_under_market<market_kind::total>;

using total_goals_home = over_under_market<market_kind::total_goals, home_tag>;
using total_goals_away = over_under_market<market_kind::total_goals, away_tag>;

using market_type = fc::static_variant<result_home,
                                       result_draw,
                                       result_away,
                                       round_home,
                                       handicap,
                                       correct_score_home,
                                       correct_score_draw,
                                       correct_score_away,
                                       correct_score,
                                       goal_home,
                                       goal_both,
                                       goal_away,
                                       total,
                                       total_goals_home,
                                       total_goals_away>;

using wincase_type = fc::static_variant<result_home::yes,
                                        result_home::no,
                                        result_draw::yes,
                                        result_draw::no,
                                        result_away::yes,
                                        result_away::no,
                                        round_home::yes,
                                        round_home::no,
                                        handicap::over,
                                        handicap::under,
                                        correct_score_home::yes,
                                        correct_score_home::no,
                                        correct_score_draw::yes,
                                        correct_score_draw::no,
                                        correct_score_away::yes,
                                        correct_score_away::no,
                                        correct_score::yes,
                                        correct_score::no,
                                        goal_home::yes,
                                        goal_home::no,
                                        goal_both::yes,
                                        goal_both::no,
                                        goal_away::yes,
                                        goal_away::no,
                                        total::over,
                                        total::under,
                                        total_goals_home::over,
                                        total_goals_home::under,
                                        total_goals_away::over,
                                        total_goals_away::under>;

std::pair<wincase_type, wincase_type> create_wincases(const market_type& market);
wincase_type create_opposite(const wincase_type& wincase);
market_type create_market(const wincase_type& wincase);
bool has_trd_state(const market_type& market);
bool match_wincases(const wincase_type& lhs, const wincase_type& rhs);
}
}

#include <scorum/protocol/betting/wincase_comparison.hpp>
#include <scorum/protocol/betting/market_comparison.hpp>

FC_REFLECT_EMPTY(scorum::protocol::result_home)
FC_REFLECT_EMPTY(scorum::protocol::result_draw)
FC_REFLECT_EMPTY(scorum::protocol::result_away)
FC_REFLECT_EMPTY(scorum::protocol::round_home)
FC_REFLECT(scorum::protocol::handicap, (threshold))
FC_REFLECT_EMPTY(scorum::protocol::correct_score_home)
FC_REFLECT_EMPTY(scorum::protocol::correct_score_draw)
FC_REFLECT_EMPTY(scorum::protocol::correct_score_away)
FC_REFLECT(scorum::protocol::correct_score, (home)(away))
FC_REFLECT_EMPTY(scorum::protocol::goal_home)
FC_REFLECT_EMPTY(scorum::protocol::goal_both)
FC_REFLECT_EMPTY(scorum::protocol::goal_away)
FC_REFLECT(scorum::protocol::total, (threshold))
FC_REFLECT(scorum::protocol::total_goals_home, (threshold))
FC_REFLECT(scorum::protocol::total_goals_away, (threshold))

FC_REFLECT_EMPTY(scorum::protocol::result_home::yes)
FC_REFLECT_EMPTY(scorum::protocol::result_home::no)
FC_REFLECT_EMPTY(scorum::protocol::result_draw::yes)
FC_REFLECT_EMPTY(scorum::protocol::result_draw::no)
FC_REFLECT_EMPTY(scorum::protocol::result_away::yes)
FC_REFLECT_EMPTY(scorum::protocol::result_away::no)
FC_REFLECT_EMPTY(scorum::protocol::round_home::yes)
FC_REFLECT_EMPTY(scorum::protocol::round_home::no)
FC_REFLECT(scorum::protocol::handicap::over, (threshold))
FC_REFLECT(scorum::protocol::handicap::under, (threshold))
FC_REFLECT_EMPTY(scorum::protocol::correct_score_home::yes)
FC_REFLECT_EMPTY(scorum::protocol::correct_score_home::no)
FC_REFLECT_EMPTY(scorum::protocol::correct_score_draw::yes)
FC_REFLECT_EMPTY(scorum::protocol::correct_score_draw::no)
FC_REFLECT_EMPTY(scorum::protocol::correct_score_away::yes)
FC_REFLECT_EMPTY(scorum::protocol::correct_score_away::no)
FC_REFLECT(scorum::protocol::correct_score::yes, (home)(away))
FC_REFLECT(scorum::protocol::correct_score::no, (home)(away))
FC_REFLECT_EMPTY(scorum::protocol::goal_home::yes)
FC_REFLECT_EMPTY(scorum::protocol::goal_home::no)
FC_REFLECT_EMPTY(scorum::protocol::goal_both::yes)
FC_REFLECT_EMPTY(scorum::protocol::goal_both::no)
FC_REFLECT_EMPTY(scorum::protocol::goal_away::yes)
FC_REFLECT_EMPTY(scorum::protocol::goal_away::no)
FC_REFLECT(scorum::protocol::total::over, (threshold))
FC_REFLECT(scorum::protocol::total::under, (threshold))
FC_REFLECT(scorum::protocol::total_goals_home::over, (threshold))
FC_REFLECT(scorum::protocol::total_goals_home::under, (threshold))
FC_REFLECT(scorum::protocol::total_goals_away::over, (threshold))
FC_REFLECT(scorum::protocol::total_goals_away::under, (threshold))
