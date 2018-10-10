#include <boost/test/unit_test.hpp>

#include "defines.hpp"

#include "database_betting_integration.hpp"
#include "actor.hpp"

#include <scorum/protocol/proposal_operations.hpp>

#include <scorum/chain/services/dynamic_global_property.hpp>
#include <scorum/chain/services/proposal.hpp>
#include <scorum/chain/services/betting_property.hpp>
#include <scorum/chain/services/account.hpp>
#include <scorum/chain/services/game.hpp>

#include <scorum/protocol/betting/market_kind.hpp>

namespace bet_operations_tests {

using namespace scorum::protocol;
using namespace scorum::chain;

struct bet_operations_fixture : public database_fixture::database_betting_integration_fixture
{
    bet_operations_fixture()
        : dgp_service(db.dynamic_global_property_service())
        , betting_property_service(db.betting_property_service())
        , account_service(db.account_service())
        , game_service(db.game_service())
    {
        open_database();

        alice.scorum(ASSET_SCR(1e+9));
        actor(initdelegate).create_account(alice);
        actor(initdelegate).give_sp(alice, 1e+9);
        actor(initdelegate).give_scr(alice, alice.scr_amount.amount.value);

        bob.scorum(ASSET_SCR(1e+9));
        actor(initdelegate).create_account(bob);
        actor(initdelegate).give_sp(bob, 1e+9);
        actor(initdelegate).give_scr(bob, bob.scr_amount.amount.value);

        actor(initdelegate).create_account(moderator);
        actor(initdelegate).give_sp(moderator, 1e+9);
        actor(initdelegate).give_scr(moderator, 1e+9);

        empower_moderator(moderator);

        BOOST_REQUIRE(betting_property_service.is_exists());
        BOOST_REQUIRE_EQUAL(betting_property_service.get().moderator, moderator.name);

        create_game(moderator, { result_home{}, total{ 2000 } }, SCORUM_BLOCK_INTERVAL * 60 * 20);

        generate_block();
    }

    Actor alice = "alice";
    Actor bob = "bob";
    Actor moderator = "smit";

    dynamic_global_property_service_i& dgp_service;
    betting_property_service_i& betting_property_service;
    account_service_i& account_service;
    game_service_i& game_service;
};

BOOST_FIXTURE_TEST_SUITE(budget_operations_tests, bet_operations_fixture)

SCORUM_TEST_CASE(post_bet_operation_check)
{
    generate_block();

    create_bet(uuid_gen("b1"), alice, result_home::yes{}, { 10, 2 }, alice.scr_amount / 2);

    generate_block();

    create_bet(uuid_gen("b2"), bob, result_home::yes{}, { 10, 8 }, bob.scr_amount / 2);

    generate_block();
}

SCORUM_TEST_CASE(cancel_single_pending_bet_by_better_operation_check)
{
    generate_block();

    create_bet(uuid_gen("b1"), alice, result_home::yes{}, { 10, 2 }, alice.scr_amount / 2);

    generate_block();

    cancel_pending_bet(alice, { uuid_gen("b1") });

    generate_block();
}

SCORUM_TEST_CASE(cancel_some_pending_bets_by_better_operation_check)
{
    generate_block();

    create_bet(uuid_gen("b1"), alice, total::under{ 2000 }, { 10, 2 }, alice.scr_amount / 2);

    generate_block();

    create_bet(uuid_gen("b2"), alice, result_home::yes(), { 10, 5 }, alice.scr_amount / 2);

    generate_block();

    create_bet(uuid_gen("b3"), bob, total::over{ 2000 }, { 10, 8 }, bob.scr_amount / 2);

    generate_block();

    cancel_pending_bet(alice, { uuid_gen("b1"), uuid_gen("b2") });

    generate_block();
}

BOOST_AUTO_TEST_SUITE_END()
}
