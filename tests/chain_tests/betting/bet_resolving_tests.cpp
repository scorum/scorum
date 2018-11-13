#include <boost/test/unit_test.hpp>

#include <scorum/protocol/proposal_operations.hpp>
#include <scorum/protocol/betting/market_kind.hpp>

#include <scorum/chain/schema/account_objects.hpp>
#include <scorum/chain/schema/game_object.hpp>
#include <scorum/chain/schema/betting_property_object.hpp>
#include <scorum/chain/dba/db_accessor.hpp>

#include "defines.hpp"
#include "detail.hpp"

#include "database_betting_integration.hpp"
#include "actor.hpp"

namespace {

using namespace scorum::protocol;
using namespace scorum::chain;
using namespace database_fixture;

struct bet_resolving_fixture : public database_fixture::database_betting_integration_fixture
{
    bet_resolving_fixture()
        : account_dba(db)
        , game_dba(db)
        , betting_prop_dba(db)
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
    }

    Actor alice = "alice";
    Actor bob = "bob";
    Actor moderator = "smit";

    dba::db_accessor<account_object> account_dba;
    dba::db_accessor<game_object> game_dba;
    dba::db_accessor<betting_property_object> betting_prop_dba;
};

BOOST_FIXTURE_TEST_SUITE(bet_resolving_tests, bet_resolving_fixture)

SCORUM_TEST_CASE(live_bets_no_pending_bets_return_after_start_check)
{
    const auto& alice_acc = account_dba.get_by<by_name>(alice.name);
    const auto& bob_acc = account_dba.get_by<by_name>(bob.name);

    create_game(moderator, { result_away{}, total{ 2000 } }, SCORUM_BLOCK_INTERVAL * 2);
    generate_block();

    create_bet(gen_uuid("b1"), alice, result_away::yes{}, { 10, 2 }, alice.scr_amount / 2); // 500'000'000
    create_bet(gen_uuid("b2"), bob, result_away::no{}, { 10, 8 }, bob.scr_amount / 2); // 500'000'000

    BOOST_CHECK(game_dba.get().status == game_status::created);
    BOOST_CHECK_EQUAL(alice_acc.balance.amount, 500'000'000);
    BOOST_CHECK_EQUAL(bob_acc.balance.amount, 500'000'000);

    generate_block(); // game started

    BOOST_CHECK(game_dba.get().status == game_status::started);
    BOOST_CHECK_EQUAL(alice_acc.balance.amount, 500'000'000); // pending bets are keeping for live bets
    BOOST_CHECK_EQUAL(bob_acc.balance.amount, 500'000'000); // pending bets are keeping for live bets
}

SCORUM_TEST_CASE(non_live_bets_return_pending_bets_after_start_check)
{
    const auto& alice_acc = account_dba.get_by<by_name>(alice.name);
    const auto& bob_acc = account_dba.get_by<by_name>(bob.name);

    create_game(moderator, { result_away{}, total{ 2000 } }, SCORUM_BLOCK_INTERVAL * 2);
    generate_block();

    create_bet(gen_uuid("b1"), alice, result_away::yes{}, { 10, 2 }, alice.scr_amount / 2, false); // 500'000'000
    create_bet(gen_uuid("b2"), bob, result_away::no{}, { 10, 8 }, bob.scr_amount / 2, false); // 500'000'000

    BOOST_CHECK(game_dba.get().status == game_status::created);
    BOOST_CHECK_EQUAL(alice_acc.balance.amount, 500'000'000);
    BOOST_CHECK_EQUAL(bob_acc.balance.amount, 500'000'000);

    generate_block(); // game started

    BOOST_CHECK(game_dba.get().status == game_status::started);
    BOOST_CHECK_EQUAL(alice_acc.balance.amount,
                      500'000'000 + 375'000'000); // 375'000'000 returned after game was started
    BOOST_CHECK_EQUAL(bob_acc.balance.amount, 500'000'000); // nothing to return
}

