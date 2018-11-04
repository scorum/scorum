#include <boost/test/unit_test.hpp>

#include <scorum/chain/dba/db_accessor.hpp>
#include <scorum/chain/schema/betting_property_object.hpp>
#include <scorum/chain/schema/game_object.hpp>

#include <scorum/protocol/betting/market_kind.hpp>

#include "defines.hpp"
#include "detail.hpp"
#include "database_betting_integration.hpp"
#include "actor.hpp"

namespace bet_operations_tests {

using namespace scorum::protocol;
using namespace scorum::chain;

struct bet_operations_fixture : public database_fixture::database_betting_integration_fixture
{
    bet_operations_fixture()
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

        BOOST_REQUIRE_EQUAL(this->betting_moderator(), moderator.name);

        create_game(moderator, { result_home{}, total{ 2000 } }, SCORUM_BLOCK_INTERVAL * 60 * 20);

        generate_block();

        BOOST_REQUIRE_EQUAL(1u, db.get_index<game_index>().indices().size());

        const auto& game = db.get(game_object::id_type(0));

        BOOST_REQUIRE_EQUAL(2u, game.markets.size());
    }

    Actor alice = "alice";
    Actor bob = "bob";
    Actor moderator = "smit";
};

BOOST_FIXTURE_TEST_SUITE(create_and_cancel_bet_tests, bet_operations_fixture)

SCORUM_TEST_CASE(post_bet_operation_check)
{
    generate_block();

    create_bet(gen_uuid("b1"), alice, result_home::yes{}, { 10, 2 }, alice.scr_amount / 2);

    generate_block();

    create_bet(gen_uuid("b2"), bob, result_home::yes{}, { 10, 8 }, bob.scr_amount / 2);

    generate_block();
}

SCORUM_TEST_CASE(cancel_single_pending_bet_by_better_operation_check)
{
    generate_block();

    create_bet(gen_uuid("b1"), alice, result_home::yes{}, { 10, 2 }, alice.scr_amount / 2);

    generate_block();

    cancel_pending_bet(alice, { gen_uuid("b1") });

    generate_block();
}

SCORUM_TEST_CASE(cancel_some_pending_bets_by_better_operation_check)
{
    generate_block();

    create_bet(gen_uuid("b1"), alice, total::under{ 2000 }, { 10, 2 }, alice.scr_amount / 2);

    generate_block();

    create_bet(gen_uuid("b2"), alice, result_home::yes(), { 10, 5 }, alice.scr_amount / 2);

    generate_block();

    create_bet(gen_uuid("b3"), bob, total::over{ 2000 }, { 10, 8 }, bob.scr_amount / 2);

    generate_block();

    cancel_pending_bet(alice, { gen_uuid("b1"), gen_uuid("b2") });

    generate_block();
}

SCORUM_TEST_CASE(throw_on_bet_with_existing_uuid)
{
    generate_block();

    create_bet(gen_uuid("b1"), alice, result_home::yes{}, { 10, 2 }, alice.scr_amount / 2);

    generate_block();

    BOOST_REQUIRE_THROW(create_bet(gen_uuid("b1"), bob, result_home::yes{}, { 10, 8 }, bob.scr_amount / 2),
                        fc::assert_exception);
}

BOOST_AUTO_TEST_SUITE_END()
}
