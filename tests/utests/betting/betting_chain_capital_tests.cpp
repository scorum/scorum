#include <boost/test/unit_test.hpp>

#include <scorum/chain/schema/betting_property_object.hpp>
#include <scorum/chain/schema/bet_objects.hpp>
#include <scorum/chain/schema/game_object.hpp>
#include <scorum/chain/schema/dynamic_global_property_object.hpp>
#include <scorum/chain/schema/account_objects.hpp>

#include <scorum/chain/betting/betting_service.hpp>
#include <scorum/chain/dba/db_accessor.hpp>
#include <scorum/chain/services/account.hpp>
#include <scorum/protocol/betting/market.hpp>
#include <db_mock.hpp>
#include <hippomocks.h>
#include <defines.hpp>

namespace {
using namespace scorum;
using namespace scorum::chain;
using namespace scorum::protocol;

struct chain_capital_fixture
{
    chain_capital_fixture()
        : betting_prop_dba(db)
        , matched_bet_dba(db)
        , pending_bet_dba(db)
        , game_dba(db)
        , dprop_dba(db)
        , account_dba(db)
    {
        db.add_index<pending_bet_index>();
        db.add_index<account_index>();
        db.add_index<dynamic_global_property_index>();

        mocks.OnCall(dbs_factory, data_service_factory_i::account_service).ReturnByRef(*account_svc);
        mocks.OnCall(vop_emitter, database_virtual_operations_emmiter_i::push_virtual_operation);
    }

    MockRepository mocks;

    data_service_factory_i* dbs_factory = mocks.Mock<data_service_factory_i>();
    account_service_i* account_svc = mocks.Mock<account_service_i>();
    database_virtual_operations_emmiter_i* vop_emitter = mocks.Mock<database_virtual_operations_emmiter_i>();

    db_mock db;
    dba::db_accessor<betting_property_object> betting_prop_dba;
    dba::db_accessor<matched_bet_object> matched_bet_dba;
    dba::db_accessor<pending_bet_object> pending_bet_dba;
    dba::db_accessor<game_object> game_dba;
    dba::db_accessor<dynamic_global_property_object> dprop_dba;
    dba::db_accessor<account_object> account_dba;
};

BOOST_FIXTURE_TEST_SUITE(betting_chain_capital_tests, chain_capital_fixture)

SCORUM_TEST_CASE(pending_bet_creation_should_change_betting_capital)
{
    // clang-format off
    dprop_dba.create([&](auto& o) { o.time = fc::time_point_sec(42); });
    account_dba.create([&](auto& o) { o.name = "alice"; o.balance = ASSET_SCR(1000); });
    // clang-format on
    // TODO: using db_mock in services. in this case we don't need to use mocks
    mocks.OnCall(account_svc, account_service_i::get_account).ReturnByRef(account_dba.get());
    mocks.OnCall(account_svc, account_service_i::decrease_balance);

    betting_service svc(*dbs_factory, *vop_emitter, betting_prop_dba, matched_bet_dba, pending_bet_dba, game_dba,
                        dprop_dba);

    BOOST_REQUIRE_EQUAL(dprop_dba.get().betting_stats.pending_bets_volume.amount, 0u);
    BOOST_REQUIRE_EQUAL(dprop_dba.get().betting_stats.matched_bets_volume.amount, 0u);

    svc.create_pending_bet("alice", ASSET_SCR(1000), odds(10, 2), result_home::yes{}, 0, uuid_type{ 1 },
                           pending_bet_kind::live);

    BOOST_CHECK_EQUAL(dprop_dba.get().betting_stats.pending_bets_volume.amount, 1000u);
    BOOST_CHECK_EQUAL(dprop_dba.get().betting_stats.matched_bets_volume.amount, 0u);
}

SCORUM_TEST_CASE(matched_bet_should_change_betting_capital)
{
    pending_bet_dba.create([](pending_bet_object& o) {
        o.game = 0;
        o.market = result_home{};
        o.data.stake = ASSET_SCR(1000);
        o.data.bet_odds = odds(10, 2);
    });
    // clang-format off

    dprop_dba.create([&](auto& o) { o.time = fc::time_point_sec(42); });
    account_dba.create([&](auto& o) { o.name = "alice"; o.balance = ASSET_SCR(1000); });
    // clang-format on
    // TODO: using db_mock in services. in this case we don't need to use mocks
    mocks.OnCall(account_svc, account_service_i::get_account).ReturnByRef(account_dba.get());
    mocks.OnCall(account_svc, account_service_i::decrease_balance);

    betting_service svc(*dbs_factory, *vop_emitter, betting_prop_dba, matched_bet_dba, pending_bet_dba, game_dba,
                        dprop_dba);

    BOOST_REQUIRE_EQUAL(dprop_dba.get().betting_stats.pending_bets_volume.amount, 0u);
    BOOST_REQUIRE_EQUAL(dprop_dba.get().betting_stats.matched_bets_volume.amount, 0u);

    svc.create_pending_bet("alice", ASSET_SCR(1000), odds(10, 2), result_home::yes{}, 0, uuid_type{ 1 },
                           pending_bet_kind::live);

    BOOST_CHECK_EQUAL(dprop_dba.get().betting_stats.pending_bets_volume.amount, 1000u);
    BOOST_CHECK_EQUAL(dprop_dba.get().betting_stats.matched_bets_volume.amount, 0u);
}
BOOST_AUTO_TEST_SUITE_END()
}
