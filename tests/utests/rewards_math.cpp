#include <boost/test/unit_test.hpp>

#include "defines.hpp"

#include <scorum/protocol/asset.hpp>
#include <scorum/rewards_math/formulas.hpp>

using namespace scorum::rewards_math;
using namespace scorum::protocol;

using scorum::protocol::curve_id;
using fc::uint128_t;

BOOST_AUTO_TEST_SUITE(rewards_math_tests)

BOOST_AUTO_TEST_CASE(predict_payout_check)
{
    const share_type fund_balance = ASSET_SCR(100e+3).amount;
    const share_type any_typical_rshares = 2222;

    uint128_t recent_claims = 0;
    share_type max_payout = fund_balance / 2;

    BOOST_CHECK_EQUAL(predict_payout(recent_claims, fund_balance, any_typical_rshares, curve_id::linear, max_payout,
                                     SCORUM_RECENT_RSHARES_DECAY_RATE, SCORUM_MIN_COMMENT_PAYOUT_SHARE),
                      max_payout);

    max_payout = fund_balance * 2;

    BOOST_CHECK_EQUAL(predict_payout(recent_claims, fund_balance, any_typical_rshares, curve_id::linear, max_payout,
                                     SCORUM_RECENT_RSHARES_DECAY_RATE, SCORUM_MIN_COMMENT_PAYOUT_SHARE),
                      fund_balance);

    recent_claims = any_typical_rshares.value;

    share_type payout = predict_payout(recent_claims, fund_balance, any_typical_rshares, curve_id::linear, max_payout,
                                       SCORUM_RECENT_RSHARES_DECAY_RATE, SCORUM_MIN_COMMENT_PAYOUT_SHARE);

    BOOST_CHECK_LT(payout, fund_balance);
}

BOOST_AUTO_TEST_CASE(calculate_total_claims_check)
{
    const share_type any_typical_rshares = SCORUM_MIN_COMMENT_PAYOUT_SHARE * 100;
    const uint128_t recent_claims = 0;
    const fc::time_point_sec last_payout_check(TEST_GENESIS_TIMESTAMP);
    const fc::time_point_sec now(last_payout_check + SCORUM_BLOCK_INTERVAL);

    BOOST_CHECK_NO_THROW(calculate_total_claims(recent_claims, now, last_payout_check, curve_id::linear,
                                                { any_typical_rshares }, SCORUM_RECENT_RSHARES_DECAY_RATE));

    SCORUM_REQUIRE_THROW(calculate_total_claims(recent_claims, now, last_payout_check, curve_id::linear,
                                                { any_typical_rshares }, fc::microseconds()),
                         fc::exception);

    SCORUM_REQUIRE_THROW(calculate_total_claims(recent_claims, last_payout_check, now, curve_id::linear,
                                                { any_typical_rshares }, SCORUM_RECENT_RSHARES_DECAY_RATE),
                         fc::exception);
}

BOOST_AUTO_TEST_CASE(calculate_payout_check)
{
    const share_type any_typical_rshares = SCORUM_MIN_COMMENT_PAYOUT_SHARE * 100;
    const share_type fund_balance = ASSET_SCR(100e+3).amount;
    const uint128_t total_claims = any_typical_rshares.value;
    const share_type max_payout = fund_balance / 2;

    BOOST_CHECK_GT(calculate_payout(any_typical_rshares, total_claims, fund_balance, curve_id::linear, max_payout,
                                    SCORUM_MIN_COMMENT_PAYOUT_SHARE),
                   share_type(0));

    BOOST_CHECK_EQUAL(calculate_payout(any_typical_rshares, total_claims, fund_balance, curve_id::linear, max_payout,
                                       fund_balance * 2),
                      share_type(0));

    SCORUM_REQUIRE_THROW(calculate_payout(share_type(0), total_claims, fund_balance, curve_id::linear, max_payout,
                                          SCORUM_MIN_COMMENT_PAYOUT_SHARE),
                         fc::exception);

    SCORUM_REQUIRE_THROW(calculate_payout(any_typical_rshares, uint128_t(0), fund_balance, curve_id::linear, max_payout,
                                          SCORUM_MIN_COMMENT_PAYOUT_SHARE),
                         fc::exception);
}

BOOST_AUTO_TEST_CASE(calculate_curations_payout_check)
{
    const share_type payout = ASSET_SCR(100e+3).amount;

    BOOST_CHECK_GT(calculate_curations_payout(payout, SCORUM_CURATION_REWARD_PERCENT), share_type());
    BOOST_CHECK_LT(calculate_curations_payout(payout, SCORUM_CURATION_REWARD_PERCENT), payout);
}

BOOST_AUTO_TEST_CASE(calculate_curation_payout_check)
{
    const share_type payout = ASSET_SCR(100e+3).amount;
    const uint64_t weight = SCORUM_PERCENT(10);
    const uint64_t total_weight = weight * 2;

    BOOST_CHECK_GT(calculate_curation_payout(payout, total_weight, weight), share_type(0));
    BOOST_CHECK_LT(calculate_curation_payout(payout, total_weight, weight), payout);
}

