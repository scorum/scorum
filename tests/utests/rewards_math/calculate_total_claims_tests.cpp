#include <boost/test/unit_test.hpp>

#include "defines.hpp"

#include <scorum/protocol/asset.hpp>
#include <scorum/rewards_math/formulas.hpp>

using namespace scorum::rewards_math;
using namespace scorum::protocol;

using scorum::protocol::curve_id;
using fc::uint128_t;

namespace database_fixture {
struct rewards_math_calculate_total_claims_fixture
{
    const share_type any_typical_rshares = SCORUM_MIN_COMMENT_PAYOUT_SHARE * 100;
    const share_type any_reward_fund = SCORUM_MIN_COMMENT_PAYOUT_SHARE * 1000;
    const uint128_t recent_claims = 0;
    const fc::time_point_sec last_payout_check = fc::time_point_sec(TEST_GENESIS_TIMESTAMP);
    const fc::time_point_sec now = last_payout_check + SCORUM_BLOCK_INTERVAL;
};
}

using namespace database_fixture;

BOOST_FIXTURE_TEST_SUITE(rewards_math_calculate_total_claims_tests, rewards_math_calculate_total_claims_fixture)

BOOST_AUTO_TEST_CASE(calculate_total_claims_invalid_params)
{
    SCORUM_REQUIRE_THROW(calculate_total_claims(recent_claims, any_reward_fund, now, last_payout_check,
                                                curve_id::linear, { any_typical_rshares }, fc::microseconds()),
                         fc::exception);

    SCORUM_REQUIRE_THROW(calculate_total_claims(recent_claims, any_reward_fund, last_payout_check, now,
                                                curve_id::linear, { any_typical_rshares },
                                                SCORUM_RECENT_RSHARES_DECAY_RATE),
                         fc::exception);
}

BOOST_AUTO_TEST_CASE(calculate_total_claims_positive)
{
    BOOST_CHECK_NO_THROW(calculate_total_claims(recent_claims, any_reward_fund, now, last_payout_check,
                                                curve_id::linear, { any_typical_rshares },
                                                SCORUM_RECENT_RSHARES_DECAY_RATE));
}

BOOST_AUTO_TEST_SUITE_END()
