#include <boost/test/unit_test.hpp>

#include "defines.hpp"

#include <scorum/protocol/asset.hpp>
#include <scorum/rewards_math/formulas.hpp>

using namespace scorum::rewards_math;
using namespace scorum::protocol;

using scorum::protocol::curve_id;
using fc::uint128_t;

namespace database_fixture {
struct rewards_math_calculate_payout_fixture
{
    const share_type any_typical_rshares = SCORUM_MIN_COMMENT_PAYOUT_SHARE * 100;
    const share_type fund_balance = SCORUM_MIN_COMMENT_PAYOUT_SHARE;
    const uint128_t total_claims = any_typical_rshares.value;
    share_type max_payout = fund_balance / 2;
};
}

using namespace database_fixture;

BOOST_FIXTURE_TEST_SUITE(rewards_math_calculate_payout_tests, rewards_math_calculate_payout_fixture)

BOOST_AUTO_TEST_CASE(predict_payout_check_max_payout_limit_payout)
{
    BOOST_CHECK_EQUAL(predict_payout(0, fund_balance, any_typical_rshares, curve_id::linear, max_payout,
                                     SCORUM_RECENT_RSHARES_DECAY_RATE, SCORUM_MIN_COMMENT_PAYOUT_SHARE),
                      0);

    BOOST_CHECK_GT(predict_payout(0, fund_balance * 2, any_typical_rshares, curve_id::linear, max_payout,
                                  SCORUM_RECENT_RSHARES_DECAY_RATE, SCORUM_MIN_COMMENT_PAYOUT_SHARE),
                   0);
}

BOOST_AUTO_TEST_CASE(predict_payout_check_recent_claims_limit_payout)
{
    share_type payout = predict_payout(any_typical_rshares.value, fund_balance, any_typical_rshares, curve_id::linear,
                                       max_payout, SCORUM_RECENT_RSHARES_DECAY_RATE, SCORUM_MIN_COMMENT_PAYOUT_SHARE);

    BOOST_CHECK_LT(payout, fund_balance);
}

BOOST_AUTO_TEST_CASE(calculate_payout_check_not_payout_for_threshold)
{
    BOOST_CHECK_EQUAL(calculate_payout(any_typical_rshares, total_claims, fund_balance, curve_id::linear, max_payout,
                                       fund_balance * 2),
                      share_type(0));
}

BOOST_AUTO_TEST_CASE(calculate_payout_check_invalid_params)
{
    SCORUM_REQUIRE_THROW(calculate_payout(share_type(0), total_claims, fund_balance, curve_id::linear, max_payout,
                                          SCORUM_MIN_COMMENT_PAYOUT_SHARE),
                         fc::exception);

    SCORUM_REQUIRE_THROW(calculate_payout(any_typical_rshares, uint128_t(0), fund_balance, curve_id::linear, max_payout,
                                          SCORUM_MIN_COMMENT_PAYOUT_SHARE),
                         fc::exception);
}

BOOST_AUTO_TEST_CASE(calculate_payout_check_positive)
{
    BOOST_CHECK_GT(calculate_payout(any_typical_rshares, total_claims, fund_balance, curve_id::linear, max_payout,
                                    SCORUM_MIN_COMMENT_PAYOUT_SHARE),
                   share_type(0));
}

BOOST_AUTO_TEST_SUITE_END()