SCORUM_TEST_CASE(non_live_bets_resolve_test_check)
{
    const auto& alice_acc = account_dba.get_by<by_name>(alice.name);
    const auto& bob_acc = account_dba.get_by<by_name>(bob.name);

    create_game(moderator, { result_away{}, total{ 2000 } }, SCORUM_BLOCK_INTERVAL * 2);
    generate_block();

    create_bet(gen_uuid("b1"), alice, result_away::yes{}, { 10, 2 }, alice.scr_amount / 2); // 500'000'000
    create_bet(gen_uuid("b2"), bob, result_away::no{}, { 10, 8 }, bob.scr_amount / 2); // 500'000'000

    generate_block(); // game started

    BOOST_CHECK_EQUAL(alice_acc.balance.amount, 500'000'000); // live pending bets weren't returned
    BOOST_CHECK_EQUAL(bob_acc.balance.amount, 500'000'000);

    auto resolve_delay = betting_prop_dba.get().resolve_delay_sec;

    post_results(moderator, { result_away::no{} });

    generate_blocks(db.head_block_time() + resolve_delay / 2);

    BOOST_CHECK_EQUAL(alice_acc.balance.amount, 500'000'000 + 375'000'000); // live pending bets were returned
    BOOST_CHECK_EQUAL(bob_acc.balance.amount, 500'000'000);

    generate_blocks(db.head_block_time() + resolve_delay / 2);

    BOOST_CHECK_EQUAL(alice_acc.balance.amount, 500'000'000 + 375'000'000);
    BOOST_CHECK_EQUAL(bob_acc.balance.amount,
                      500'000'000 + 500'000'000 + 125'000'000); // returned it's own bet + won 125'000'000
}

SCORUM_TEST_CASE(bets_auto_resolve_test_check)
{
    const auto& alice_acc = account_dba.get_by<by_name>(alice.name);
    const auto& bob_acc = account_dba.get_by<by_name>(bob.name);

    create_game(moderator, { result_away{}, total{ 2000 } }, SCORUM_BLOCK_INTERVAL * 2);
    auto start_time = game_dba.get().start_time;
    generate_block();

    create_bet(gen_uuid("b1"), alice, result_away::yes{}, { 10, 2 }, alice.scr_amount / 2); // 500'000'000
    create_bet(gen_uuid("b2"), bob, result_away::no{}, { 10, 8 }, bob.scr_amount / 2); // 500'000'000

    generate_block(); // game started

    BOOST_CHECK_EQUAL(alice_acc.balance.amount, 500'000'000); // pending bets are keeping for live bets
    BOOST_CHECK_EQUAL(bob_acc.balance.amount, 500'000'000);

    generate_blocks(start_time + auto_resolve_delay_default);

    BOOST_CHECK_EQUAL(alice_acc.balance.amount, 500'000'000 + 500'000'000); // returned all
    BOOST_CHECK_EQUAL(bob_acc.balance.amount, 500'000'000 + 500'000'000); // returned all
}

SCORUM_TEST_CASE(cancel_game_before_start_return_all_check)
{
    const auto& alice_acc = account_dba.get_by<by_name>(alice.name);
    const auto& bob_acc = account_dba.get_by<by_name>(bob.name);

    create_game(moderator, { result_away{}, total{ 2000 } }, SCORUM_BLOCK_INTERVAL * 3);
    generate_block();

    create_bet(gen_uuid("b1"), alice, result_away::yes{}, { 10, 2 }, alice.scr_amount / 2); // 500'000'000
    create_bet(gen_uuid("b2"), bob, result_away::no{}, { 10, 8 }, bob.scr_amount / 2); // 500'000'000

    generate_block();

    BOOST_CHECK_EQUAL(alice_acc.balance.amount, 500'000'000);
    BOOST_CHECK_EQUAL(bob_acc.balance.amount, 500'000'000);

    cancel_game(moderator);

    BOOST_CHECK_EQUAL(alice_acc.balance.amount, 500'000'000 + 500'000'000); // returned all
    BOOST_CHECK_EQUAL(bob_acc.balance.amount, 500'000'000 + 500'000'000); // returned all
}

SCORUM_TEST_CASE(cancel_game_after_start_return_all_check)
{
    const auto& alice_acc = account_dba.get_by<by_name>(alice.name);
    const auto& bob_acc = account_dba.get_by<by_name>(bob.name);

    create_game(moderator, { result_away{}, total{ 2000 } }, SCORUM_BLOCK_INTERVAL * 2);
    generate_block();

    create_bet(gen_uuid("b1"), alice, result_away::yes{}, { 10, 2 }, alice.scr_amount / 2); // 500'000'000
    create_bet(gen_uuid("b2"), bob, result_away::no{}, { 10, 8 }, bob.scr_amount / 2); // 500'000'000

    BOOST_CHECK_EQUAL(alice_acc.balance.amount, 500'000'000);
    BOOST_CHECK_EQUAL(bob_acc.balance.amount, 500'000'000);

    generate_block(); // game started
    cancel_game(moderator);

    BOOST_CHECK_EQUAL(alice_acc.balance.amount, 500'000'000 + 500'000'000); // returned all
    BOOST_CHECK_EQUAL(bob_acc.balance.amount, 500'000'000 + 500'000'000); // returned all
}

