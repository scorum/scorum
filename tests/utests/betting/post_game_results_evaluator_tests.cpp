#include <boost/test/unit_test.hpp>

#include "service_wrappers.hpp"

#include <scorum/chain/services/betting_property.hpp>
#include <scorum/chain/services/account.hpp>
#include <scorum/chain/services/game.hpp>

#include <scorum/chain/betting/betting_service.hpp>

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

    using get_by_id_ptr = const game_object& (game_service_i::*)(const scorum::uuid_type&)const;
    using exists_by_id_ptr = bool (game_service_i::*)(const scorum::uuid_type&) const;

    MockRepository mocks;

    data_service_factory_i* dbs_services = mocks.Mock<data_service_factory_i>();
    betting_service_i* betting_service = mocks.Mock<betting_service_i>();
    dynamic_global_property_service_i* dynprop_service = mocks.Mock<dynamic_global_property_service_i>();
    account_service_i* account_service = mocks.Mock<account_service_i>();
    game_service_i* game_service = mocks.Mock<game_service_i>();

    game_evaluator_fixture()
    {
        mocks.OnCall(dbs_services, data_service_factory_i::account_service).ReturnByRef(*account_service);
        mocks.OnCall(dbs_services, data_service_factory_i::dynamic_global_property_service)
            .ReturnByRef(*dynprop_service);
        mocks.OnCall(dbs_services, data_service_factory_i::game_service).ReturnByRef(*game_service);
    }
};

BOOST_FIXTURE_TEST_SUITE(post_game_results_evaluator_tests, game_evaluator_fixture)

SCORUM_TEST_CASE(invalid_moderator_should_throw)
{
    post_game_results_evaluator ev(*dbs_services, *betting_service);

    post_game_results_operation op;

    mocks.ExpectCallOverload(account_service, (check_account_existence_ptr)&account_service_i::check_account_existence);
    mocks.ExpectCall(betting_service, betting_service_i::is_betting_moderator).Return(false);

    BOOST_REQUIRE_THROW(ev.do_apply(op), fc::assert_exception);
}

SCORUM_TEST_CASE(non_existing_game_should_throw)
{
    post_game_results_evaluator ev(*dbs_services, *betting_service);

    post_game_results_operation op;

    mocks.OnCallOverload(account_service, (check_account_existence_ptr)&account_service_i::check_account_existence);
    mocks.OnCall(betting_service, betting_service_i::is_betting_moderator).Return(true);
    mocks.ExpectCallOverload(game_service, (exists_by_id_ptr)&game_service_i::is_exists).Return(false);

    BOOST_REQUIRE_THROW(ev.do_apply(op), fc::assert_exception);
}

SCORUM_TEST_CASE(non_started_game_should_throw)
{
    post_game_results_evaluator ev(*dbs_services, *betting_service);

    post_game_results_operation op;

    auto game_obj = create_object<game_object>(shm, [](game_object& o) { o.status = game_status::created; });

    mocks.OnCallOverload(account_service, (check_account_existence_ptr)&account_service_i::check_account_existence);
    mocks.OnCall(betting_service, betting_service_i::is_betting_moderator).Return(true);
    mocks.OnCallOverload(game_service, (exists_by_id_ptr)&game_service_i::is_exists).Return(true);
    mocks.ExpectCallOverload(game_service, (get_by_id_ptr)&game_service_i::get_game).ReturnByRef(game_obj);

    BOOST_REQUIRE_THROW(ev.do_apply(op), fc::assert_exception);
}

SCORUM_TEST_CASE(post_results_after_bets_resolve_delay_should_throw)
{
    post_game_results_evaluator ev(*dbs_services, *betting_service);

    post_game_results_operation op;

    auto game_obj = create_object<game_object>(shm, [](game_object& o) {
        o.status = game_status::started;
        o.bets_resolve_time = fc::time_point_sec(10);
    });

    mocks.OnCallOverload(account_service, (check_account_existence_ptr)&account_service_i::check_account_existence);
    mocks.OnCall(betting_service, betting_service_i::is_betting_moderator).Return(true);
    mocks.OnCallOverload(game_service, (exists_by_id_ptr)&game_service_i::is_exists).Return(true);
    mocks.ExpectCallOverload(game_service, (get_by_id_ptr)&game_service_i::get_game).ReturnByRef(game_obj);
    mocks.ExpectCall(dynprop_service, dynamic_global_property_service_i::head_block_time)
        .Return(fc::time_point_sec(10));

    BOOST_REQUIRE_THROW(ev.do_apply(op), fc::assert_exception);
}

