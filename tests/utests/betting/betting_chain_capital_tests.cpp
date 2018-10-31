#include <boost/test/unit_test.hpp>

#include <scorum/chain/database/block_tasks/process_bets_resolving.hpp>
#include <scorum/chain/database/block_tasks/process_bets_auto_resolving.hpp>

#include <scorum/chain/schema/account_objects.hpp>
#include <scorum/chain/schema/betting_property_object.hpp>
#include <scorum/chain/schema/bet_objects.hpp>
#include <scorum/chain/schema/game_object.hpp>
#include <scorum/chain/schema/dynamic_global_property_object.hpp>
#include <scorum/chain/schema/chain_property_object.hpp>

#include <scorum/chain/dba/db_accessor.hpp>
#include <scorum/chain/services/dynamic_global_property.hpp>
#include <scorum/chain/services/witness.hpp>
#include <scorum/chain/services/witness_schedule.hpp>
#include <scorum/chain/services/account.hpp>
#include <scorum/chain/services/betting_property.hpp>

#include <scorum/chain/betting/betting_service.hpp>
#include <scorum/chain/betting/betting_matcher.hpp>
#include <scorum/chain/betting/betting_resolver.hpp>
#include <scorum/protocol/betting/market.hpp>

#include <db_mock.hpp>
#include <hippomocks.h>
#include <defines.hpp>

namespace {
using namespace scorum;
using namespace scorum::chain;
using namespace scorum::protocol;
using namespace scorum::chain::database_ns;

struct chain_capital_fixture
{
    chain_capital_fixture()
        : betting_prop_dba(db)
        , matched_bet_dba(db)
        , pending_bet_dba(db)
        , game_dba(db)
        , dprop_dba(db)
        , account_dba(db)
        , uuid_hist_dba(db)
        , chain_dba(db)
        , dprop_svc(db)
        , witness_schedule_svc(db)
        , witness_svc(db, witness_schedule_svc, dprop_svc, chain_dba)
        , account_svc(db, dprop_svc, witness_svc)
    {
        db.add_index<betting_property_index>();
        db.add_index<pending_bet_index>();
        db.add_index<matched_bet_index>();
        db.add_index<game_index>();
        db.add_index<account_index>();
        db.add_index<dynamic_global_property_index>();
        db.add_index<bet_uuid_history_index>();

        mocks.OnCall(vop_emitter, database_virtual_operations_emmiter_i::push_virtual_operation);
    }

    MockRepository mocks;
    database_virtual_operations_emmiter_i* vop_emitter = mocks.Mock<database_virtual_operations_emmiter_i>();

    db_mock db;
    dba::db_accessor<betting_property_object> betting_prop_dba;
    dba::db_accessor<matched_bet_object> matched_bet_dba;
    dba::db_accessor<pending_bet_object> pending_bet_dba;
    dba::db_accessor<game_object> game_dba;
    dba::db_accessor<dynamic_global_property_object> dprop_dba;
    dba::db_accessor<account_object> account_dba;
    dba::db_accessor<bet_uuid_history_object> uuid_hist_dba;
    dba::db_accessor<chain_property_object> chain_dba;

