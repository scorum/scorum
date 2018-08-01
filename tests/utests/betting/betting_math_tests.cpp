#include <boost/test/unit_test.hpp>

#include <scorum/protocol/asset.hpp>
#include <scorum/protocol/odds.hpp>

#include <scorum/chain/services/betting_service.hpp>

#include "defines.hpp"

namespace betting_math_tests {

using namespace scorum::protocol;
using namespace scorum::chain;

BOOST_AUTO_TEST_SUITE(betting_math_tests)

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

    BOOST_REQUIRE_EQUAL(matched.amount.value, 777'777'778);
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

    BOOST_REQUIRE_EQUAL(matched.amount.value, -1'100'000'000);
}

SCORUM_TEST_CASE(equal_matched_check)
{
    const asset stake_my = ASSET_SCR(1e+9);
    const asset stake_other = ASSET_SCR(9e+9);

    const odds odds_my = odds(10, 1);
    const odds odds_other = odds(10, 9);

    BOOST_CHECK_EQUAL(stake_my * odds_my, stake_other * odds_other);

    auto matched = calculate_matched_stake(stake_my, stake_other, odds_my, odds_other);

    BOOST_REQUIRE_EQUAL(matched.amount.value, 0);
}

BOOST_AUTO_TEST_SUITE_END()
}
