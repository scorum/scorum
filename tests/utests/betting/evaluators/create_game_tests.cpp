#include <boost/test/unit_test.hpp>

#include "service_wrappers.hpp"

#include <scorum/chain/dba/db_accessor.hpp>
#include <scorum/chain/services/betting_property.hpp>
#include <scorum/chain/services/account.hpp>
#include <scorum/chain/services/game.hpp>

#include <scorum/chain/betting/betting_service.hpp>

#include <scorum/chain/evaluators/create_game_evaluator.hpp>

#include <scorum/protocol/betting/market.hpp>

#include <scorum/chain/schema/game_object.hpp>

namespace {

using namespace scorum;
using namespace scorum::chain;
using namespace scorum::protocol;

using namespace service_wrappers;

struct game_evaluator_fixture : public shared_memory_fixture
{
    using check_account_existence_ptr
        = void (account_service_i::*)(const account_name_type&, const optional<const char*>&) const;

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
    dynamic_global_property_service_i* dynprop_service = mocks.Mock<dynamic_global_property_service_i>();
    account_service_i* account_service = mocks.Mock<account_service_i>();
    game_service_i* game_service = mocks.Mock<game_service_i>();
    database_virtual_operations_emmiter_i* virt_op_emitter = mocks.Mock<database_virtual_operations_emmiter_i>();
    dba::db_accessor<game_uuid_history_object> uuid_hist_dba;

    game_evaluator_fixture()
        : uuid_hist_dba(*mocks.Mock<dba::db_index>())
    {
        mocks.OnCall(dbs_services, data_service_factory_i::account_service).ReturnByRef(*account_service);
        mocks.OnCall(dbs_services, data_service_factory_i::dynamic_global_property_service)
            .ReturnByRef(*dynprop_service);
        mocks.OnCall(dbs_services, data_service_factory_i::game_service).ReturnByRef(*game_service);
        mocks.OnCall(virt_op_emitter, database_virtual_operations_emmiter_i::push_virtual_operation);
    }
};

BOOST_FIXTURE_TEST_SUITE(create_game_evaluator_tests, game_evaluator_fixture)

SCORUM_TEST_CASE(create_invalid_start_time_throw)
{
    create_game_evaluator ev(*dbs_services, *betting_service, uuid_hist_dba);

    create_game_operation op;
    op.start_time = fc::time_point_sec(0);

    mocks.OnCall(dynprop_service, dynamic_global_property_service_i::head_block_time).Return(fc::time_point_sec(0));

    BOOST_REQUIRE_THROW(ev.do_apply(op), fc::assert_exception);
}

SCORUM_TEST_CASE(create_with_already_existing_uuid_throw)
{
    create_game_evaluator ev(*dbs_services, *betting_service, uuid_hist_dba);

    create_game_operation op;
    op.start_time = fc::time_point_sec(2);

    auto game_obj = create_object<game_object>(shm, [](game_object& o) {});

    mocks.OnCall(dynprop_service, dynamic_global_property_service_i::head_block_time).Return(fc::time_point_sec(1));
    mocks.OnCallOverload(game_service, (exists_by_name_ptr)&game_service_i::is_exists).Return(false);
    mocks.ExpectCallFunc((dba::detail::is_exists_by<game_uuid_history_object, by_uuid, uuid_type>)).Return(true);

    BOOST_REQUIRE_THROW(ev.do_apply(op), fc::assert_exception);
}

SCORUM_TEST_CASE(create_by_no_moderator_throw)
{
    create_game_evaluator ev(*dbs_services, *betting_service, uuid_hist_dba);

    create_game_operation op;
    op.start_time = fc::time_point_sec(1);

    mocks.OnCall(dynprop_service, dynamic_global_property_service_i::head_block_time).Return(fc::time_point_sec(0));
    mocks.OnCallOverload(game_service, (exists_by_name_ptr)&game_service_i::is_exists).Return(false);
    mocks.OnCallFunc((dba::detail::is_exists_by<game_uuid_history_object, by_uuid, uuid_type>)).Return(false);
    mocks.OnCallOverload(account_service, (check_account_existence_ptr)&account_service_i::check_account_existence);
    mocks.OnCall(betting_service, betting_service_i::is_betting_moderator).Return(false);

    BOOST_REQUIRE_THROW(ev.do_apply(op), fc::assert_exception);
}

SCORUM_TEST_CASE(game_should_be_created)
{
    create_game_evaluator ev(*dbs_services, *betting_service, uuid_hist_dba);

    create_game_operation op;
    op.name = "game";
    op.moderator = "cartman";
    op.game = soccer_game{};
    op.start_time = fc::time_point_sec(1);
    op.auto_resolve_delay_sec = 42;

    mocks.OnCall(dynprop_service, dynamic_global_property_service_i::head_block_time).Return(fc::time_point_sec(0));
    mocks.OnCallOverload(game_service, (exists_by_name_ptr)&game_service_i::is_exists).Return(false);
    mocks.OnCallFunc((dba::detail::is_exists_by<game_uuid_history_object, by_uuid, uuid_type>)).Return(false);
    mocks.OnCallOverload(account_service, (check_account_existence_ptr)&account_service_i::check_account_existence);
    mocks.OnCall(betting_service, betting_service_i::is_betting_moderator).Return(true);

    auto game_obj = create_object<game_object>(shm, [](game_object& o) {});

    mocks.ExpectCallOverload(game_service, (create_game_ptr)&game_service_i::create_game)
        .With(_, "cartman", "game", _, _, _)
        .ReturnByRef(game_obj);

    BOOST_REQUIRE_NO_THROW(ev.do_apply(op));
}

BOOST_AUTO_TEST_SUITE_END()
}
