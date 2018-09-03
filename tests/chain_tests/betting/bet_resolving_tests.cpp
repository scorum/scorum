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

namespace {

using namespace scorum::protocol;
using namespace scorum::protocol::betting;
using namespace scorum::chain;
using namespace database_fixture;

struct bet_resolving_fixture : public database_fixture::database_betting_integration_fixture
{
    bet_resolving_fixture()
    {
        open_database();

        alice.scorum(ASSET_SCR(1e+6));
        actor(initdelegate).create_account(alice);
        actor(initdelegate).give_sp(alice, 1e+6);
        actor(initdelegate).give_scr(alice, alice.scr_amount.amount.value);

        bob.scorum(ASSET_SCR(1e+6));
        actor(initdelegate).create_account(bob);
        actor(initdelegate).give_sp(bob, 1e+6);
        actor(initdelegate).give_scr(bob, bob.scr_amount.amount.value);

        actor(initdelegate).create_account(moderator);
        actor(initdelegate).give_sp(moderator, 1e+6);
        actor(initdelegate).give_scr(moderator, 1e+6);

        empower_moderator(moderator);

        BOOST_REQUIRE(betting_property_service.is_exists());
        BOOST_REQUIRE_EQUAL(betting_property_service.get().moderator, moderator.name);
    }

