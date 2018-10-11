#include <boost/test/unit_test.hpp>

#include "service_wrappers.hpp"

#include <scorum/chain/services/betting_property.hpp>
#include <scorum/chain/services/account.hpp>
#include <scorum/chain/services/game.hpp>

#include <scorum/chain/betting/betting_service.hpp>
#include <scorum/chain/betting/betting_resolver.hpp>

#include <scorum/chain/evaluators/create_game_evaluator.hpp>
#include <scorum/chain/evaluators/cancel_game_evaluator.hpp>
#include <scorum/chain/evaluators/update_game_markets_evaluator.hpp>
#include <scorum/chain/evaluators/update_game_start_time_evaluator.hpp>
#include <scorum/chain/evaluators/post_game_results_evaluator.hpp>

#include <scorum/protocol/betting/market.hpp>

namespace {

using namespace scorum::chain;
using namespace scorum::protocol;

using namespace service_wrappers;

struct game_evaluator_fixture : public shared_memory_fixture
{
    using check_account_existence_ptr
        = void (account_service_i::*)(const account_name_type&, const optional<const char*>&) const;

    using get_by_name_ptr = const game_object& (game_service_i::*)(const std::string&)const;
    using get_by_id_ptr = const game_object& (game_service_i::*)(const scorum::uuid_type&)const;
    using exists_by_name_ptr = bool (game_service_i::*)(const std::string&) const;
    using exists_by_id_ptr = bool (game_service_i::*)(const scorum::uuid_type&) const;
    using create_game_ptr = const game_object& (game_service_i::*)(const scorum::uuid_type&,
                                                                   const account_name_type&,
                                                                   const std::string&,
                                                                   fc::time_point_sec,
                                                                   const game_type&,
                                                                   const fc::flat_set<market_type>&);

    MockRepository mocks;

    data_service_factory_i* dbs_services = mocks.Mock<data_service_factory_i>();
    betting_service_i* betting_service = mocks.Mock<betting_service_i>();
    betting_resolver_i* betting_resolver = mocks.Mock<betting_resolver_i>();
    betting_property_service_i* betting_prop_service = mocks.Mock<betting_property_service_i>();
    dynamic_global_property_service_i* dynprop_service = mocks.Mock<dynamic_global_property_service_i>();
    account_service_i* account_service = mocks.Mock<account_service_i>();
    game_service_i* game_service = mocks.Mock<game_service_i>();

