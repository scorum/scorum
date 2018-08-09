#include <boost/test/unit_test.hpp>

#include <scorum/chain/betting/betting_service.hpp>

#include "betting_common.hpp"

namespace betting_service_tests {

using namespace scorum::chain;
using namespace scorum::chain::betting;
using namespace scorum::protocol;
using namespace scorum::protocol::betting;

struct betting_service_fixture : public betting_common::betting_service_fixture_impl
{
    betting_service_fixture()
        : service(*dbs_services)
    {
    }

    betting_service service;
};

BOOST_FIXTURE_TEST_SUITE(betting_service_tests, betting_service_fixture)

SCORUM_TEST_CASE(is_betting_moderator_check)
{
    BOOST_REQUIRE_NE(moderator, "jack");

    BOOST_CHECK(!service.is_betting_moderator("jack"));
    BOOST_CHECK(service.is_betting_moderator(moderator));
}

SCORUM_TEST_CASE(create_bet_positive_check)
{
    const auto& new_bet = service.create_bet(test_bet_better, test_bet_game, test_bet_wincase,
                                             odds::from_string(test_bet_k), test_bet_stake);

    BOOST_REQUIRE(bets.is_exists(new_bet.id));

    BOOST_CHECK_EQUAL(new_bet.better, test_bet_better);
    BOOST_CHECK_EQUAL(new_bet.game._id, test_bet_game._id);
    BOOST_CHECK_EQUAL(new_bet.odds_value.to_string(), test_bet_k);
    BOOST_CHECK_EQUAL(new_bet.stake, test_bet_stake);
    BOOST_CHECK_EQUAL(new_bet.rest_stake, test_bet_stake);
}

SCORUM_TEST_CASE(create_bet_negative_check)
{
    BOOST_CHECK_THROW(
        service.create_bet("alice", test_bet_game, goal_home_yes(), odds::from_string("10/1"), ASSET_SP(1e+9)),
        fc::exception);
    BOOST_CHECK_THROW(
        service.create_bet("alice", test_bet_game, goal_home_yes(), odds::from_string("10/1"), ASSET_NULL_SCR),
        fc::exception);
    BOOST_CHECK_THROW(
        service.create_bet("alice", test_bet_game, goal_home_yes(), odds::from_string("10/1"), ASSET_NULL_SP),
        fc::exception);
}

BOOST_AUTO_TEST_SUITE_END()
}
