#include <boost/test/unit_test.hpp>

#include "defines.hpp"

#include "database_betting_integration.hpp"
#include "actor.hpp"

#include <scorum/protocol/proposal_operations.hpp>

#include <scorum/chain/dba/db_accessor.hpp>
#include <scorum/chain/services/dynamic_global_property.hpp>
#include <scorum/chain/services/proposal.hpp>
#include <scorum/chain/services/betting_property.hpp>
#include <scorum/chain/services/account.hpp>
#include <scorum/chain/services/game.hpp>
#include <scorum/chain/services/matched_bet.hpp>
#include <scorum/chain/services/pending_bet.hpp>

#include <scorum/protocol/betting/market_kind.hpp>

#include <scorum/utils/collect_range_adaptor.hpp>

namespace {
using namespace scorum::utils::adaptors;
using namespace scorum::protocol;
using namespace scorum::chain;
using namespace database_fixture;

struct game_operations_fixture : public database_fixture::database_betting_integration_fixture
{
    game_operations_fixture()
        : dgp_service(db.dynamic_global_property_service())
        , betting_property_service(db.betting_property_service())
        , account_service(db.account_service())
        , game_service(db.game_service())
        , matched_bet_service(db.matched_bet_service())
        , pending_bet_service(db.pending_bet_service())
        , matched_bet_dba(db.get_dba<matched_bet_object>())
        , pending_bet_dba(db.get_dba<pending_bet_object>())
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
    }

    Actor alice = "alice";
    Actor bob = "bob";
    Actor moderator = "smit";

    dynamic_global_property_service_i& dgp_service;
    betting_property_service_i& betting_property_service;
    account_service_i& account_service;
    game_service_i& game_service;
    matched_bet_service_i& matched_bet_service;
    pending_bet_service_i& pending_bet_service;
    dba::db_accessor<matched_bet_object>& matched_bet_dba;
    dba::db_accessor<pending_bet_object>& pending_bet_dba;

    std::vector<matched_bet_object> get_matched_bets()
    {
        auto matched_bets
            = matched_bet_dba.get_range_by<by_game_uuid_market>(uuid_gen("test")) | collect<std::vector>();
        return matched_bets;
    }

    std::vector<pending_bet_object> get_pending_bets()
    {
        auto pending_bets
            = pending_bet_dba.get_range_by<by_game_uuid_market>(uuid_gen("test")) | collect<std::vector>();
        return pending_bets;
    }
};

BOOST_FIXTURE_TEST_SUITE(post_game_results_operation_tests, game_operations_fixture)

SCORUM_TEST_CASE(post_game_results_positive_test)
{
    // clang-format off
    create_game(moderator, {
        result_home{},
        total{ 2000 },
        total{ 500 },
        correct_score{ 0, 0 } });
    // clang-format on

    auto creation_time = db.head_block_time();

    generate_block();

    const auto& game = db.get(game_object::id_type(0));

    BOOST_REQUIRE_EQUAL((int)game.status, (int)game_status::started);
    BOOST_REQUIRE_EQUAL(game.results.size(), 0u);
    BOOST_REQUIRE_EQUAL(game.last_update.to_iso_string(), creation_time.to_iso_string());

    post_results(moderator, { result_home::no{}, total::over{ 500 }, correct_score::no{ 0, 0 } });
    auto update_time = db.head_block_time();

    generate_block();

    BOOST_REQUIRE_EQUAL((int)game.status, (int)game_status::finished);
    BOOST_REQUIRE_EQUAL(game.results.size(), 3u);
    BOOST_REQUIRE_EQUAL(game.bets_resolve_time.to_iso_string(),
                        (update_time + SCORUM_BETTING_RESOLVE_DELAY_SEC).to_iso_string());
}