    game_evaluator_fixture()
    {
        mocks.OnCall(dbs_services, data_service_factory_i::betting_property_service).ReturnByRef(*betting_prop_service);
        mocks.OnCall(dbs_services, data_service_factory_i::account_service).ReturnByRef(*account_service);
        mocks.OnCall(dbs_services, data_service_factory_i::dynamic_global_property_service)
            .ReturnByRef(*dynprop_service);
        mocks.OnCall(dbs_services, data_service_factory_i::game_service).ReturnByRef(*game_service);
    }
};

BOOST_FIXTURE_TEST_SUITE(create_game_evaluator_tests, game_evaluator_fixture)

SCORUM_TEST_CASE(create_invalid_start_time_throw)
{
    create_game_evaluator ev(*dbs_services, *betting_service);

    create_game_operation op;
    op.start_time = fc::time_point_sec(0);

    mocks.OnCall(dynprop_service, dynamic_global_property_service_i::head_block_time).Return(fc::time_point_sec(0));

    BOOST_REQUIRE_THROW(ev.do_apply(op), fc::assert_exception);
}

SCORUM_TEST_CASE(create_with_already_existing_name_throw)
{
    create_game_evaluator ev(*dbs_services, *betting_service);

    create_game_operation op;
    op.start_time = fc::time_point_sec(0);

    auto game_obj = create_object<game_object>(shm, [](game_object& o) {});

    mocks.OnCall(dynprop_service, dynamic_global_property_service_i::head_block_time).Return(fc::time_point_sec(1));
    mocks.OnCallOverload(game_service, (exists_by_name_ptr)&game_service_i::is_exists).Return(false);

    BOOST_REQUIRE_THROW(ev.do_apply(op), fc::assert_exception);
}

SCORUM_TEST_CASE(create_by_no_moderator_throw)
{
    create_game_evaluator ev(*dbs_services, *betting_service);

    create_game_operation op;
    op.start_time = fc::time_point_sec(1);

    mocks.OnCall(dynprop_service, dynamic_global_property_service_i::head_block_time).Return(fc::time_point_sec(0));
    mocks.OnCallOverload(game_service, (exists_by_name_ptr)&game_service_i::is_exists).Return(false);
    mocks.OnCallOverload(account_service, (check_account_existence_ptr)&account_service_i::check_account_existence);
    mocks.OnCall(betting_service, betting_service_i::is_betting_moderator).Return(false);

    BOOST_REQUIRE_THROW(ev.do_apply(op), fc::assert_exception);
}

SCORUM_TEST_CASE(game_should_be_created)
{
    create_game_evaluator ev(*dbs_services, *betting_service);

    create_game_operation op;
    op.name = "game";
    op.moderator = "cartman";
    op.game = soccer_game{};
    op.start_time = fc::time_point_sec(1);
    op.auto_resolve_delay_sec = 42;

    mocks.OnCall(dynprop_service, dynamic_global_property_service_i::head_block_time).Return(fc::time_point_sec(0));
    mocks.OnCallOverload(game_service, (exists_by_name_ptr)&game_service_i::is_exists).Return(false);
    mocks.OnCallOverload(game_service, (exists_by_id_ptr)&game_service_i::is_exists).Return(false);
    mocks.OnCallOverload(account_service, (check_account_existence_ptr)&account_service_i::check_account_existence);
    mocks.OnCall(betting_service, betting_service_i::is_betting_moderator).Return(true);

    auto game_obj = create_object<game_object>(shm, [](game_object& o) {});

    mocks.ExpectCallOverload(game_service, (create_game_ptr)&game_service_i::create_game)
        .With(_, "cartman", "game", _, _, _)
        .ReturnByRef(game_obj);

    BOOST_REQUIRE_NO_THROW(ev.do_apply(op));
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_FIXTURE_TEST_SUITE(cancel_game_evaluator_tests, game_evaluator_fixture)

SCORUM_TEST_CASE(cancel_by_no_moderator_throw)
{
    cancel_game_evaluator ev(*dbs_services, *betting_service);

    cancel_game_operation op;

    mocks.OnCallOverload(account_service, (check_account_existence_ptr)&account_service_i::check_account_existence);
    mocks.OnCall(betting_service, betting_service_i::is_betting_moderator).Return(false);

    BOOST_REQUIRE_THROW(ev.do_apply(op), fc::assert_exception);
}

SCORUM_TEST_CASE(cancel_after_game_finished_throw)
{
    cancel_game_evaluator ev(*dbs_services, *betting_service);

    cancel_game_operation op;

    auto game_obj = create_object<game_object>(shm, [](game_object& o) { o.status = game_status::finished; });

    mocks.OnCallOverload(account_service, (check_account_existence_ptr)&account_service_i::check_account_existence);
    mocks.OnCallOverload(game_service, (exists_by_id_ptr)&game_service_i::is_exists).Return(true);
    mocks.OnCallOverload(game_service, (get_by_id_ptr)&game_service_i::get_game).ReturnByRef(game_obj);
    mocks.OnCall(betting_service, betting_service_i::is_betting_moderator).Return(true);

    BOOST_REQUIRE_THROW(ev.do_apply(op), fc::assert_exception);
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_FIXTURE_TEST_SUITE(update_game_markets_evaluator_tests, game_evaluator_fixture)

SCORUM_TEST_CASE(update_by_no_moderator_throw)
{
    update_game_markets_evaluator ev(*dbs_services, *betting_service);

    update_game_markets_operation op;

    mocks.OnCallOverload(account_service, (check_account_existence_ptr)&account_service_i::check_account_existence);
    mocks.OnCall(betting_service, betting_service_i::is_betting_moderator).Return(false);

    BOOST_REQUIRE_THROW(ev.do_apply(op), fc::assert_exception);
}

SCORUM_TEST_CASE(update_after_game_finished_throw)
{
    update_game_markets_evaluator ev(*dbs_services, *betting_service);

    update_game_markets_operation op;

    auto game_obj = create_object<game_object>(shm, [](game_object& o) { o.status = game_status::finished; });

    mocks.OnCallOverload(account_service, (check_account_existence_ptr)&account_service_i::check_account_existence);
    mocks.OnCallOverload(game_service, (exists_by_id_ptr)&game_service_i::is_exists).Return(true);
    mocks.OnCallOverload(game_service, (get_by_id_ptr)&game_service_i::get_game).ReturnByRef(game_obj);
    mocks.OnCall(betting_service, betting_service_i::is_betting_moderator).Return(true);

    BOOST_REQUIRE_THROW(ev.do_apply(op), fc::assert_exception);
}

SCORUM_TEST_CASE(update_invalid_markets_throw)
{
    update_game_markets_evaluator ev(*dbs_services, *betting_service);

    update_game_markets_operation op;
    op.markets = { total_goals_home{} }; // hockey game doesn't have 'total_goals' market (yet!)

    auto game_obj = create_object<game_object>(shm, [](game_object& o) {
        o.game = hockey_game{};
        o.status = game_status::finished;
    });

    mocks.OnCallOverload(account_service, (check_account_existence_ptr)&account_service_i::check_account_existence);
    mocks.OnCallOverload(game_service, (exists_by_id_ptr)&game_service_i::is_exists).Return(true);
    mocks.OnCallOverload(game_service, (get_by_id_ptr)&game_service_i::get_game).ReturnByRef(game_obj);
    mocks.OnCall(betting_service, betting_service_i::is_betting_moderator).Return(true);

    BOOST_REQUIRE_THROW(ev.do_apply(op), fc::assert_exception);
}

SCORUM_TEST_CASE(update_game_new_markets_is_overset_no_cancelled_bets)
{
    update_game_markets_evaluator ev(*dbs_services, *betting_service);

    update_game_markets_operation op;
    op.markets = { total{ 1000 }, total{ 0 }, total{ 500 }, result_home{} };

    auto game_obj = create_object<game_object>(shm, [](game_object& o) {
        o.status = game_status::started;
        o.markets = { total{ 1000 }, total{ 0 } };
    });

    mocks.OnCallOverload(account_service, (check_account_existence_ptr)&account_service_i::check_account_existence);
    mocks.OnCallOverload(game_service, (exists_by_id_ptr)&game_service_i::is_exists).Return(true);
    mocks.OnCallOverload(game_service, (get_by_id_ptr)&game_service_i::get_game).ReturnByRef(game_obj);
    mocks.OnCall(betting_service, betting_service_i::is_betting_moderator).Return(true);
    mocks
        .ExpectCallOverload(betting_service,
                            (void (betting_service_i::*)(game_id_type, const fc::flat_set<market_type>&))
                                & betting_service_i::cancel_bets)
        .Do([](const game_id_type&, const fc::flat_set<market_type>& cancelled_markets) {
            BOOST_CHECK_EQUAL(cancelled_markets.size(), 0u);
        });
    mocks.OnCall(game_service, game_service_i::update_markets);

    BOOST_REQUIRE_NO_THROW(ev.do_apply(op));
}

SCORUM_TEST_CASE(update_game_new_markets_is_subset_some_bets_cancelled)
{
    update_game_markets_evaluator ev(*dbs_services, *betting_service);

    update_game_markets_operation op;
    op.markets = { total{ 1000 }, result_home{} };

    auto game_obj = create_object<game_object>(shm, [](game_object& o) {
        o.status = game_status::created;
        o.markets = { total{ 1000 }, total{ 0 }, total{ 500 }, result_home{} };
    });

    mocks.OnCallOverload(account_service, (check_account_existence_ptr)&account_service_i::check_account_existence);
    mocks.OnCallOverload(game_service, (exists_by_id_ptr)&game_service_i::is_exists).Return(true);
    mocks.OnCallOverload(game_service, (get_by_id_ptr)&game_service_i::get_game).ReturnByRef(game_obj);
    mocks.OnCall(betting_service, betting_service_i::is_betting_moderator).Return(true);
    mocks
        .ExpectCallOverload(betting_service,
                            (void (betting_service_i::*)(game_id_type, const fc::flat_set<market_type>&))
                                & betting_service_i::cancel_bets)
        .Do([](const game_id_type&, const fc::flat_set<market_type>& cancelled_markets) {
            BOOST_REQUIRE_EQUAL(cancelled_markets.size(), 2u);
            auto cancelled_wincases_0 = create_wincases(*cancelled_markets.nth(0));
            auto cancelled_wincases_1 = create_wincases(*cancelled_markets.nth(1));
            BOOST_CHECK_EQUAL(cancelled_wincases_0.first.get<total::over>().threshold, 0u);
            BOOST_CHECK_EQUAL(cancelled_wincases_0.second.get<total::under>().threshold, 0u);
            BOOST_CHECK_EQUAL(cancelled_wincases_1.first.get<total::over>().threshold, 500u);
            BOOST_CHECK_EQUAL(cancelled_wincases_1.second.get<total::under>().threshold, 500u);
        });
    mocks.OnCall(game_service, game_service_i::update_markets);

    BOOST_REQUIRE_NO_THROW(ev.do_apply(op));
}

SCORUM_TEST_CASE(update_game_new_markets_overlap_old_ones_some_bets_cancelled)
{
    update_game_markets_evaluator ev(*dbs_services, *betting_service);

    update_game_markets_operation op;
    op.markets = { total{ 1000 }, total{ 0 }, correct_score_home{} /* this one is new */ };

    auto game_obj = create_object<game_object>(shm, [](game_object& o) {
        o.status = game_status::created;
        o.markets = { total{ 1000 }, total{ 0 }, total{ 500 }, /* should be returned */
                      result_home{} /* should be returned */ };
    });

    mocks.OnCallOverload(account_service, (check_account_existence_ptr)&account_service_i::check_account_existence);
    mocks.OnCallOverload(game_service, (exists_by_id_ptr)&game_service_i::is_exists).Return(true);
    mocks.OnCallOverload(game_service, (get_by_id_ptr)&game_service_i::get_game).ReturnByRef(game_obj);
    mocks.OnCall(betting_service, betting_service_i::is_betting_moderator).Return(true);
    mocks
        .ExpectCallOverload(betting_service,
                            (void (betting_service_i::*)(game_id_type, const fc::flat_set<market_type>&))
                                & betting_service_i::cancel_bets)
        .Do([](const game_id_type&, const fc::flat_set<market_type>& cancelled_markets) {
            BOOST_REQUIRE_EQUAL(cancelled_markets.size(), 2u);
            auto cancelled_wincases_0 = create_wincases(*cancelled_markets.nth(0));
            auto cancelled_wincases_1 = create_wincases(*cancelled_markets.nth(1));
            BOOST_CHECK_NO_THROW(cancelled_wincases_0.first.get<result_home::yes>());
            BOOST_CHECK_NO_THROW(cancelled_wincases_0.second.get<result_home::no>());
            BOOST_CHECK_EQUAL(cancelled_wincases_1.first.get<total::over>().threshold, 500u);
            BOOST_CHECK_EQUAL(cancelled_wincases_1.second.get<total::under>().threshold, 500u);
        });
    mocks.OnCall(game_service, game_service_i::update_markets);

    BOOST_REQUIRE_NO_THROW(ev.do_apply(op));
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_FIXTURE_TEST_SUITE(update_game_start_time_evaluator_tests, game_evaluator_fixture)

SCORUM_TEST_CASE(update_invalid_time_throw)
{
    update_game_start_time_evaluator ev(*dbs_services, *betting_service);

    update_game_start_time_operation op;
    op.start_time = fc::time_point_sec(0);

    mocks.OnCall(dynprop_service, dynamic_global_property_service_i::head_block_time).Return(fc::time_point_sec(0));

    BOOST_REQUIRE_THROW(ev.do_apply(op), fc::assert_exception);
}

SCORUM_TEST_CASE(update_by_no_moderator_throw)
{
    update_game_start_time_evaluator ev(*dbs_services, *betting_service);

    update_game_start_time_operation op;
    op.start_time = fc::time_point_sec(1);

    mocks.OnCall(dynprop_service, dynamic_global_property_service_i::head_block_time).Return(fc::time_point_sec(0));
    mocks.OnCallOverload(account_service, (check_account_existence_ptr)&account_service_i::check_account_existence);
    mocks.OnCall(betting_service, betting_service_i::is_betting_moderator).Return(false);

    BOOST_REQUIRE_THROW(ev.do_apply(op), fc::assert_exception);
}

SCORUM_TEST_CASE(cannot_find_game_throw)
{
    update_game_start_time_evaluator ev(*dbs_services, *betting_service);

    update_game_start_time_operation op;
    op.start_time = fc::time_point_sec(1);

    mocks.OnCall(dynprop_service, dynamic_global_property_service_i::head_block_time).Return(fc::time_point_sec(0));
    mocks.OnCallOverload(account_service, (check_account_existence_ptr)&account_service_i::check_account_existence);
    mocks.OnCall(betting_service, betting_service_i::is_betting_moderator).Return(true);
    mocks.OnCallOverload(game_service, (exists_by_id_ptr)&game_service_i::is_exists).Return(false);

    BOOST_REQUIRE_THROW(ev.do_apply(op), fc::assert_exception);
}

SCORUM_TEST_CASE(after_game_started_shouldnt_throw)
{
    update_game_start_time_evaluator ev(*dbs_services, *betting_service);

    update_game_start_time_operation op;
    op.start_time = fc::time_point_sec(1);

    auto game_obj = create_object<game_object>(shm, [](game_object& o) { o.status = game_status::started; });

    mocks.OnCall(dynprop_service, dynamic_global_property_service_i::head_block_time).Return(fc::time_point_sec(0));
    mocks.OnCallOverload(account_service, (check_account_existence_ptr)&account_service_i::check_account_existence);
    mocks.OnCall(betting_service, betting_service_i::is_betting_moderator).Return(true);
    mocks.OnCallOverload(game_service, (exists_by_id_ptr)&game_service_i::is_exists).Return(true);
    mocks.OnCallOverload(game_service, (get_by_id_ptr)&game_service_i::get_game).ReturnByRef(game_obj);
    mocks.ExpectCallOverload(betting_service,
                             (void (betting_service_i::*)(game_id_type, fc::time_point_sec))
                                 & betting_service_i::cancel_bets);
    mocks.ExpectCallOverload(game_service,
                             (void (game_service_i::*)(const game_object&, const game_service_i::modifier_type&))
                                 & game_service_i::update);

    BOOST_REQUIRE_NO_THROW(ev.do_apply(op));
}

SCORUM_TEST_CASE(expected_time_update)
{
    update_game_start_time_evaluator ev(*dbs_services, *betting_service);

    update_game_start_time_operation op;
    op.start_time = fc::time_point_sec(1);

    auto game_obj = create_object<game_object>(shm, [](game_object& o) { o.status = game_status::created; });

    mocks.OnCall(dynprop_service, dynamic_global_property_service_i::head_block_time).Return(fc::time_point_sec(0));
    mocks.OnCallOverload(account_service, (check_account_existence_ptr)&account_service_i::check_account_existence);
    mocks.OnCall(betting_service, betting_service_i::is_betting_moderator).Return(true);
    mocks.OnCallOverload(game_service, (exists_by_id_ptr)&game_service_i::is_exists).Return(true);
    mocks.OnCallOverload(game_service, (get_by_id_ptr)&game_service_i::get_game).ReturnByRef(game_obj);
    mocks.OnCallOverload(betting_service,
                         (void (betting_service_i::*)(game_id_type, fc::time_point_sec))
                             & betting_service_i::cancel_bets);

    mocks
        .OnCallOverload(game_service,
                        (void (game_service_i::*)(const game_object&, const game_service_i::modifier_type&))
                            & game_service_i::update)
        .Do([](const game_object& obj, const game_service_i::modifier_type& mod) -> void {
            BOOST_CHECK_EQUAL(obj.start_time.sec_since_epoch(), 0u);
            mod(const_cast<game_object&>(obj));
            BOOST_CHECK_EQUAL(obj.start_time.sec_since_epoch(), 1u);
        });

    BOOST_REQUIRE_NO_THROW(ev.do_apply(op));
}

BOOST_AUTO_TEST_SUITE_END()
}