SCORUM_TEST_CASE(update_markets_trigger_bets_cancelling_check)
{
    const auto& alice_acc = account_dba.get_by<by_name>(alice.name);
    const auto& bob_acc = account_dba.get_by<by_name>(bob.name);

    create_game(moderator, { result_away{}, result_draw{} }, SCORUM_BLOCK_INTERVAL * 3);
    generate_block();

    create_bet(gen_uuid("b1"), alice, result_away::yes{}, { 10, 2 }, alice.scr_amount / 4); // 250'000'000
    create_bet(gen_uuid("b2"), bob, result_away::no{}, { 10, 8 }, bob.scr_amount / 4); // 250'000'000
    create_bet(gen_uuid("b3"), alice, result_draw::yes{}, { 10, 2 }, alice.scr_amount * 3 / 4); // 750'000'000
    create_bet(gen_uuid("b4"), bob, result_draw::no{}, { 10, 8 }, bob.scr_amount * 3 / 4); // 750'000'000

    generate_block();

    BOOST_CHECK_EQUAL(alice_acc.balance.amount, 0);
    BOOST_CHECK_EQUAL(bob_acc.balance.amount, 0);

    update_markets(moderator, { result_draw{} });

    BOOST_CHECK_EQUAL(alice_acc.balance.amount, 250'000'000);
    BOOST_CHECK_EQUAL(bob_acc.balance.amount, 250'000'000);
}

SCORUM_TEST_CASE(cancel_game_after_markets_update_check)
{
    const auto& alice_acc = account_dba.get_by<by_name>(alice.name);
    const auto& bob_acc = account_dba.get_by<by_name>(bob.name);

    create_game(moderator, { result_away{}, total{ 2000 } }, SCORUM_BLOCK_INTERVAL * 4);
    generate_block();

    create_bet(gen_uuid("b1"), alice, result_away::yes{}, { 10, 2 },
               alice.scr_amount / 4); // 250'000'000 (125'000'000 matched)
    create_bet(gen_uuid("b2"), bob, result_away::no{}, { 10, 8 },
               bob.scr_amount / 2); // 500'000'000 (500'000'000 matched)

    create_bet(gen_uuid("b3"), alice, total::over{ 2000 }, { 10, 2 },
               alice.scr_amount / 2); // 500'000'000 (62'500'000 matched)
    create_bet(gen_uuid("b4"), bob, total::under{ 2000 }, { 10, 8 },
               bob.scr_amount / 4); // 250'000'000 (250'000'000 matched)

    generate_block();

    BOOST_CHECK_EQUAL(alice_acc.balance.amount, 250'000'000);
    BOOST_CHECK_EQUAL(bob_acc.balance.amount, 250'000'000);

    // game isn't started yet so 'result_home_market' market will be cancelled
    update_markets(moderator, { total{ 2000 } });
    generate_block();

    BOOST_CHECK_EQUAL(alice_acc.balance.amount,
                      250'000'000 + 125'000'000 + 125'000'000); // matched 125000+pending 125000 returned
    BOOST_CHECK_EQUAL(bob_acc.balance.amount, 250'000'000 + 500'000'000); // matched 500'000'000 + 0 pending returned

    generate_block(); // game started now.

    BOOST_CHECK_EQUAL(alice_acc.balance.amount, 500'000'000); // live. no pending bets return
    BOOST_CHECK_EQUAL(bob_acc.balance.amount, 750'000'000); // live. no pending bets return

    cancel_game(moderator);

    BOOST_CHECK_EQUAL(alice_acc.balance.amount,
                      500'000'000 + 62'500'000 + 437'500'000); // matched 62'500+pending 437'500 returned
    BOOST_CHECK_EQUAL(bob_acc.balance.amount, 750'000'000 + 250'000'000); // matched 250'000'000 + 0 pending returned
}

SCORUM_TEST_CASE(game_resolve_time_is_after_auto_resolve_time)
{
    create_game(moderator, { result_away{} }, SCORUM_BLOCK_INTERVAL, SCORUM_BLOCK_INTERVAL * 4);

    generate_blocks(3);
    post_results(moderator, { result_away::yes{} });
    generate_block();

    BOOST_CHECK(game_dba.is_exists_by<by_id>(0));

    generate_block();

    BOOST_CHECK(game_dba.is_exists_by<by_id>(0));
}

BOOST_AUTO_TEST_SUITE_END()
}