SCORUM_TEST_CASE(post_game_results_twice_positive_test)
{
    // clang-format off
    create_game(moderator, {
        result_home{},
        total{ 2000 },
        total{ 500 },
        correct_score{ 0, 0 } });
    // clang-format on

    generate_block();

    const auto& game = db.get(game_object::id_type(0));

    auto fst_update_time = db.head_block_time();
    post_results(moderator, { result_home::no{}, total::over{ 500 }, correct_score::no{ 0, 0 } });

    BOOST_REQUIRE_EQUAL((int)game.status, (int)game_status::finished);
    BOOST_REQUIRE_EQUAL(game.results.size(), 3u);
    BOOST_REQUIRE_EQUAL(game.last_update.to_iso_string(), fst_update_time.to_iso_string());
    BOOST_REQUIRE_EQUAL(game.bets_resolve_time.to_iso_string(),
                        (fst_update_time + SCORUM_BETTING_RESOLVE_DELAY_SEC).to_iso_string());

    auto snd_update_time = db.head_block_time();

    post_results(moderator,
                 { result_home::no{}, total::under{ 500 }, total::under{ 2000 }, correct_score::no{ 0, 0 } });

    BOOST_REQUIRE_EQUAL((int)game.status, (int)game_status::finished);
    BOOST_REQUIRE_EQUAL(game.results.size(), 4u);
    BOOST_REQUIRE_EQUAL(game.last_update.to_iso_string(), snd_update_time.to_iso_string());
    BOOST_REQUIRE_EQUAL(game.bets_resolve_time.to_iso_string(),
                        (fst_update_time + SCORUM_BETTING_RESOLVE_DELAY_SEC).to_iso_string());
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_FIXTURE_TEST_SUITE(update_start_time_operation_tests, game_operations_fixture)

SCORUM_TEST_CASE(auto_resolve_time_updating_test)
{
    auto time = db.head_block_time();

    create_game(moderator, { result_home{}, total{ 2000 } }, SCORUM_BLOCK_INTERVAL * 2);
    generate_block();

    BOOST_CHECK_EQUAL((game_service.get().start_time - time).to_seconds(), SCORUM_BLOCK_INTERVAL * 2);
    BOOST_CHECK_EQUAL((game_service.get().auto_resolve_time - game_service.get().start_time).to_seconds(),
                      auto_resolve_delay_default);

    time = db.head_block_time();
    update_start_time(moderator, SCORUM_BLOCK_INTERVAL * 3);

    BOOST_CHECK_EQUAL((game_service.get().start_time - time).to_seconds(), SCORUM_BLOCK_INTERVAL * 3);
    BOOST_CHECK_EQUAL((game_service.get().auto_resolve_time - game_service.get().start_time).to_seconds(),
                      auto_resolve_delay_default);
}

SCORUM_TEST_CASE(update_after_game_started_should_cancel_bets)
{
    create_game(moderator, { result_home{}, total{ 2000 } }, SCORUM_BLOCK_INTERVAL);
    generate_block(); // game started
    generate_block(); // generate one more block in order to make bets creation time diff from start_time

    create_bet(uuid_gen("b1"), alice, total::under{ 2000 }, { 10, 2 }, ASSET_SCR(1000)); // 250 matched, 750 pending
    create_bet(uuid_gen("b2"), bob, total::over{ 2000 }, { 10, 8 }, ASSET_SCR(1000)); // 1000 matched

    generate_block();

    {
        auto matched_bets = get_matched_bets();
        auto pending_bets = get_pending_bets();
        BOOST_REQUIRE_EQUAL(matched_bets.size(), 1u);
        BOOST_REQUIRE_EQUAL(pending_bets.size(), 1u);
    }

    update_start_time(moderator, SCORUM_BLOCK_INTERVAL);

    {
        auto matched_bets = get_matched_bets();
        auto pending_bets = get_pending_bets();
        BOOST_REQUIRE_EQUAL(matched_bets.size(), 0u);
        BOOST_REQUIRE_EQUAL(pending_bets.size(), 0u);
    }
}

SCORUM_TEST_CASE(update_after_game_started_should_keep_bets)
{
    create_game(moderator, { result_home{}, total{ 2000 } }, SCORUM_BLOCK_INTERVAL * 2);
    generate_block();

    create_bet(uuid_gen("b1"), alice, total::under{ 2000 }, { 10, 2 }, ASSET_SCR(1000)); // 250 matched, 750 pending
    create_bet(uuid_gen("b2"), bob, total::over{ 2000 }, { 10, 8 }, ASSET_SCR(1000)); // 1000 matched

    generate_block(); // game started

    {
        auto matched_bets = get_matched_bets();
        auto pending_bets = get_pending_bets();
        BOOST_REQUIRE_EQUAL(matched_bets.size(), 1u);
        BOOST_REQUIRE_EQUAL(pending_bets.size(), 1u);
    }

    update_start_time(moderator, SCORUM_BLOCK_INTERVAL);

    {
        auto matched_bets = get_matched_bets();
        auto pending_bets = get_pending_bets();
        BOOST_REQUIRE_EQUAL(matched_bets.size(), 1u);
        BOOST_REQUIRE_EQUAL(pending_bets.size(), 1u);
    }
}

SCORUM_TEST_CASE(update_after_game_started_should_increase_existing_pending_bet)
{
    create_game(moderator, { result_home{}, total{ 2000 } }, SCORUM_BLOCK_INTERVAL * 2);
    generate_block();

    create_bet(uuid_gen("b1"), alice, total::under{ 2000 }, { 10, 2 }, ASSET_SCR(1000)); // 250 matched, 750 pending

    generate_block(); // game started

    create_bet(uuid_gen("b2"), bob, total::over{ 2000 }, { 10, 8 }, ASSET_SCR(1000)); // 1000 matched

    auto matched_bets = matched_bet_dba.get_range_by<by_game_uuid_market>(uuid_gen("test"));
    auto pending_bets = pending_bet_dba.get_range_by<by_game_uuid_market>(uuid_gen("test"));
    BOOST_REQUIRE_EQUAL(boost::size(matched_bets), 1u);
    BOOST_REQUIRE_EQUAL(boost::size(pending_bets), 1u);

    const pending_bet_object* pending_bet_address = std::addressof(*pending_bets.begin());

    update_start_time(moderator, SCORUM_BLOCK_INTERVAL);

    {
        auto matched_bets = matched_bet_dba.get_range_by<by_game_uuid_market>(uuid_gen("test"));
        auto pending_bets = pending_bet_dba.get_range_by<by_game_uuid_market>(uuid_gen("test"));
        BOOST_REQUIRE_EQUAL(boost::size(matched_bets), 0u);
        BOOST_REQUIRE_EQUAL(boost::size(pending_bets), 1u);
        BOOST_REQUIRE_EQUAL(pending_bets.begin()->data.stake.amount, 1000u);
        BOOST_REQUIRE_EQUAL(pending_bet_address,
                            std::addressof(*pending_bets.begin())); // check this is the same object
    }
}

SCORUM_TEST_CASE(update_after_game_started_should_restore_missing_pending_bet)
{
    create_game(moderator, { result_home{}, total{ 2000 } }, SCORUM_BLOCK_INTERVAL * 2);
    generate_block();

    auto original_create_time = db.head_block_time();

    create_bet(uuid_gen("b1"), alice, total::under{ 2000 }, { 10, 2 }, ASSET_SCR(1000)); // 1000 matched

    generate_block(); // game started

    create_bet(uuid_gen("b2"), bob, total::over{ 2000 }, { 10, 8 }, ASSET_SCR(4000)); // 4000 matched

    auto matched_bets = get_matched_bets();
    auto pending_bets = get_pending_bets();
    BOOST_REQUIRE_EQUAL(matched_bets.size(), 1u);
    BOOST_REQUIRE_EQUAL(pending_bets.size(), 0u);

    update_start_time(moderator, SCORUM_BLOCK_INTERVAL);

    {
        auto matched_bets = get_matched_bets();
        auto pending_bets = get_pending_bets();
        BOOST_REQUIRE_EQUAL(matched_bets.size(), 0u);
        BOOST_REQUIRE_EQUAL(pending_bets.size(), 1u);
        const auto& fst_pbet = *pending_bets.begin();
        BOOST_CHECK_EQUAL(fst_pbet.data.better, "alice");
        BOOST_CHECK_EQUAL(fst_pbet.data.stake.amount, 1000);
        BOOST_CHECK_EQUAL(fst_pbet.data.created.to_iso_string(), original_create_time.to_iso_string());
        BOOST_CHECK_EQUAL(fst_pbet.data.odds, (odds{ 10, 2 }));
        BOOST_CHECK_EQUAL(fst_pbet.data.wincase.get<total::under>().threshold, 2000);
    }
}

SCORUM_TEST_CASE(update_after_game_started_should_change_status)
{
    create_game(moderator, { result_home{} }, SCORUM_BLOCK_INTERVAL);
    generate_block(); // game started

    BOOST_REQUIRE(game_service.get_game(0).status == game_status::started);

    update_start_time(moderator, SCORUM_BLOCK_INTERVAL);

    BOOST_CHECK(game_service.get_game(0).status == game_status::created);
}

BOOST_AUTO_TEST_SUITE_END()
}
