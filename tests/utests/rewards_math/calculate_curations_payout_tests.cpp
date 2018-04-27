#include <boost/test/unit_test.hpp>

#include "defines.hpp"

#include <scorum/protocol/asset.hpp>
#include <scorum/rewards_math/formulas.hpp>

using namespace scorum::rewards_math;
using namespace scorum::protocol;

using scorum::protocol::curve_id;
using fc::uint128_t;

namespace database_fixture {
struct rewards_math_calculate_curations_payout_fixture
{
    const share_type payout = ASSET_SCR(100e+3).amount;
    const uint64_t weight = SCORUM_PERCENT(10);
    const uint64_t total_weight = weight * 2;
};
}

using namespace database_fixture;

BOOST_FIXTURE_TEST_SUITE(rewards_math_calculate_curations_payout_tests, rewards_math_calculate_curations_payout_fixture)

BOOST_AUTO_TEST_CASE(calculate_curations_payout_check)
{
    BOOST_CHECK_GT(calculate_curations_payout(payout, SCORUM_CURATION_REWARD_PERCENT), share_type());
    BOOST_CHECK_LT(calculate_curations_payout(payout, SCORUM_CURATION_REWARD_PERCENT), payout);
}

BOOST_AUTO_TEST_CASE(calculate_curation_payout_check)
{
    BOOST_CHECK_GT(calculate_curation_payout(payout, total_weight, weight), share_type(0));
    BOOST_CHECK_LT(calculate_curation_payout(payout, total_weight, weight), payout);
}

BOOST_AUTO_TEST_SUITE_END()
