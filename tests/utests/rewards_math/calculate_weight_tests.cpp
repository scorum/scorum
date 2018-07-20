#include <boost/test/unit_test.hpp>

#include "defines.hpp"

#include <scorum/protocol/asset.hpp>
#include <scorum/rewards_math/formulas.hpp>

using namespace scorum::rewards_math;
using namespace scorum::protocol;

using scorum::protocol::curve_id;

namespace database_fixture {
struct rewards_math_calculate_weight_fixture
{
    const share_type any_positive_rshares = 10000;
    const share_type recent_positive_rshares = any_positive_rshares / 2;
    const uint64_t any_max_vote_weight = 10000u;
    const fc::time_point_sec when_comment_created = fc::time_point_sec(TEST_GENESIS_TIMESTAMP);
    const fc::time_point_sec now = when_comment_created + SCORUM_BLOCK_INTERVAL * 10;
};
}

using namespace database_fixture;

BOOST_FIXTURE_TEST_SUITE(rewards_math_calculate_weight_tests, rewards_math_calculate_weight_fixture)

BOOST_AUTO_TEST_CASE(calculate_max_vote_weight_invalid_params)
{
    SCORUM_REQUIRE_THROW(
        calculate_max_vote_weight(recent_positive_rshares, any_positive_rshares, curve_id::square_root), fc::exception);
}

BOOST_AUTO_TEST_CASE(calculate_max_vote_weight_positive)
{
    BOOST_CHECK_GT(calculate_max_vote_weight(any_positive_rshares, recent_positive_rshares, curve_id::square_root), 0u);
}

BOOST_AUTO_TEST_CASE(calculate_vote_weight_invalid_params)
{
    SCORUM_REQUIRE_THROW(calculate_vote_weight(any_max_vote_weight, now, when_comment_created, fc::microseconds(0)),
                         fc::exception);

    SCORUM_REQUIRE_THROW(
        calculate_vote_weight(any_max_vote_weight, when_comment_created, now, SCORUM_REVERSE_AUCTION_WINDOW_SECONDS),
        fc::exception);
}

BOOST_AUTO_TEST_CASE(calculate_vote_weight_positive)
{
    BOOST_CHECK_GT(
        calculate_vote_weight(any_max_vote_weight, now, when_comment_created, SCORUM_REVERSE_AUCTION_WINDOW_SECONDS),
        0u);
}

BOOST_AUTO_TEST_SUITE_END()
