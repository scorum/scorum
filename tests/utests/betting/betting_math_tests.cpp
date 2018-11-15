#include <boost/test/unit_test.hpp>

#include <scorum/protocol/asset.hpp>
#include <scorum/protocol/odds.hpp>

#include <scorum/chain/betting/betting_math.hpp>

#include "defines.hpp"

namespace betting_math_tests {

using namespace scorum::protocol;
using namespace scorum::chain;

BOOST_AUTO_TEST_SUITE(betting_math_tests)

SCORUM_TEST_CASE(extra_large_coefficient_big_gain_mismatch_test)
{
    const asset bet1_stake = ASSET_SCR(10000);
    const asset bet2_stake = ASSET_SCR(90'000'000);

    const odds bet1_odds(10000, 1);
    const odds bet2_odds(10000, 9999);

    const auto matched = calculate_matched_stake(bet1_stake, bet2_stake, bet1_odds, bet2_odds);

    BOOST_CHECK_EQUAL(matched.bet1_matched.amount, 9000);
    BOOST_CHECK_EQUAL(matched.bet2_matched.amount, bet2_stake.amount);
}

SCORUM_TEST_CASE(more_than_matched_check) // to catch and log exceptions in calculate_matched_stake
{
    const asset stake_my = ASSET_SCR(1e+9);
    const asset stake_other = ASSET_SCR(2e+9);

    const odds odds_my = odds(10, 1);
    const odds odds_other = odds(10, 9);

    BOOST_CHECK_EQUAL(stake_my * odds_my, ASSET_SCR(10'000'000'000));
    BOOST_CHECK_EQUAL(stake_other * odds_other, ASSET_SCR(2'222'222'222));

    BOOST_CHECK_EQUAL(odds_my.simplified(), odds_other.inverted());
    BOOST_CHECK_EQUAL(odds_other.simplified(), odds_my.inverted());

    auto matched = calculate_matched_stake(stake_my, stake_other, odds_my, odds_other);

    BOOST_REQUIRE_EQUAL(matched.bet1_matched.amount.value, 222'222'222);
}

SCORUM_TEST_CASE(less_than_matched_check)
{
    const asset stake_my = ASSET_SCR(0.1e+9);
    const asset stake_other = ASSET_SCR(2e+9);

    const odds odds_my = odds(10, 1);
    const odds odds_other = odds(10, 9);

    BOOST_CHECK_EQUAL(stake_my * odds_my, ASSET_SCR(1'000'000'000));
    BOOST_CHECK_EQUAL(stake_other * odds_other, ASSET_SCR(2'222'222'222));

    auto matched = calculate_matched_stake(stake_my, stake_other, odds_my, odds_other);

    BOOST_REQUIRE_EQUAL(matched.bet2_matched.amount.value, 900'000'000);
}

SCORUM_TEST_CASE(equal_matched_check)
{
    const asset stake_my = ASSET_SCR(1e+9);
    const asset stake_other = ASSET_SCR(9e+9);

    const odds odds_my = odds(10, 1);
    const odds odds_other = odds(10, 9);

    BOOST_CHECK_EQUAL(stake_my * odds_my, stake_other * odds_other);

    auto matched = calculate_matched_stake(stake_my, stake_other, odds_my, odds_other);

    BOOST_REQUIRE_EQUAL(matched.bet1_matched, stake_my);

    matched = calculate_matched_stake(stake_other, stake_my, odds_other, odds_my);

    BOOST_REQUIRE_EQUAL(matched.bet1_matched, stake_other);
}

SCORUM_TEST_CASE(calculate_matched_stake_negative_check)
{
    BOOST_CHECK_THROW(calculate_matched_stake(ASSET_SCR(1e+9), ASSET_SP(9e+9), odds(10, 1), odds(10, 9)),
                      fc::assert_exception);
    BOOST_CHECK_THROW(calculate_matched_stake(ASSET_SP(1e+9), ASSET_SCR(9e+9), odds(10, 1), odds(10, 9)),
                      fc::assert_exception);
    BOOST_CHECK_THROW(calculate_matched_stake(ASSET_SP(1e+9), ASSET_SP(9e+9), odds(10, 1), odds(10, 9)),
                      fc::assert_exception);

    BOOST_CHECK_THROW(calculate_matched_stake(ASSET_SCR(1e+9), ASSET_SCR(9e+9), odds(10, 1), odds(10, 5)),
                      fc::assert_exception);
    BOOST_CHECK_THROW(calculate_matched_stake(ASSET_SCR(1e+9), ASSET_SCR(9e+9), odds(10, 5), odds(10, 9)),
                      fc::assert_exception);
}

SCORUM_TEST_CASE(calculate_gain_positive_check)
{
    auto potential_profit = calculate_gain(ASSET_SCR(1e+9), odds(10, 1));

    BOOST_CHECK_EQUAL(potential_profit, ASSET_SCR(9e+9));
}

SCORUM_TEST_CASE(min_bet_min_odds)
{
    const asset bet_stake = SCORUM_MIN_BET_STAKE;
    const odds bet_odds = SCORUM_MIN_ODDS;

    auto r1 = bet_stake * bet_odds;

    BOOST_CHECK_EQUAL(1'001'000u, r1.amount);
}

SCORUM_TEST_CASE(min_bet_max_odds)
{
    const asset bet_stake = SCORUM_MIN_BET_STAKE;
    const odds bet_odds = SCORUM_MIN_ODDS.inverted();

    auto r1 = bet_stake * bet_odds;

    BOOST_CHECK_EQUAL(1'001'000'000u, r1.amount);
}

SCORUM_TEST_CASE(calculate_potential_result)
{
    const asset bet_stake = asset::from_string("1000000.000000000 SCR");
    const odds bet_odds = SCORUM_MIN_ODDS.inverted();

    auto result = bet_stake * bet_odds;

    BOOST_CHECK_EQUAL("1001000000.000000000 SCR", result.to_string());
}

SCORUM_TEST_CASE(calc_max_bet_stake)
{
    asset bet_stake(share_type::max(), SCORUM_SYMBOL);

    BOOST_CHECK_EQUAL("9223372036.854775807 SCR", bet_stake.to_string());

    const odds bet_odds = SCORUM_MIN_ODDS.inverted();

    auto result = bet_stake * bet_odds.base().coup();

    BOOST_CHECK_EQUAL(SCORUM_MAX_BET_STAKE, result);
}

SCORUM_TEST_CASE(calc_potential_result_for_max_stake_max_odds)
{
    asset bet_stake = SCORUM_MAX_BET_STAKE;
    const odds bet_odds = SCORUM_MIN_ODDS.inverted();

    auto result = bet_stake * bet_odds;

    BOOST_CHECK_EQUAL("9223372036.854775800 SCR", result.to_string());
}

SCORUM_TEST_CASE(calculate_matched_stake_for_max_stake_max_odds)
{
    asset bet1_stake = SCORUM_MAX_BET_STAKE;
    const odds bet1_odds = SCORUM_MIN_ODDS.inverted();

    asset bet2_stake = bet1_stake * bet1_odds - bet1_stake;
    odds bet2_odds = SCORUM_MIN_ODDS;

    std::cout << bet2_stake.to_string() << std::endl;

    auto matched = calculate_matched_stake(bet1_stake, bet2_stake, bet1_odds, bet2_odds);

    BOOST_CHECK_EQUAL(bet1_stake, matched.bet1_matched);
    BOOST_CHECK_EQUAL(bet2_stake, matched.bet2_matched);
}

SCORUM_TEST_CASE(calculate_matched_stake_throw_exception_when_potential_result_is_to_big)
{
    asset bet1_stake = SCORUM_MAX_BET_STAKE + 10;
    const odds bet1_odds = SCORUM_MIN_ODDS.inverted();

    asset bet2_stake = asset::from_string("9214157878.975800000 SCR");
    odds bet2_odds = SCORUM_MIN_ODDS;

    SCORUM_CHECK_EXCEPTION(calculate_matched_stake(bet1_stake, bet2_stake, bet1_odds, bet2_odds),
                           fc::underflow_exception, "");
}

BOOST_AUTO_TEST_SUITE_END()
}
