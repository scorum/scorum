#include <boost/test/unit_test.hpp>

#include <initializer_list>

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

template <typename LeftWincase, typename RightWincase> struct strict_wincase_pair_type
{
    static_assert(LeftWincase::kind_v == RightWincase::kind_v, "Left and right wincases should have same market kind");
    static_assert(std::is_same<LeftWincase, typename RightWincase::opposite_type>::value,
                  "Left and right wincases should be opposite each other");

    using left_wincase = LeftWincase;
    using right_wincase = RightWincase;

    constexpr strict_wincase_pair_type() = default;
};

template <typename Wincase> constexpr bool check_market_kind(market_kind kind, Wincase&& w)
{
    return kind == std::decay_t<decltype(w)>::left_wincase::kind_v; // second check for right_wincase is not necessary
}

template <typename Wincase, typename... Wincases>
constexpr bool check_market_kind(market_kind kind, Wincase&& first, Wincases&&... ws)
{
    return check_market_kind(kind, std::forward<Wincase>(first))
        && check_market_kind(kind, std::forward<Wincases>(ws)...);
}

using wincase_pairs_type = fc::flat_set<wincase_pair>;

#define WINCASE(l, r) strict_wincase_pair_type<l, r>

template <market_kind Kind, typename... Wincases> struct base_market_type
{
    static_assert(check_market_kind(Kind, Wincases{}...), "All wincases should have same market kind");

    static constexpr market_kind kind = Kind;

protected:
    base_market_type() = default;
};

template <market_kind Kind, typename... Wincases>
struct base_market_without_parameters_type : public base_market_type<Kind, Wincases...>
{
    base_market_without_parameters_type() = default;

    wincase_pairs_type create_wincase_pairs() const
    {
        wincase_pairs_type result;
        boost::fusion::set<Wincases...> wincases;
        boost::fusion::for_each(wincases, [&](auto w) {
            typename decltype(w)::left_wincase wl{};
            result.emplace(std::make_pair(wincase_type(wl), wincase_type(wl.create_opposite())));
        });
        return result;
    }
};

template <market_kind Kind, typename... Wincases>
struct base_market_with_threshold_type : public base_market_type<Kind, Wincases...>
{
    threshold_type::value_type threshold = 0;

    base_market_with_threshold_type() = default;

    base_market_with_threshold_type(const threshold_type::value_type threshold_)
        : threshold(threshold_)
    {
    }

    wincase_pairs_type create_wincase_pairs() const
    {
        wincase_pairs_type result;
        boost::fusion::set<Wincases...> wincases;
        boost::fusion::for_each(wincases, [&](auto w) {
            typename decltype(w)::left_wincase wl{ threshold };
            result.emplace(std::make_pair(wincase_type(wl), wincase_type(wl.create_opposite())));
        });
        return result;
    }
};

template <market_kind Kind, typename... Wincases>
struct base_market_with_score_type : public base_market_type<Kind, Wincases...>
{
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
        boost::fusion::set<Wincases...> wincases;
        boost::fusion::for_each(wincases, [&](auto w) {
            typename decltype(w)::left_wincase wl{ home, away };
            result.emplace(std::make_pair(wincase_type(wl), wincase_type(wl.create_opposite())));
        });
        return result;
    }
};

using test_result_market = base_market_without_parameters_type<market_kind::result,
                                                               WINCASE(result_home, result_draw_away),
                                                               WINCASE(result_draw, result_home_away),
                                                               WINCASE(result_away, result_home_draw)>;
using test_handicap_market
    = base_market_with_threshold_type<market_kind::handicap, WINCASE(handicap_home_over, handicap_home_under)>;
using test_correct_score_market = base_market_with_score_type<market_kind::correct_score_parametrized,
                                                              WINCASE(correct_score_yes, correct_score_no)>;

using test_market_type = fc::static_variant<test_result_market, test_handicap_market, test_correct_score_market>;

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
    // WINCASE(result_home, result_home_away) w; <- should raise static_assertion
    // WINCASE(result_home, result_home) w; <- should raise static_assertion
    // WINCASE(result_home, correct_score_home_no) <- should raise static_assertion
    WINCASE(result_home, result_draw_away) w;
    decltype(w)::left_wincase{};

    test_market_type market1 = test_result_market{};
    test_market_type market2 = test_handicap_market{ 1000 };
    test_market_type market3 = test_correct_score_market{ 1, 3 };

    SCORUM_MESSAGE("-- Calculate wincase pairs");

    BOOST_REQUIRE_EQUAL(create_wincase_pairs(market1).size(), 3u);

    BOOST_REQUIRE_EQUAL(create_wincase_pairs(market2).size(), 1u);

    BOOST_REQUIRE_EQUAL(create_wincase_pairs(market3).size(), 1u);
}

BOOST_AUTO_TEST_SUITE_END()
}

FC_REFLECT_EMPTY(market_type_tests::test_result_market)
FC_REFLECT(market_type_tests::test_handicap_market, (threshold))
FC_REFLECT(market_type_tests::test_correct_score_market, (home)(away))

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
