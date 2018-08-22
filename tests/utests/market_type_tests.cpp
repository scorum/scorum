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
    // WINCASE(total_goals_home_over, result_home_away) w; <- should raise static_assertion
    // WINCASE(total_goals_home_over, total_goals_home_over) w; <- should raise static_assertion
    // WINCASE(total_goals_home_over, handicap_home_under) <- should raise static_assertion
    WINCASE(total_goals_home_over, total_goals_home_under) wp;
    auto w = decltype(wp)::left_wincase{};
    wdump((wincase_type(w)));

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
