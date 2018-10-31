#include <boost/test/unit_test.hpp>

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

#include <defines.hpp>
#include <db_mock.hpp>

#include <hippomocks.h>

namespace {
using namespace scorum;
using namespace scorum::chain;
using namespace scorum::protocol;

struct betting_service_fixture
{
    MockRepository mocks;
    database_virtual_operations_emmiter_i* vop_emitter = mocks.Mock<database_virtual_operations_emmiter_i>();

    db_mock db;

    betting_service_fixture()
        : betting_prop_dba(db)
        , matched_bet_dba(db)
        , pending_bet_dba(db)
        , game_dba(db)
        , dprop_dba(db)
        , bet_uuid_hist_dba(db)
        , chain_dba(db)
        , account_dba(db)
        , dprop_svc(db)
        , witness_schedule_svc(db)
        , witness_svc(db, witness_schedule_svc, dprop_svc, chain_dba)
        , account_svc(db, dprop_svc, witness_svc)
    {
        db.add_index<account_index>();
        db.add_index<betting_property_index>();
        db.add_index<matched_bet_index>();
        db.add_index<pending_bet_index>();
        db.add_index<game_index>();
        db.add_index<dynamic_global_property_index>();
        db.add_index<bet_uuid_history_index>();
        db.add_index<chain_property_index>();
    }

    dba::db_accessor<betting_property_object> betting_prop_dba;
    dba::db_accessor<matched_bet_object> matched_bet_dba;
    dba::db_accessor<pending_bet_object> pending_bet_dba;
    dba::db_accessor<game_object> game_dba;
    dba::db_accessor<dynamic_global_property_object> dprop_dba;
    dba::db_accessor<bet_uuid_history_object> bet_uuid_hist_dba;
    dba::db_accessor<chain_property_object> chain_dba;
    dba::db_accessor<account_object> account_dba;

