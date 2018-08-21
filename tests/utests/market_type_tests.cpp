#include <boost/test/unit_test.hpp>

#include <fc/reflect/reflect.hpp>

#include <scorum/protocol/betting/market.hpp>

#include <boost/fusion/container.hpp>
#include <boost/fusion/algorithm/iteration/for_each.hpp>
#include <boost/fusion/include/for_each.hpp>
#include <boost/container/flat_set.hpp>

#include <scorum/utils/static_variant_serialization.hpp>

#include "defines.hpp"

namespace market_type_tests {

using namespace scorum::protocol;
using namespace scorum::protocol::betting;

template <typename Wincase> constexpr bool check_market_kind(market_kind kind, Wincase&& w)
{
    return kind == std::decay_t<decltype(w)>::kind_v;
}

template <typename Wincase, typename... Wincases>
constexpr bool check_market_kind(market_kind kind, Wincase&& first, Wincases&&... ws)
{
    return check_market_kind(kind, std::forward<Wincase>(first))
        && check_market_kind(kind, std::forward<Wincases>(ws)...);
}

using wincase_pairs_type = fc::flat_set<wincase_pair>;

template <market_kind Kind, typename... Wincases> struct base_market_without_parameters_type
{
    static_assert(check_market_kind(Kind, Wincases{}...), "All wincases should have same market kind");

    static constexpr market_kind kind = Kind;

    base_market_without_parameters_type() = default;

    wincase_pairs_type create_wincase_pairs() const
    {
        wincase_pairs_type result;
        boost::fusion::set<Wincases...> wincases = { Wincases{}... };
        boost::fusion::for_each(wincases, [&](auto w) {
            result.emplace(std::make_pair(wincase_type(w), wincase_type(w.create_opposite())));
        });
        return result;
    }
};

template <market_kind Kind, typename... Wincases> struct base_market_with_threshold_type
{
    static_assert(check_market_kind(Kind, Wincases{ 0 }...), "All wincases should have same market kind");

    static constexpr market_kind kind = Kind;

    threshold_type::value_type threshold = 0;

    base_market_with_threshold_type() = default;

    base_market_with_threshold_type(const threshold_type::value_type threshold_)
        : threshold(threshold_)
    {
    }

    wincase_pairs_type create_wincase_pairs() const
    {
        wincase_pairs_type result;
        boost::fusion::set<Wincases...> wincases = { Wincases{ threshold }... };
        boost::fusion::for_each(wincases, [&](auto w) {
            result.emplace(std::make_pair(wincase_type(w), wincase_type(w.create_opposite())));
        });
        return result;
    }
};

template <market_kind Kind, typename... Wincases> struct base_market_with_score_type
{
    static_assert(check_market_kind(Kind, Wincases{ 0, 0 }...), "All wincases should have same market kind");

    static constexpr market_kind kind = Kind;

    uint16_t home = 0;
    uint16_t away = 0;

    base_market_with_score_type() = default;

    base_market_with_score_type(const int16_t home_, const int16_t away_)
        : home(home_)
        , away(away_)
    {
    }

    wincase_pairs_type create_wincase_pairs() const
    {
        wincase_pairs_type result;
        boost::fusion::set<Wincases...> wincases = { Wincases{ home, away }... };
        boost::fusion::for_each(wincases, [&](auto w) {
            result.emplace(std::make_pair(wincase_type(w), wincase_type(w.create_opposite())));
        });
        return result;
    }
};

// case #1 Market defined by wincase pairs (base case)
//
using test_result_market = base_market_without_parameters_type<market_kind::result,
                                                               result_home,
                                                               result_draw_away,
                                                               result_draw,
                                                               result_home_away,
                                                               result_away,
                                                               result_home_draw>;
using test_handicap_market
    = base_market_with_threshold_type<market_kind::handicap, handicap_home_over, handicap_home_under>;
using test_correct_score_market
    = base_market_with_score_type<market_kind::correct_score, correct_score_yes, correct_score_no>;

// case #2 Market defined by only single wincases (alternative case)
//
using test_goal_market_c0 = base_market_without_parameters_type<market_kind::goal,
                                                                goal_home_yes,
                                                                goal_home_no,
                                                                goal_both_yes,
                                                                goal_both_no,
                                                                goal_away_yes,
                                                                goal_away_no>;

using test_goal_market_c1
    = base_market_without_parameters_type<market_kind::goal, goal_home_yes, goal_both_yes, goal_away_yes>;

using test_goal_market_c2
    = base_market_without_parameters_type<market_kind::goal, goal_home_no, goal_both_no, goal_away_no>;

using test_market_type = fc::static_variant<test_result_market,
                                            test_handicap_market,
                                            test_correct_score_market,
                                            test_goal_market_c0,
                                            test_goal_market_c1,
                                            test_goal_market_c2>;

struct market_type_fixture
{
    market_type_fixture()
    {
    }

    wincase_pairs_type create_wincase_pairs(const test_market_type& var)
    {
        wdump((var));
        wincase_pairs_type result;
        var.visit([&](const auto& market) {
            result = market.create_wincase_pairs();
            for (const auto& wp : result)
            {
                wdump((wp.first));
                wdump((wp.second));
            }
        });
        return result;
    }
};

BOOST_AUTO_TEST_SUITE(market_type_tests)

BOOST_FIXTURE_TEST_CASE(create_wincase_pairs_check, market_type_fixture)
{
    test_market_type market1 = test_result_market{};
    test_market_type market2 = test_handicap_market{ 1000 };
    test_market_type market3 = test_correct_score_market{ 1, 3 };

    SCORUM_MESSAGE("-- Calculate wincase pairs");

    BOOST_REQUIRE_EQUAL(create_wincase_pairs(market1).size(), 3u);

    BOOST_REQUIRE_EQUAL(create_wincase_pairs(market2).size(), 1u);

    BOOST_REQUIRE_EQUAL(create_wincase_pairs(market3).size(), 1u);

    SCORUM_MESSAGE("-- Check same result for same logic but differents definitions");

    test_market_type market_c0 = test_goal_market_c0{};
    test_market_type market_c1 = test_goal_market_c1{};
    test_market_type market_c2 = test_goal_market_c2{};

    BOOST_REQUIRE_EQUAL(create_wincase_pairs(market_c0).size(), 3u);

    BOOST_REQUIRE_EQUAL(create_wincase_pairs(market_c1).size(), 3u);

    BOOST_REQUIRE_EQUAL(create_wincase_pairs(market_c2).size(), 3u);
}

BOOST_AUTO_TEST_SUITE_END()
}

FC_REFLECT_EMPTY(market_type_tests::test_result_market)
FC_REFLECT(market_type_tests::test_handicap_market, (threshold))
FC_REFLECT(market_type_tests::test_correct_score_market, (home)(away))
FC_REFLECT_EMPTY(market_type_tests::test_goal_market_c0)
FC_REFLECT_EMPTY(market_type_tests::test_goal_market_c1)
FC_REFLECT_EMPTY(market_type_tests::test_goal_market_c2)

namespace fc {
using namespace market_type_tests;

void to_variant(const test_market_type& market, fc::variant& var)
{
    scorum::utils::to_variant(market, var);
}
void from_variant(const fc::variant& var, test_market_type& market)
{
    scorum::utils::from_variant(var, market);
}
}