BOOST_AUTO_TEST_CASE(calculate_max_vote_weight_check)
{
    const share_type any_positive_rshares = 10000;
    const share_type recent_positive_rshares = any_positive_rshares / 2;

    BOOST_CHECK_GT(calculate_max_vote_weight(any_positive_rshares, recent_positive_rshares, curve_id::square_root), 0u);

    SCORUM_REQUIRE_THROW(
        calculate_max_vote_weight(recent_positive_rshares, any_positive_rshares, curve_id::square_root), fc::exception);
}

BOOST_AUTO_TEST_CASE(calculate_vote_weight_check)
{
    const uint64_t any_max_vote_weight = 10000u;
    const fc::time_point_sec when_comment_created(TEST_GENESIS_TIMESTAMP);
    const fc::time_point_sec now(when_comment_created + SCORUM_BLOCK_INTERVAL * 10);

    BOOST_CHECK_GT(
        calculate_vote_weight(any_max_vote_weight, now, when_comment_created, SCORUM_REVERSE_AUCTION_WINDOW_SECONDS),
        0u);

    SCORUM_REQUIRE_THROW(
        calculate_vote_weight(any_max_vote_weight, when_comment_created, now, SCORUM_REVERSE_AUCTION_WINDOW_SECONDS),
        fc::exception);

    SCORUM_REQUIRE_THROW(calculate_vote_weight(any_max_vote_weight, now, when_comment_created, fc::microseconds(0)),
                         fc::exception);
}

BOOST_AUTO_TEST_CASE(calculate_restoring_power_check)
{
    const fc::time_point_sec last_voted(TEST_GENESIS_TIMESTAMP);
    const uint16_t voting_power = SCORUM_100_PERCENT;
    const fc::time_point_sec now(last_voted + SCORUM_VOTE_REGENERATION_SECONDS.to_seconds() / 2);

    BOOST_CHECK_EQUAL(calculate_restoring_power(voting_power, now, last_voted, SCORUM_VOTE_REGENERATION_SECONDS),
                      voting_power);

    SCORUM_REQUIRE_THROW(calculate_restoring_power(voting_power, now, last_voted, fc::microseconds(0)), fc::exception);

    SCORUM_REQUIRE_THROW(calculate_restoring_power(voting_power, last_voted, now, SCORUM_VOTE_REGENERATION_SECONDS),
                         fc::exception);
}

BOOST_AUTO_TEST_CASE(calculate_used_power_check)
{
    const percent_type voting_power = SCORUM_100_PERCENT;
    const vote_weight_type vote_weight = SCORUM_PERCENT(50);

    BOOST_CHECK_GT(calculate_used_power(voting_power, vote_weight, SCORUM_MAX_VOTES_PER_DAY_VOTING_POWER_RATE,
                                        SCORUM_VOTE_REGENERATION_SECONDS),
                   0u);

    BOOST_CHECK_LE(calculate_used_power(voting_power, vote_weight, SCORUM_MAX_VOTES_PER_DAY_VOTING_POWER_RATE,
                                        SCORUM_VOTE_REGENERATION_SECONDS),
                   voting_power);
}

BOOST_AUTO_TEST_CASE(calculate_abs_reward_shares_check)
{
    const uint16_t used_voting_power = SCORUM_PERCENT(50);
    const share_type effective_balance_shares = ASSET_SP(100e+3).amount;

    BOOST_CHECK_LT(calculate_abs_reward_shares(used_voting_power, effective_balance_shares), effective_balance_shares);

    BOOST_CHECK_EQUAL(calculate_abs_reward_shares(used_voting_power, share_type(0)), share_type(0));
}

BOOST_AUTO_TEST_CASE(restoring_voting_power_check)
{
    const vote_weight_type vote_weight = SCORUM_PERCENT(50);

    percent_type voting_power = SCORUM_100_PERCENT;

    int ci = 0;
    // decrease power with limiting by result percent or iterations (if decreasing too slow)
    while (voting_power > SCORUM_PERCENT(75) && ci++ < SCORUM_MAX_VOTES_PER_DAY_VOTING_POWER_RATE * 10)
    {
        uint16_t used_power = calculate_used_power(
            voting_power, vote_weight, SCORUM_MAX_VOTES_PER_DAY_VOTING_POWER_RATE, SCORUM_VOTE_REGENERATION_SECONDS);
        voting_power -= used_power;
    }

    const fc::time_point_sec last_voted(TEST_GENESIS_TIMESTAMP);
    const fc::time_point_sec now(last_voted + SCORUM_VOTE_REGENERATION_SECONDS.to_seconds() / 2);

    BOOST_CHECK_EQUAL(calculate_restoring_power(voting_power, now, last_voted, SCORUM_VOTE_REGENERATION_SECONDS),
                      SCORUM_100_PERCENT);
}

BOOST_AUTO_TEST_SUITE_END()