    dbs_dynamic_global_property dprop_svc;
    dbs_witness_schedule witness_schedule_svc;
    dbs_witness witness_svc;
    dbs_account account_svc;
};

BOOST_FIXTURE_TEST_SUITE(betting_service_tests, betting_service_fixture)

SCORUM_TEST_CASE(budget_service_is_betting_moderator_check)
{
    betting_prop_dba.create([](betting_property_object& o) { o.moderator = "moder"; });

    betting_service service(account_svc, *vop_emitter, betting_prop_dba, matched_bet_dba, pending_bet_dba, game_dba,
                            dprop_dba, bet_uuid_hist_dba);

    BOOST_CHECK(!service.is_betting_moderator("jack"));
    BOOST_CHECK(service.is_betting_moderator("moder"));
}

SCORUM_TEST_CASE(cancel_bets_1_bet_recovered_by_increasing_existing_pending_bet_check_stakes)
{
    dprop_dba.create([](dynamic_global_property_object&) {});
    account_dba.create([](account_object& o) { o.name = "better1"; });
    account_dba.create([](account_object& o) { o.name = "better2"; });
    matched_bet_dba.create([](matched_bet_object& o) {
        o.game_uuid = { 3 };
        o.created = fc::time_point_sec(42);
        o.bet1_data.uuid = { 1 };
        o.bet1_data.better = "better1";
        o.bet1_data.created = fc::time_point_sec(7); // created before start_time
        o.bet1_data.stake = ASSET_SCR(10); // this stake should be returned to the pending bet rest
        o.bet2_data.uuid = { 2 };
        o.bet2_data.better = "better2";
        o.bet2_data.created = fc::time_point_sec(42); // created after start_time
        o.bet2_data.stake = ASSET_SCR(20); // this stake should be cancelled and returned to owner
    });

    pending_bet_dba.create([](pending_bet_object& o) {
        o.game_uuid = { 3 };
        o.data.uuid = { 1 };
        o.data.created = fc::time_point_sec(7);
        o.data.stake = ASSET_SCR(5); // should be updated
    });

    mocks.OnCall(vop_emitter, database_virtual_operations_emmiter_i::push_virtual_operation);

    betting_service service(account_svc, *vop_emitter, betting_prop_dba, matched_bet_dba, pending_bet_dba, game_dba,
                            dprop_dba, bet_uuid_hist_dba);

    service.cancel_bets(uuid_type{ 3 }, fc::time_point_sec(20));

    BOOST_CHECK(matched_bet_dba.is_empty());
    BOOST_REQUIRE_EQUAL(1u, boost::size(pending_bet_dba.get_all_by<by_id>()));
    BOOST_CHECK_EQUAL(15u, pending_bet_dba.get().data.stake.amount);
    BOOST_CHECK_EQUAL(20u, account_svc.get_account("better2").balance.amount);
}

SCORUM_TEST_CASE(cancel_bets_1_bet_recovered_by_restoring_non_existing_pending_bet_check_stakes)
{
    dprop_dba.create([](dynamic_global_property_object&) {});
    account_dba.create([](account_object& o) { o.name = "better1"; });
    account_dba.create([](account_object& o) { o.name = "better2"; });
    matched_bet_dba.create([](matched_bet_object& o) {
        o.game_uuid = { 3 };
        o.created = fc::time_point_sec(42);
        o.bet1_data.uuid = { 1 };
        o.bet1_data.better = "better1";
        o.bet1_data.created = fc::time_point_sec(7); // created before start_time
        o.bet1_data.stake = ASSET_SCR(10); // this stake should be restored as new pending bet with same uuid
        o.bet2_data.uuid = { 2 };
        o.bet2_data.better = "better2";
        o.bet2_data.created = fc::time_point_sec(42); // created after start_time
        o.bet2_data.stake = ASSET_SCR(20); // this stake should be cancelled and returned to owner
    });

    mocks.OnCall(vop_emitter, database_virtual_operations_emmiter_i::push_virtual_operation);

    betting_service service(account_svc, *vop_emitter, betting_prop_dba, matched_bet_dba, pending_bet_dba, game_dba,
                            dprop_dba, bet_uuid_hist_dba);

    service.cancel_bets(uuid_type{ 3 }, fc::time_point_sec(20));

    BOOST_CHECK(matched_bet_dba.is_empty());
    BOOST_REQUIRE_EQUAL(1u, boost::size(pending_bet_dba.get_all_by<by_id>()));
    BOOST_CHECK_EQUAL(10u, pending_bet_dba.get().data.stake.amount);
    BOOST_CHECK_EQUAL(20u, account_svc.get_account("better2").balance.amount);
}

SCORUM_TEST_CASE(cancel_bets_1_bet_recovered_by_increasing_existing_pending_bet_check_virt_ops)
{
    dprop_dba.create([](dynamic_global_property_object&) {});
    account_dba.create([](account_object& o) { o.name = "better1"; });
    account_dba.create([](account_object& o) { o.name = "better2"; });
    matched_bet_dba.create([](matched_bet_object& o) {
        o.game_uuid = { 3 };
        o.created = fc::time_point_sec(42);
        o.bet1_data.uuid = { 1 };
        o.bet1_data.better = "better1";
        o.bet1_data.created = fc::time_point_sec(7); // created before start_time
        o.bet1_data.stake = ASSET_SCR(10); // this stake should be returned to the pending bet rest
        o.bet2_data.uuid = { 2 };
        o.bet2_data.better = "better2";
        o.bet2_data.created = fc::time_point_sec(42); // created after start_time
        o.bet2_data.stake = ASSET_SCR(20); // this stake should be cancelled and returned to owner
    });

    pending_bet_dba.create([](pending_bet_object& o) {
        o.game_uuid = { 3 };
        o.data.uuid = { 1 };
        o.data.created = fc::time_point_sec(7);
        o.data.stake = ASSET_SCR(5); // should be updated
    });

    std::vector<operation> ops;
    mocks.OnCall(vop_emitter, database_virtual_operations_emmiter_i::push_virtual_operation)
        .Do([&](const operation& op) { ops.push_back(op); });

    betting_service service(account_svc, *vop_emitter, betting_prop_dba, matched_bet_dba, pending_bet_dba, game_dba,
                            dprop_dba, bet_uuid_hist_dba);

    service.cancel_bets(uuid_type{ 3 }, fc::time_point_sec(20));

    BOOST_REQUIRE_EQUAL(2u, ops.size());
    BOOST_CHECK(ops[0].which() == operation::tag<bet_updated_operation>::value);
    BOOST_CHECK(ops[1].which() == operation::tag<bet_cancelled_operation>::value);
}

SCORUM_TEST_CASE(cancel_bets_1_bet_recovered_by_restoring_non_existing_pending_bet_check_virt_ops)
{
    dprop_dba.create([](dynamic_global_property_object&) {});
    account_dba.create([](account_object& o) { o.name = "better1"; });
    account_dba.create([](account_object& o) { o.name = "better2"; });
    matched_bet_dba.create([](matched_bet_object& o) {
        o.game_uuid = { 3 };
        o.created = fc::time_point_sec(42);
        o.bet1_data.uuid = { 1 };
        o.bet1_data.better = "better1";
        o.bet1_data.created = fc::time_point_sec(7); // created before start_time
        o.bet1_data.stake = ASSET_SCR(10); // this stake should be restored as new pending bet with same uuid
        o.bet2_data.uuid = { 2 };
        o.bet2_data.better = "better2";
        o.bet2_data.created = fc::time_point_sec(42); // created after start_time
        o.bet2_data.stake = ASSET_SCR(20); // this stake should be cancelled and returned to owner
    });

    std::vector<operation> ops;
    mocks.OnCall(vop_emitter, database_virtual_operations_emmiter_i::push_virtual_operation)
        .Do([&](const operation& op) { ops.push_back(op); });

    betting_service service(account_svc, *vop_emitter, betting_prop_dba, matched_bet_dba, pending_bet_dba, game_dba,
                            dprop_dba, bet_uuid_hist_dba);

    service.cancel_bets(uuid_type{ 3 }, fc::time_point_sec(20));

    BOOST_REQUIRE_EQUAL(2u, ops.size());
    BOOST_CHECK(ops[0].which() == operation::tag<bet_restored_operation>::value);
    BOOST_CHECK(ops[1].which() == operation::tag<bet_cancelled_operation>::value);
}

BOOST_AUTO_TEST_SUITE_END()
}
