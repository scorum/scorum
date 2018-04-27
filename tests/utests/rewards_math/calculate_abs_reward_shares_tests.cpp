#include <boost/test/unit_test.hpp>

#include "defines.hpp"

#include <scorum/protocol/asset.hpp>
#include <scorum/rewards_math/formulas.hpp>

using namespace scorum::rewards_math;
using namespace scorum::protocol;

using scorum::protocol::curve_id;
using fc::uint128_t;

namespace database_fixture {
struct rewards_math_abs_reward_shares_fixture
{
    const uint16_t used_voting_power = SCORUM_PERCENT(50);
    const share_type effective_balance_shares = ASSET_SP(100e+3).amount;
};
}

using namespace database_fixture;

BOOST_FIXTURE_TEST_SUITE(rewards_math_abs_reward_shares_tests, rewards_math_abs_reward_shares_fixture)

BOOST_AUTO_TEST_CASE(calculate_abs_reward_shares_zerro_balance)
{
    BOOST_CHECK_EQUAL(calculate_abs_reward_shares(used_voting_power, share_type(0)), share_type(0));
}

BOOST_AUTO_TEST_CASE(calculate_abs_reward_shares_positive)
{
    BOOST_CHECK_LT(calculate_abs_reward_shares(used_voting_power, effective_balance_shares), effective_balance_shares);
}

BOOST_AUTO_TEST_SUITE_END()
