#include <boost/test/unit_test.hpp>

#include "service_wrappers.hpp"

#include <scorum/chain/services/betting_property.hpp>
#include <scorum/chain/betting/betting_service.hpp>
#include <scorum/chain/dba/db_accessor_mock.hpp>
#include <scorum/chain/dba/db_accessor_i.hpp>
#include <scorum/chain/dba/db_accessor_factory.hpp>
#include "betting_common.hpp"

namespace {

using namespace scorum::chain;
using namespace service_wrappers;
using namespace scorum::chain::dba;
using namespace scorum::chain::betting;
using namespace scorum::protocol;
using namespace scorum::protocol::betting;

struct betting_service_fixture : public betting_common::betting_service_fixture_impl
{
    MockRepository mocks;

    db_accessor_factory* dba_factory = mocks.Mock<db_accessor_factory>();

    const account_name_type moderator = "smit";

    betting_service_fixture()
        : betting_prop_dba_i(db_accessor_mock<betting_property_object>{})
        , betting_prop_dba(betting_prop_dba_i.get_accessor_inst<db_accessor_mock<betting_property_object>>())
        , betting_prop(
              create_object<betting_property_object>(shm, [&](betting_property_object& o) { o.moderator = moderator; }))
        , bet_dba_i(db_accessor_mock<bet_object>{})
        , bet_dba(bet_dba_i.get_accessor_inst<db_accessor_mock<bet_object>>())
        , bet(create_object<bet_object>(shm, [&](bet_object& o) {
            o.better = test_bet_better;
            o.game = test_bet_game;
            o.stake = test_bet_stake;
            o.rest_stake = test_bet_stake;
            o.odds_value = odds::from_string(test_bet_k);
        }))
    {
        mocks.OnCallFunc(get_db_accessor<betting_property_object>).ReturnByRef(betting_prop_dba_i);
        mocks.OnCallFunc(get_db_accessor<bet_object>).ReturnByRef(bet_dba_i);

        betting_prop_dba.mock(&db_accessor_mock<betting_property_object>::get,
                              [&]() -> decltype(auto) { return (betting_prop); });
        bet_dba.mock(&db_accessor_mock<bet_object>::create,
                     [&](const db_accessor_mock<bet_object>::modifier_type&) -> decltype(auto) { return (bet); });
    }

protected:
    db_accessor_i<betting_property_object> betting_prop_dba_i;
    db_accessor_mock<betting_property_object>& betting_prop_dba;
    betting_property_object betting_prop;

    db_accessor_i<bet_object> bet_dba_i;
    db_accessor_mock<bet_object>& bet_dba;
    bet_object bet;
};

BOOST_FIXTURE_TEST_SUITE(betting_service_tests, betting_service_fixture)

SCORUM_TEST_CASE(budget_service_is_betting_moderator_check)
{
    betting_service service(*dbs_services, *dba_factory);

    BOOST_CHECK(!service.is_betting_moderator("jack"));
    BOOST_CHECK(service.is_betting_moderator(moderator));
}

SCORUM_TEST_CASE(create_bet_positive_check)
{
    betting_service service(*dbs_services, *dba_factory);
    const auto& new_bet = service.create_bet(test_bet_better, test_bet_game, test_bet_wincase,
                                             odds::from_string(test_bet_k), test_bet_stake);

    BOOST_CHECK_EQUAL(new_bet.better, test_bet_better);
    BOOST_CHECK_EQUAL(new_bet.game._id, test_bet_game._id);
    BOOST_CHECK_EQUAL(new_bet.odds_value.to_string(), test_bet_k);
    BOOST_CHECK_EQUAL(new_bet.stake, test_bet_stake);
    BOOST_CHECK_EQUAL(new_bet.rest_stake, test_bet_stake);
}

SCORUM_TEST_CASE(create_bet_negative_check)
{
    betting_service service(*dbs_services, *dba_factory);

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