SCORUM_TEST_CASE(wincases_and_markets_mismatch_should_throw)
{
    using namespace scorum::protocol;

    post_game_results_evaluator ev(*dbs_services, *betting_service);

    post_game_results_operation op;
    op.wincases = { round_home::no{} };

    auto game_obj = create_object<game_object>(shm, [](game_object& o) {
        o.status = game_status::started;
        o.bets_resolve_time = fc::time_point_sec(10);
        o.markets = { result_home{} };
    });

    mocks.OnCallOverload(account_service, (check_account_existence_ptr)&account_service_i::check_account_existence);
    mocks.OnCall(betting_service, betting_service_i::is_betting_moderator).Return(true);
    mocks.OnCallOverload(game_service, (exists_by_id_ptr)&game_service_i::is_exists).Return(true);
    mocks.OnCallOverload(game_service, (get_by_id_ptr)&game_service_i::get_game).ReturnByRef(game_obj);
    mocks.OnCall(dynprop_service, dynamic_global_property_service_i::head_block_time).Return(fc::time_point_sec(9));

    BOOST_REQUIRE_THROW(ev.do_apply(op), fc::assert_exception);
}

SCORUM_TEST_CASE(both_wincases_from_pair_were_provided_should_throw)
{
    using namespace scorum::protocol;

    post_game_results_evaluator ev(*dbs_services, *betting_service);

    post_game_results_operation op;
    op.wincases = { round_home::yes{}, round_home::no{} };

    auto game_obj = create_object<game_object>(shm, [](game_object& o) {
        o.status = game_status::started;
        o.bets_resolve_time = fc::time_point_sec(10);
        o.markets = { round_home{} };
    });

    mocks.OnCallOverload(account_service, (check_account_existence_ptr)&account_service_i::check_account_existence);
    mocks.OnCall(betting_service, betting_service_i::is_betting_moderator).Return(true);
    mocks.OnCallOverload(game_service, (exists_by_id_ptr)&game_service_i::is_exists).Return(true);
    mocks.OnCallOverload(game_service, (get_by_id_ptr)&game_service_i::get_game).ReturnByRef(game_obj);
    mocks.OnCall(dynprop_service, dynamic_global_property_service_i::head_block_time).Return(fc::time_point_sec(9));

    BOOST_REQUIRE_THROW(ev.do_apply(op), fc::assert_exception);
}

SCORUM_TEST_CASE(not_all_winners_were_posted_should_throw)
{
    using namespace scorum::protocol;

    post_game_results_evaluator ev(*dbs_services, *betting_service);

    post_game_results_operation op;
    op.wincases = { total::over{ 500 } };

    auto game_obj = create_object<game_object>(shm, [](game_object& o) {
        o.status = game_status::started;
        o.bets_resolve_time = fc::time_point_sec(10);
        o.markets = { total{ 1500 }, total{ 500 } };
    });

    mocks.OnCallOverload(account_service, (check_account_existence_ptr)&account_service_i::check_account_existence);
    mocks.OnCall(betting_service, betting_service_i::is_betting_moderator).Return(true);
    mocks.OnCallOverload(game_service, (exists_by_id_ptr)&game_service_i::is_exists).Return(true);
    mocks.OnCallOverload(game_service, (get_by_id_ptr)&game_service_i::get_game).ReturnByRef(game_obj);
    mocks.OnCall(dynprop_service, dynamic_global_property_service_i::head_block_time).Return(fc::time_point_sec(9));

    BOOST_REQUIRE_THROW(ev.do_apply(op), fc::assert_exception);
}

