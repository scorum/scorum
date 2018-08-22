#include <boost/test/unit_test.hpp>

#include <fc/reflect/reflect.hpp>

#include <scorum/protocol/betting/market.hpp>

#include <scorum/utils/static_variant_serialization.hpp>

#include "defines.hpp"

namespace market_type_tests {

using namespace scorum::protocol;
using namespace scorum::protocol::betting;

struct market_type_fixture
{
    market_type_fixture()
    {
    }

    wincase_pairs_type create_wincase_pairs(const market_type& var)
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

    market_type market1 = result_market{};
    market_type market2 = handicap_market{ 1000 };
    market_type market3 = correct_score_parametrized_market{ 1, 3 };

    SCORUM_MESSAGE("-- Calculate wincase pairs");

    BOOST_REQUIRE_EQUAL(create_wincase_pairs(market1).size(), 3u);

    BOOST_REQUIRE_EQUAL(create_wincase_pairs(market2).size(), 1u);

    BOOST_REQUIRE_EQUAL(create_wincase_pairs(market3).size(), 1u);
}

BOOST_AUTO_TEST_SUITE_END()
}