    Actor alice = "alice";
    Actor bob = "bob";
    Actor moderator = "smit";
};

BOOST_FIXTURE_TEST_SUITE(bet_resolving_tests, bet_resolving_fixture)

SCORUM_TEST_CASE(bets_resolve_test_check)
{
    const auto& alice_acc = account_service.get_account(alice.name);
    const auto& bob_acc = account_service.get_account(bob.name);

    create_game(moderator, { result_home_market{}, total_market{ 2000 } }, fc::seconds(SCORUM_BLOCK_INTERVAL * 2));
    generate_block();

    create_bet(alice, result_home{}, { 10, 2 }, asset(alice.scr_amount.amount / 2, SCORUM_SYMBOL)); // 500'000
    create_bet(bob, result_draw_away{}, { 10, 8 }, asset(bob.scr_amount.amount / 2, SCORUM_SYMBOL)); // 500'000

    BOOST_CHECK_EQUAL(alice_acc.balance.amount, 500'000);
    BOOST_CHECK_EQUAL(bob_acc.balance.amount, 500'000);

    generate_block(); // game started

    BOOST_CHECK_EQUAL(alice_acc.balance.amount, 500'000); // pending bets are keeping for live bets
    BOOST_CHECK_EQUAL(bob_acc.balance.amount, 500'000);

    auto resolve_delay = betting_property_service.get().resolve_delay_sec;

    post_results(moderator, { result_draw_away{} });

    generate_blocks(db.head_block_time() + resolve_delay / 2);

    BOOST_CHECK_EQUAL(alice_acc.balance.amount, 500'000);
    BOOST_CHECK_EQUAL(bob_acc.balance.amount, 500'000);

    generate_blocks(db.head_block_time() + resolve_delay / 2);

    BOOST_CHECK_EQUAL(alice_acc.balance.amount, 500'000 + 375'000); // returned unmatched
    BOOST_CHECK_EQUAL(bob_acc.balance.amount, 500'000 + 500'000 + 125'000); // returned it's own bet + won 125'000
}

SCORUM_TEST_CASE(bets_auto_resolve_test_check)
{
    const auto& alice_acc = account_service.get_account(alice.name);
    const auto& bob_acc = account_service.get_account(bob.name);

    auto start_time = db.head_block_time();
    create_game(moderator, { result_home_market{}, total_market{ 2000 } }, fc::seconds(SCORUM_BLOCK_INTERVAL * 2));
    generate_block();

    create_bet(alice, result_home{}, { 10, 2 }, asset(alice.scr_amount.amount / 2, SCORUM_SYMBOL)); // 500'000
    create_bet(bob, result_draw_away{}, { 10, 8 }, asset(bob.scr_amount.amount / 2, SCORUM_SYMBOL)); // 500'000

    BOOST_CHECK_EQUAL(alice_acc.balance.amount, 500'000);
    BOOST_CHECK_EQUAL(bob_acc.balance.amount, 500'000);

    generate_block(); // game started

    BOOST_CHECK_EQUAL(alice_acc.balance.amount, 500'000); // pending bets are keeping for live bets
    BOOST_CHECK_EQUAL(bob_acc.balance.amount, 500'000);

    generate_blocks(start_time + auto_resolve_delay_default);

    BOOST_CHECK_EQUAL(alice_acc.balance.amount, 500'000 + 500'000); // returned all
    BOOST_CHECK_EQUAL(bob_acc.balance.amount, 500'000 + 500'000); // returned all
}

SCORUM_TEST_CASE(cancel_game_before_start_return_all_check)
{
    const auto& alice_acc = account_service.get_account(alice.name);
    const auto& bob_acc = account_service.get_account(bob.name);

    create_game(moderator, { result_home_market{}, total_market{ 2000 } }, fc::seconds(SCORUM_BLOCK_INTERVAL * 3));
    generate_block();

    create_bet(alice, result_home{}, { 10, 2 }, asset(alice.scr_amount.amount / 2, SCORUM_SYMBOL)); // 500'000
    create_bet(bob, result_draw_away{}, { 10, 8 }, asset(bob.scr_amount.amount / 2, SCORUM_SYMBOL)); // 500'000

    generate_block();

    BOOST_CHECK_EQUAL(alice_acc.balance.amount, 500'000);
    BOOST_CHECK_EQUAL(bob_acc.balance.amount, 500'000);

    cancel_game(moderator);

    BOOST_CHECK_EQUAL(alice_acc.balance.amount, 500'000 + 500'000); // returned all
    BOOST_CHECK_EQUAL(bob_acc.balance.amount, 500'000 + 500'000); // returned all
}

SCORUM_TEST_CASE(cancel_game_after_start_return_all_check)
{
    const auto& alice_acc = account_service.get_account(alice.name);
    const auto& bob_acc = account_service.get_account(bob.name);

    create_game(moderator, { result_home_market{}, total_market{ 2000 } }, fc::seconds(SCORUM_BLOCK_INTERVAL * 2));
    generate_block();

    create_bet(alice, result_home{}, { 10, 2 }, asset(alice.scr_amount.amount / 2, SCORUM_SYMBOL)); // 500'000
    create_bet(bob, result_draw_away{}, { 10, 8 }, asset(bob.scr_amount.amount / 2, SCORUM_SYMBOL)); // 500'000

    BOOST_CHECK_EQUAL(alice_acc.balance.amount, 500'000);
    BOOST_CHECK_EQUAL(bob_acc.balance.amount, 500'000);

    generate_block(); // game started
    cancel_game(moderator);

    BOOST_CHECK_EQUAL(alice_acc.balance.amount, 500'000 + 500'000); // returned all
    BOOST_CHECK_EQUAL(bob_acc.balance.amount, 500'000 + 500'000); // returned all
}

SCORUM_TEST_CASE(update_markets_trigger_bets_cancelling_check)
{
    const auto& alice_acc = account_service.get_account(alice.name);
    const auto& bob_acc = account_service.get_account(bob.name);

    create_game(moderator, { result_home_market{}, result_draw_market{} }, fc::seconds(SCORUM_BLOCK_INTERVAL * 3));
    generate_block();

    create_bet(alice, result_home{}, { 10, 2 }, asset(alice.scr_amount.amount / 4, SCORUM_SYMBOL)); // 250'000
    create_bet(bob, result_draw_away{}, { 10, 8 }, asset(bob.scr_amount.amount / 4, SCORUM_SYMBOL)); // 250'000
    create_bet(alice, result_draw{}, { 10, 2 }, asset(3 * alice.scr_amount.amount / 4, SCORUM_SYMBOL)); // 750'000
    create_bet(bob, result_home_away{}, { 10, 8 }, asset(3 * bob.scr_amount.amount / 4, SCORUM_SYMBOL)); // 750'000

    generate_block();

    BOOST_CHECK_EQUAL(alice_acc.balance.amount, 0);
    BOOST_CHECK_EQUAL(bob_acc.balance.amount, 0);

    update_markets(moderator, { result_draw_market{} });

    BOOST_CHECK_EQUAL(alice_acc.balance.amount, 250'000);
    BOOST_CHECK_EQUAL(bob_acc.balance.amount, 250'000);
}

BOOST_AUTO_TEST_SUITE_END()
}