SCORUM_TEST_CASE(wrong_coefficient_should_throw)
{
    using namespace scorum::protocol;

    post_game_results_evaluator ev(*dbs_services, *betting_service);

    post_game_results_operation op;
    op.wincases = { total::over{ 500 } };

    auto game_obj = create_object<game_object>(shm, [](game_object& o) {
        o.status = game_status::started;
        o.bets_resolve_time = fc::time_point_sec(10);
        o.markets = { total{ 1500 } };
    });

    mocks.OnCallOverload(account_service, (check_account_existence_ptr)&account_service_i::check_account_existence);
    mocks.OnCall(betting_service, betting_service_i::is_betting_moderator).Return(true);
    mocks.OnCallOverload(game_service, (exists_by_id_ptr)&game_service_i::is_exists).Return(true);
    mocks.OnCallOverload(game_service, (get_by_id_ptr)&game_service_i::get_game).ReturnByRef(game_obj);
    mocks.OnCall(dynprop_service, dynamic_global_property_service_i::head_block_time).Return(fc::time_point_sec(9));

    BOOST_REQUIRE_THROW(ev.do_apply(op), fc::assert_exception);
}

SCORUM_TEST_CASE(winners_with_same_wincase_type_but_diff_coefficients_shouldnt_throw)
{
    using namespace scorum::protocol;

    post_game_results_evaluator ev(*dbs_services, *betting_service);

    post_game_results_operation op;
    op.wincases = { total::over{ 500 }, total::over{ 1500 } };

    auto game_obj = create_object<game_object>(shm, [](game_object& o) {
        o.status = game_status::started;
        o.bets_resolve_time = fc::time_point_sec(10);
        o.markets = { total{ 500 }, total{ 1500 } };
    });

    mocks.OnCallOverload(account_service, (check_account_existence_ptr)&account_service_i::check_account_existence);
    mocks.OnCall(betting_service, betting_service_i::is_betting_moderator).Return(true);
    mocks.OnCallOverload(game_service, (exists_by_id_ptr)&game_service_i::is_exists).Return(true);
    mocks.OnCallOverload(game_service, (get_by_id_ptr)&game_service_i::get_game).ReturnByRef(game_obj);
    mocks.OnCall(dynprop_service, dynamic_global_property_service_i::head_block_time).Return(fc::time_point_sec(9));
    mocks.OnCallOverload(betting_service,
                         (void (betting_service_i::*)(const game_id_type&)) & betting_service_i::cancel_pending_bets);
    mocks.ExpectCall(game_service, game_service_i::finish);

    BOOST_REQUIRE_NO_THROW(ev.do_apply(op));
}

SCORUM_TEST_CASE(three_state_market_no_posted_result_shouldnt_throw)
{
    using namespace scorum::protocol;

    post_game_results_evaluator ev(*dbs_services, *betting_service);

    post_game_results_operation op;
    op.wincases = {};

    auto game_obj = create_object<game_object>(shm, [](game_object& o) {
        o.status = game_status::started;
        o.bets_resolve_time = fc::time_point_sec(10);
        o.markets = { total{ 1000 }, total{ 2000 } };
    });

    mocks.OnCallOverload(account_service, (check_account_existence_ptr)&account_service_i::check_account_existence);
    mocks.OnCall(betting_service, betting_service_i::is_betting_moderator).Return(true);
    mocks.OnCallOverload(game_service, (exists_by_id_ptr)&game_service_i::is_exists).Return(true);
    mocks.OnCallOverload(game_service, (get_by_id_ptr)&game_service_i::get_game).ReturnByRef(game_obj);
    mocks.OnCall(dynprop_service, dynamic_global_property_service_i::head_block_time).Return(fc::time_point_sec(9));
    mocks.OnCallOverload(betting_service,
                         (void (betting_service_i::*)(const game_id_type&)) & betting_service_i::cancel_pending_bets);
    mocks.ExpectCall(game_service, game_service_i::finish);

    BOOST_REQUIRE_NO_THROW(ev.do_apply(op));
}

BOOST_AUTO_TEST_SUITE_END()
}
