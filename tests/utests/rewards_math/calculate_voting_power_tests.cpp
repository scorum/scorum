#include <boost/test/unit_test.hpp>

#include "defines.hpp"

#include <scorum/protocol/asset.hpp>
#include <scorum/rewards_math/formulas.hpp>

using namespace scorum::rewards_math;
using namespace scorum::protocol;

using scorum::protocol::curve_id;

namespace database_fixture {
struct rewards_math_voting_power_fixture
{
    const fc::time_point_sec last_voted = fc::time_point_sec(TEST_GENESIS_TIMESTAMP);
    const fc::time_point_sec now = last_voted + SCORUM_VOTE_REGENERATION_SECONDS.to_seconds() / 2;
    const vote_weight_type vote_weight = SCORUM_PERCENT(50);
    percent_type voting_power = SCORUM_100_PERCENT;
};
}

using namespace database_fixture;

BOOST_FIXTURE_TEST_SUITE(rewards_math_voting_power_tests, rewards_math_voting_power_fixture)

BOOST_AUTO_TEST_CASE(calculate_restoring_power_invalid_params)
{
    SCORUM_REQUIRE_THROW(calculate_restoring_power(voting_power, now, last_voted, fc::microseconds(0)), fc::exception);

    SCORUM_REQUIRE_THROW(calculate_restoring_power(voting_power, last_voted, now, SCORUM_VOTE_REGENERATION_SECONDS),
                         fc::exception);
}

BOOST_AUTO_TEST_CASE(calculate_restoring_power_check_voting_power_is_max)
{
    BOOST_CHECK_EQUAL(calculate_restoring_power(voting_power, now, last_voted, SCORUM_VOTE_REGENERATION_SECONDS),
                      voting_power);
}

BOOST_AUTO_TEST_CASE(calculate_used_power_positive)
{
    BOOST_CHECK_GT(calculate_used_power(voting_power, vote_weight, SCORUM_VOTING_POWER_DECAY_PERCENT), 0u);

    BOOST_CHECK_LE(calculate_used_power(voting_power, vote_weight, SCORUM_VOTING_POWER_DECAY_PERCENT), voting_power);
}

BOOST_AUTO_TEST_CASE(decreasing_and_restoring_voting_power_check)
{
    const int max_steps = 1000;
    int ci = 0;
    // decrease power with limiting by result percent or iterations (if decreasing too slow)
    while (voting_power > SCORUM_PERCENT(75) && ci++ < max_steps)
    {
        uint16_t used_power = calculate_used_power(voting_power, vote_weight, SCORUM_VOTING_POWER_DECAY_PERCENT);
        voting_power -= used_power;
    }

    // check if power decays too slow
    BOOST_CHECK_LT(ci, max_steps);

    const fc::time_point_sec last_voted(TEST_GENESIS_TIMESTAMP);
    const fc::time_point_sec now(last_voted + SCORUM_VOTE_REGENERATION_SECONDS.to_seconds() / 2);

    BOOST_CHECK_EQUAL(calculate_restoring_power(voting_power, now, last_voted, SCORUM_VOTE_REGENERATION_SECONDS),
                      SCORUM_100_PERCENT);
}

BOOST_AUTO_TEST_SUITE_END()