    dbs_dynamic_global_property dprop_svc;
    dbs_witness_schedule witness_schedule_svc;
    dbs_witness witness_svc;
    dbs_account account_svc;
};

BOOST_FIXTURE_TEST_SUITE(betting_chain_capital_tests, chain_capital_fixture)

SCORUM_TEST_CASE(pending_bet_creation_should_change_betting_capital)
{
    // clang-format off
    dprop_dba.create([&](auto& o) { o.time = fc::time_point_sec(42); });
    account_dba.create([&](auto& o) { o.name = "alice"; o.balance = ASSET_SCR(1000); });
    // clang-format on

    betting_service svc(account_svc, *vop_emitter, betting_prop_dba, matched_bet_dba, pending_bet_dba, game_dba,
                        dprop_dba, uuid_hist_dba);

    svc.create_pending_bet("alice", ASSET_SCR(1000), odds(10, 2), result_home::yes{}, { 0 }, uuid_type{ 1 },
                           pending_bet_kind::live);

    BOOST_CHECK_EQUAL(dprop_dba.get().betting_stats.pending_bets_volume.amount, 1000u);
    BOOST_CHECK_EQUAL(dprop_dba.get().betting_stats.matched_bets_volume.amount, 0u);
}

SCORUM_TEST_CASE(bets_resolving_should_change_matched_betting_capital)
{
    account_dba.create([](account_object&) {});
    game_dba.create([](game_object& o) {});
    matched_bet_dba.create([](matched_bet_object& o) {
        o.game_uuid = { 0 };
        o.market = result_home{};
        o.bet1_data.stake = ASSET_SCR(500);
        o.bet1_data.wincase = result_home::yes{};
        o.bet2_data.stake = ASSET_SCR(2000);
        o.bet2_data.wincase = result_home::no{};
    });
    dprop_dba.create([&](dynamic_global_property_object& o) { o.betting_stats.matched_bets_volume = ASSET_SCR(2500); });

    betting_resolver resolver(account_svc, *vop_emitter, matched_bet_dba, game_dba, dprop_dba);

    resolver.resolve_matched_bets({ 0 }, { result_home::yes{} });

    BOOST_CHECK_EQUAL(0u, dprop_dba.get().betting_stats.matched_bets_volume.amount);
}

SCORUM_TEST_CASE(cancel_all_bets_should_change_betting_capital)
{
    account_dba.create([](account_object&) {});
    game_dba.create([](game_object& o) {});
    pending_bet_dba.create([](pending_bet_object& o) {
        o.game_uuid = { 0 };
        o.data.stake = ASSET_SCR(1000);
    });
    matched_bet_dba.create([](matched_bet_object& o) {
        o.game_uuid = { 0 };
        o.market = result_home{};
        o.bet1_data.stake = ASSET_SCR(500);
        o.bet2_data.stake = ASSET_SCR(2000);
    });
    dprop_dba.create([&](dynamic_global_property_object& o) {
        o.time = fc::time_point_sec(42);
        o.betting_stats.pending_bets_volume = ASSET_SCR(1000);
        o.betting_stats.matched_bets_volume = ASSET_SCR(2500);
    });

    betting_service svc(account_svc, *vop_emitter, betting_prop_dba, matched_bet_dba, pending_bet_dba, game_dba,
                        dprop_dba, uuid_hist_dba);

    svc.cancel_bets({ 0 });

    BOOST_CHECK_EQUAL(dprop_dba.get().betting_stats.pending_bets_volume.amount, 0u);
    BOOST_CHECK_EQUAL(dprop_dba.get().betting_stats.matched_bets_volume.amount, 0u);
}

SCORUM_TEST_CASE(cancel_bets_all_bets_created_after_game_started_should_be_removed_from_betting_stats)
{
    account_dba.create([](account_object&) {});
    const auto& g = game_dba.create([](game_object& o) { o.start_time = fc::time_point_sec(7); });
    pending_bet_dba.create([](pending_bet_object& o) {
        o.game_uuid = { 0 };
        o.data.created = fc::time_point_sec(42);
        o.data.stake = ASSET_SCR(1000);
    });
    matched_bet_dba.create([](matched_bet_object& o) {
        o.game_uuid = { 0 };
        o.market = result_home{};
        o.created = fc::time_point_sec(42);
        o.bet1_data.stake = ASSET_SCR(500);
        o.bet1_data.created = fc::time_point_sec(42);
        o.bet2_data.stake = ASSET_SCR(2000);
        o.bet2_data.created = fc::time_point_sec(42);
    });
    dprop_dba.create([&](dynamic_global_property_object& o) {
        o.time = fc::time_point_sec(99);
        o.betting_stats.pending_bets_volume = ASSET_SCR(1000);
        o.betting_stats.matched_bets_volume = ASSET_SCR(2500);
    });

    betting_service svc(account_svc, *vop_emitter, betting_prop_dba, matched_bet_dba, pending_bet_dba, game_dba,
                        dprop_dba, uuid_hist_dba);

    svc.cancel_bets({ 0 }, g.start_time);

    BOOST_CHECK_EQUAL(dprop_dba.get().betting_stats.pending_bets_volume.amount, 0u);
    BOOST_CHECK_EQUAL(dprop_dba.get().betting_stats.matched_bets_volume.amount, 0u);
}

SCORUM_TEST_CASE(cancel_bets_pending_bet_created_before_game_started_shouldnt_be_removed_from_betting_stats)
{
    const auto& g = game_dba.create([](game_object& o) { o.start_time = fc::time_point_sec(7); });
    pending_bet_dba.create([](pending_bet_object& o) {
        o.game_uuid = { 0 };
        o.data.created = fc::time_point_sec(5);
        o.data.stake = ASSET_SCR(1000);
    });
    dprop_dba.create([&](dynamic_global_property_object& o) {
        o.time = fc::time_point_sec(99);
        o.betting_stats.pending_bets_volume = ASSET_SCR(1000);
    });

    betting_service svc(account_svc, *vop_emitter, betting_prop_dba, matched_bet_dba, pending_bet_dba, game_dba,
                        dprop_dba, uuid_hist_dba);

    svc.cancel_bets({ 0 }, g.start_time);

    BOOST_CHECK_EQUAL(dprop_dba.get().betting_stats.pending_bets_volume.amount, 1000u);
}

SCORUM_TEST_CASE(
    cancel_bets_pending_bet_created_before_game_started_which_was_matched_after_game_started_shouldnt_be_removed_from_betting_stats)
{
    account_dba.create([](account_object&) {});
    const auto& g = game_dba.create([](game_object& o) { o.start_time = fc::time_point_sec(42); });
    matched_bet_dba.create([](matched_bet_object& o) {
        o.game_uuid = { 0 };
        o.market = result_home{};
        o.created = fc::time_point_sec(42);
        o.bet1_data.stake = ASSET_SCR(500);
        o.bet1_data.created = fc::time_point_sec(42);
        o.bet2_data.stake = ASSET_SCR(2000);
        o.bet2_data.created = fc::time_point_sec(13);
    });
    dprop_dba.create([&](dynamic_global_property_object& o) {
        o.time = fc::time_point_sec(99);
        o.betting_stats.matched_bets_volume = ASSET_SCR(2500);
    });

    betting_service svc(account_svc, *vop_emitter, betting_prop_dba, matched_bet_dba, pending_bet_dba, game_dba,
                        dprop_dba, uuid_hist_dba);

    svc.cancel_bets({ 0 }, g.start_time);

    BOOST_CHECK_EQUAL(dprop_dba.get().betting_stats.matched_bets_volume.amount, 0u);
    BOOST_CHECK_EQUAL(dprop_dba.get().betting_stats.pending_bets_volume.amount, 2000u);
}

SCORUM_TEST_CASE(cancel_pending_bets_only_live_pending_bet_should_be_removed_from_betting_stats)
{
    account_dba.create([](account_object&) {});
    game_dba.create([](game_object& o) {});
    pending_bet_dba.create([](pending_bet_object& o) {
        o.game_uuid = { 0 };
        o.data.uuid = { 0 };
        o.data.kind = pending_bet_kind::live;
        o.data.stake = ASSET_SCR(500);
    });
    pending_bet_dba.create([](pending_bet_object& o) {
        o.game_uuid = { 0 };
        o.data.uuid = { 1 };
        o.data.kind = pending_bet_kind::non_live;
        o.data.stake = ASSET_SCR(1000);
    });
    dprop_dba.create([&](dynamic_global_property_object& o) { o.betting_stats.pending_bets_volume = ASSET_SCR(1500); });

    betting_service svc(account_svc, *vop_emitter, betting_prop_dba, matched_bet_dba, pending_bet_dba, game_dba,
                        dprop_dba, uuid_hist_dba);

    svc.cancel_pending_bets({ 0 }, pending_bet_kind::live);

    BOOST_CHECK_EQUAL(dprop_dba.get().betting_stats.pending_bets_volume.amount, 1000u);
}

BOOST_AUTO_TEST_SUITE_END()
}
