#include <boost/test/unit_test.hpp>

#include <scorum/chain/database/budget_management_algorithms.hpp>

#include <scorum/protocol/types.hpp>
#include <scorum/protocol/asset.hpp>

#include "defines.hpp"

#include <vector>

namespace vcg_calculation_tests {

using namespace scorum::chain;
using namespace scorum::protocol;

struct vcg_calculation_fixture
{
    vcg_calculation_fixture()
        : default_position_weights{ 100, 85, 75, 65 }
        , default_per_block_values{ ASSET_SCR(10e+9).amount, ASSET_SCR(7e+9).amount, ASSET_SCR(5e+9).amount,
                                    ASSET_SCR(3e+9).amount, ASSET_SCR(2e+9).amount }
    {
    }

    percent_type next_decreasing_percent(const percent_type current, const percent_type delta = 15)
    {
        BOOST_REQUIRE_GT(delta, 0);
        BOOST_REQUIRE_LT(delta, 100);
        return current - current * delta / 100;
    }

    share_type next_decreasing_per_block(const share_type current, const share_type delta = ASSET_SCR(0.5e+9).amount)
    {
        BOOST_REQUIRE_GT(delta, 0);
        BOOST_REQUIRE_LT(delta, current);
        return current - delta;
    }

    using position_weights_type = std::vector<percent_type>;
    using per_block_values_type = std::vector<share_type>;

    const position_weights_type default_position_weights;
    const per_block_values_type default_per_block_values;
};

BOOST_FIXTURE_TEST_SUITE(vcg_calculation_check, vcg_calculation_fixture)

SCORUM_TEST_CASE(invalid_input_check)
{
    BOOST_MESSAGE("Check invalid coefficient's list");

    SCORUM_CHECK_THROW(calculate_vcg_cash(0, position_weights_type(), default_per_block_values), fc::assert_exception);

    BOOST_MESSAGE("Check list of per-block values");

    SCORUM_CHECK_THROW(calculate_vcg_cash(0, default_position_weights, per_block_values_type()), fc::assert_exception);

    BOOST_MESSAGE("Check invalid position");

    SCORUM_CHECK_THROW(
        calculate_vcg_cash(default_position_weights.size() * 2, default_position_weights, default_per_block_values),
        fc::assert_exception);

    BOOST_MESSAGE("Check invalid (negative) coefficient value");

    position_weights_type invalid_position_weights = default_position_weights;
    (*invalid_position_weights.rbegin()) *= -1;
    SCORUM_CHECK_THROW(calculate_vcg_cash(0, invalid_position_weights, per_block_values_type()), fc::assert_exception);

    BOOST_MESSAGE("Check invalid (disrupted sorting) coefficient value");

    invalid_position_weights = default_position_weights;
    auto half_position = default_position_weights.size() / 2;
    std::swap(invalid_position_weights[0], invalid_position_weights[half_position]);
    SCORUM_CHECK_THROW(calculate_vcg_cash(0, invalid_position_weights, per_block_values_type()), fc::assert_exception);

    BOOST_MESSAGE("Check invalid (negative) per-block value");

    per_block_values_type invalid_per_block_values = default_per_block_values;
    (*invalid_per_block_values.rbegin()) *= -1;
    SCORUM_CHECK_THROW(calculate_vcg_cash(0, default_position_weights, invalid_per_block_values), fc::assert_exception);
}

SCORUM_TEST_CASE(etalon_calculation_check)
{
    BOOST_CHECK_EQUAL(calculate_vcg_cash(0, default_position_weights, default_per_block_values),
                      ASSET_SCR(3.15e+9).amount);
    BOOST_CHECK_EQUAL(calculate_vcg_cash(1, default_position_weights, default_per_block_values),
                      ASSET_SCR(2.470588235e+9).amount);
    BOOST_CHECK_EQUAL(calculate_vcg_cash(2, default_position_weights, default_per_block_values),
                      ASSET_SCR(2.133333333e+9).amount);
    BOOST_CHECK_EQUAL(calculate_vcg_cash(3, default_position_weights, default_per_block_values),
                      ASSET_SCR(2e+9).amount);
}

SCORUM_TEST_CASE(extrapolations_check)
{
    position_weights_type next_position_weights = default_position_weights;
    per_block_values_type next_per_block_values = default_per_block_values;

    auto old_last_pos = next_position_weights.size() - 1;

    next_position_weights.push_back(next_decreasing_percent(*next_position_weights.rbegin()));
    next_per_block_values.push_back(next_decreasing_per_block(*next_position_weights.rbegin()));

    auto new_last_pos = next_position_weights.size() - 1;

    BOOST_CHECK_GT(calculate_vcg_cash(old_last_pos, default_position_weights, default_per_block_values),
                   calculate_vcg_cash(new_last_pos, default_position_weights, default_per_block_values));
}

BOOST_AUTO_TEST_SUITE_END()
}
