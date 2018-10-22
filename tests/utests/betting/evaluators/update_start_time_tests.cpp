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
    using cancel_bets = void (betting_service_i::*)(game_id_type, fc::time_point_sec);
    using game_update_ptr = void (game_service_i::*)(const game_object&, const game_service_i::modifier_type&);

    MockRepository mocks;

    data_service_factory_i* dbs_services = mocks.Mock<data_service_factory_i>();
    betting_service_i* betting_service = mocks.Mock<betting_service_i>();
    betting_property_service_i* betting_prop_service = mocks.Mock<betting_property_service_i>();
    dynamic_global_property_service_i* dynprop_service = mocks.Mock<dynamic_global_property_service_i>();
    account_service_i* account_service = mocks.Mock<account_service_i>();
    game_service_i* game_service = mocks.Mock<game_service_i>();
    database_virtual_operations_emmiter_i* virt_op_emitter = mocks.Mock<database_virtual_operations_emmiter_i>();

    game_evaluator_fixture()
    {
        mocks.OnCall(dbs_services, data_service_factory_i::betting_property_service).ReturnByRef(*betting_prop_service);
        mocks.OnCall(dbs_services, data_service_factory_i::account_service).ReturnByRef(*account_service);
        mocks.OnCall(dbs_services, data_service_factory_i::dynamic_global_property_service)
            .ReturnByRef(*dynprop_service);
        mocks.OnCall(dbs_services, data_service_factory_i::game_service).ReturnByRef(*game_service);
        mocks.OnCall(virt_op_emitter, database_virtual_operations_emmiter_i::push_virtual_operation);
    }
};

BOOST_FIXTURE_TEST_SUITE(update_game_start_time_evaluator_tests, game_evaluator_fixture)

SCORUM_TEST_CASE(update_invalid_time_throw)
{
    update_game_start_time_evaluator ev(*dbs_services, *betting_service, *virt_op_emitter);

    update_game_start_time_operation op;
    op.start_time = fc::time_point_sec(0);

    mocks.OnCall(dynprop_service, dynamic_global_property_service_i::head_block_time).Return(fc::time_point_sec(0));

    BOOST_REQUIRE_THROW(ev.do_apply(op), fc::assert_exception);
}

SCORUM_TEST_CASE(update_by_no_moderator_throw)
{
    update_game_start_time_evaluator ev(*dbs_services, *betting_service, *virt_op_emitter);

    update_game_start_time_operation op;
    op.start_time = fc::time_point_sec(1);

    mocks.OnCall(dynprop_service, dynamic_global_property_service_i::head_block_time).Return(fc::time_point_sec(0));
    mocks.OnCallOverload(account_service, (check_account_existence_ptr)&account_service_i::check_account_existence);
    mocks.OnCall(betting_service, betting_service_i::is_betting_moderator).Return(false);

    BOOST_REQUIRE_THROW(ev.do_apply(op), fc::assert_exception);
}

SCORUM_TEST_CASE(cannot_find_game_throw)
{
    update_game_start_time_evaluator ev(*dbs_services, *betting_service, *virt_op_emitter);

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
    update_game_start_time_evaluator ev(*dbs_services, *betting_service, *virt_op_emitter);

    update_game_start_time_operation op;
    op.start_time = fc::time_point_sec(1);

    auto game_obj = create_object<game_object>(shm, [](game_object& o) { o.status = game_status::started; });

    mocks.OnCall(dynprop_service, dynamic_global_property_service_i::head_block_time).Return(fc::time_point_sec(0));
    mocks.OnCallOverload(account_service, (check_account_existence_ptr)&account_service_i::check_account_existence);
    mocks.OnCall(betting_service, betting_service_i::is_betting_moderator).Return(true);
    mocks.OnCallOverload(game_service, (exists_by_id_ptr)&game_service_i::is_exists).Return(true);
    mocks.OnCallOverload(game_service, (get_by_id_ptr)&game_service_i::get_game).ReturnByRef(game_obj);
    mocks.ExpectCallOverload(betting_service, (cancel_bets)&betting_service_i::cancel_bets);
    mocks.ExpectCallOverload(game_service, (game_update_ptr)&game_service_i::update);

    BOOST_REQUIRE_NO_THROW(ev.do_apply(op));
}

SCORUM_TEST_CASE(should_update_start_time)
{
    update_game_start_time_evaluator ev(*dbs_services, *betting_service, *virt_op_emitter);

    update_game_start_time_operation op;
    op.start_time = fc::time_point_sec(1);

    auto game_obj = create_object<game_object>(shm, [](game_object& o) { o.status = game_status::created; });

    mocks.OnCall(dynprop_service, dynamic_global_property_service_i::head_block_time).Return(fc::time_point_sec(0));
    mocks.OnCallOverload(account_service, (check_account_existence_ptr)&account_service_i::check_account_existence);
    mocks.OnCall(betting_service, betting_service_i::is_betting_moderator).Return(true);
    mocks.OnCallOverload(game_service, (exists_by_id_ptr)&game_service_i::is_exists).Return(true);
    mocks.OnCallOverload(game_service, (get_by_id_ptr)&game_service_i::get_game).ReturnByRef(game_obj);
    mocks.OnCallOverload(betting_service, (cancel_bets)&betting_service_i::cancel_bets);

    mocks.ExpectCallOverload(game_service, (game_update_ptr)&game_service_i::update)
        .Do([](const game_object& obj, const game_service_i::modifier_type& mod) -> void {
            BOOST_CHECK_EQUAL(obj.start_time.sec_since_epoch(), 0u); // before modification
            mod(const_cast<game_object&>(obj));
            BOOST_CHECK_EQUAL(obj.start_time.sec_since_epoch(), 1u); // after modification
        });

    BOOST_REQUIRE_NO_THROW(ev.do_apply(op));
}

SCORUM_TEST_CASE(should_raise_game_status_changed_virt_op)
{
    update_game_start_time_evaluator ev(*dbs_services, *betting_service, *virt_op_emitter);

    update_game_start_time_operation op;
    op.start_time = fc::time_point_sec(1);

    auto game_obj = create_object<game_object>(
        shm, [](game_object& o) { o.status = game_status::started; }); // game was already started

    mocks.OnCall(dynprop_service, dynamic_global_property_service_i::head_block_time).Return(fc::time_point_sec(0));
    mocks.OnCallOverload(account_service, (check_account_existence_ptr)&account_service_i::check_account_existence);
    mocks.OnCall(betting_service, betting_service_i::is_betting_moderator).Return(true);
    mocks.OnCallOverload(game_service, (exists_by_id_ptr)&game_service_i::is_exists).Return(true);
    mocks.OnCallOverload(game_service, (get_by_id_ptr)&game_service_i::get_game).ReturnByRef(game_obj);
    mocks.OnCallOverload(betting_service, (cancel_bets)&betting_service_i::cancel_bets);
    mocks.OnCallOverload(game_service, (game_update_ptr)&game_service_i::update);

    mocks.ExpectCall(virt_op_emitter, database_virtual_operations_emmiter_i::push_virtual_operation)
        .Do([](const operation& op) {
            BOOST_REQUIRE_EQUAL(op.which(), (uint32_t)operation::tag<game_status_changed_operation>::value);
            auto& typed_op = op.get<game_status_changed_operation>();
            BOOST_CHECK(typed_op.old_status == game_status::started);
            BOOST_CHECK(typed_op.new_status == game_status::created);
        });

    BOOST_REQUIRE_NO_THROW(ev.do_apply(op));
}

BOOST_AUTO_TEST_SUITE_END()
}
