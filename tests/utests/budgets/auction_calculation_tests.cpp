#include <boost/test/unit_test.hpp>

#include <scorum/chain/database/budget_management_algorithms.hpp>

#include <scorum/protocol/types.hpp>
#include <scorum/protocol/asset.hpp>

#include "defines.hpp"

#include <vector>

namespace auction_calculation_tests {

using namespace scorum::chain;
using namespace scorum::protocol;

struct auction_calculation_fixture
{
    auction_calculation_fixture()
        : default_position_weights{ 100, 85, 75, 65 }
        , default_per_block_values{ ASSET_SCR(10e+9), ASSET_SCR(7e+9), ASSET_SCR(5e+9), ASSET_SCR(3e+9),
                                    ASSET_SCR(2e+9) }
    {
    }

    percent_type next_decreasing_percent(const percent_type current, const percent_type delta = 15)
    {
        BOOST_REQUIRE_GT(delta, 0);
        BOOST_REQUIRE_LT(delta, 100);
        return current - current * delta / 100;
    }

    asset next_decreasing_per_block(const asset& current, const share_type delta = ASSET_SCR(0.5e+9).amount)
    {
        BOOST_REQUIRE_GT(delta, 0);
        BOOST_REQUIRE_LT(delta, current.amount);
        return current - delta;
    }

    using position_weights_type = std::vector<percent_type>;
    using per_block_values_type = std::vector<asset>;

    const position_weights_type default_position_weights;
    const per_block_values_type default_per_block_values;
};

BOOST_FIXTURE_TEST_SUITE(auction_calculation_check, auction_calculation_fixture)

SCORUM_TEST_CASE(invalid_input_check)
{
    BOOST_TEST_MESSAGE("Check invalid coefficient's list");

    SCORUM_CHECK_THROW(calculate_auction_bets(default_per_block_values, position_weights_type()), fc::assert_exception);

    BOOST_TEST_MESSAGE("Check invalid (negative) coefficient value");

    position_weights_type invalid_position_weights = default_position_weights;
    (*invalid_position_weights.rbegin()) *= -1;
    SCORUM_CHECK_THROW(calculate_auction_bets(per_block_values_type(), invalid_position_weights), fc::assert_exception);

    BOOST_TEST_MESSAGE("Check invalid (disrupted sorting) coefficient value");

    invalid_position_weights = default_position_weights;
    auto half_position = default_position_weights.size() / 2;
    std::swap(invalid_position_weights[0], invalid_position_weights[half_position]);
    SCORUM_CHECK_THROW(calculate_auction_bets(per_block_values_type(), invalid_position_weights), fc::assert_exception);

    BOOST_TEST_MESSAGE("Check invalid (negative) per-block value");

    per_block_values_type invalid_per_block_values = default_per_block_values;
    (*invalid_per_block_values.rbegin()) *= -1;
    SCORUM_CHECK_THROW(calculate_auction_bets(invalid_per_block_values, default_position_weights),
                       fc::assert_exception);
}

SCORUM_TEST_CASE(etalon_calculation_check)
{
    auto result = calculate_auction_bets(default_per_block_values, default_position_weights);
    BOOST_REQUIRE_EQUAL(result.size(), 4);
    BOOST_CHECK_EQUAL(result[0], ASSET_SCR(3.85e+9));
    BOOST_CHECK_EQUAL(result[1], ASSET_SCR(2.8e+9));
    BOOST_CHECK_EQUAL(result[2], ASSET_SCR(2.3e+9));
    BOOST_CHECK_EQUAL(result[3], ASSET_SCR(2e+9));
}

SCORUM_TEST_CASE(check_top_less_that_coefficients)
{
    per_block_values_type per_block_values = { ASSET_SCR(10e+9), ASSET_SCR(7e+9), ASSET_SCR(5e+9) };

    auto result = calculate_auction_bets(per_block_values, default_position_weights);
    BOOST_REQUIRE_EQUAL(result.size(), 3);
    BOOST_CHECK_EQUAL(result[0], ASSET_SCR(6.55e+9));
    BOOST_CHECK_EQUAL(result[1], ASSET_SCR(5.5e+9));
    BOOST_CHECK_EQUAL(result[2], ASSET_SCR(5e+9));
}

SCORUM_TEST_CASE(check_no_budgets)
{
    per_block_values_type per_block_empty;

    auto result = calculate_auction_bets(per_block_empty, default_position_weights);
    BOOST_REQUIRE_EQUAL(result.size(), 0);
}

SCORUM_TEST_CASE(extrapolations_check)
{
    position_weights_type next_position_weights = default_position_weights;
    per_block_values_type next_per_block_values = default_per_block_values;

    next_position_weights.push_back(next_decreasing_percent(*next_position_weights.rbegin()));
    next_per_block_values.push_back(next_decreasing_per_block(*next_per_block_values.rbegin()));

    BOOST_CHECK_LT((*calculate_auction_bets(next_per_block_values, next_position_weights).rbegin()),
                   (*calculate_auction_bets(default_per_block_values, default_position_weights).rbegin()));
}

BOOST_AUTO_TEST_SUITE_END()
}
