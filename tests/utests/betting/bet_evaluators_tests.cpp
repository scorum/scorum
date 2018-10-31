#include <boost/test/unit_test.hpp>

#include <scorum/chain/dba/db_accessor.hpp>
#include <scorum/chain/evaluators/post_bet_evalulator.hpp>
#include <scorum/chain/evaluators/cancel_pending_bets_evaluator.hpp>

#include "betting_common.hpp"
#include "object_wrapper.hpp"

#include "detail.hpp"

namespace {

using namespace scorum;
using namespace scorum::chain;
using namespace scorum::protocol;

SCORUM_TEST_CASE(post_bet_evaluator_operation_validate_check)
{
    post_bet_operation test_op;
    test_op.better = "aclice";
    test_op.uuid = gen_uuid("game");
    test_op.wincase = correct_score_home::yes();
    test_op.odds = { 3, 1 };
    test_op.stake = ASSET_SCR(100);

    post_bet_operation op = test_op;

    BOOST_CHECK_NO_THROW(op.validate());

    op.better = "";
    BOOST_CHECK_THROW(op.validate(), fc::assert_exception);
    op = test_op;

    op.odds = { 1, 10 };
    BOOST_CHECK_THROW(op.validate(), fc::assert_exception);
    op = test_op;

    op.stake.amount = 0;
    BOOST_CHECK_THROW(op.validate(), fc::assert_exception);
    op = test_op;

    op.stake = ASSET_SP(1e+9);
    BOOST_CHECK_THROW(op.validate(), fc::assert_exception);
    op = test_op;
}

struct cancel_pending_bets_evaluator_fixture : public shared_memory_fixture
{
    using check_account_existence_ptr
        = void (account_service_i::*)(const account_name_type&, const fc::optional<const char*>&) const;
    using is_exists_ptr = bool (pending_bet_service_i::*)(const uuid_type&) const;
    using get_ptr = const pending_bet_object& (pending_bet_service_i::*)(const uuid_type&)const;

    MockRepository mocks;

    data_service_factory_i* dbs_factory = mocks.Mock<data_service_factory_i>();
    betting_service_i* betting_svc = mocks.Mock<betting_service_i>();
    account_service_i* acc_svc = mocks.Mock<account_service_i>();
    pending_bet_service_i* pending_bet_svc = mocks.Mock<pending_bet_service_i>();

    cancel_pending_bets_evaluator_fixture()
    {
        mocks.OnCall(dbs_factory, data_service_factory_i::account_service).ReturnByRef(*acc_svc);
        mocks.OnCall(dbs_factory, data_service_factory_i::pending_bet_service).ReturnByRef(*pending_bet_svc);
    }
};

BOOST_FIXTURE_TEST_SUITE(cancel_pending_bets_evaluator_tests, cancel_pending_bets_evaluator_fixture)

SCORUM_TEST_CASE(bet_id_existance_check_should_throw)
{
    cancel_pending_bets_operation op;
    op.better = "better";
    op.bet_uuids = { gen_uuid("0") };

    cancel_pending_bets_evaluator ev(*dbs_factory, *betting_svc);

    mocks.ExpectCallOverload(acc_svc, (check_account_existence_ptr)&account_service_i::check_account_existence);
    mocks.ExpectCallOverload(pending_bet_svc, (is_exists_ptr)&pending_bet_service_i::is_exists)
        .With(gen_uuid("0"))
        .Return(false);

    BOOST_CHECK_THROW(ev.do_apply(op), fc::assert_exception);
}

SCORUM_TEST_CASE(better_mismatch_should_throw)
{
    cancel_pending_bets_operation op;
    op.better = "better";
    op.bet_uuids = { gen_uuid("0") };

    auto obj = create_object<pending_bet_object>(shm, [](pending_bet_object& o) { o.data.better = "cartman"; });

    mocks.OnCallOverload(acc_svc, (check_account_existence_ptr)&account_service_i::check_account_existence);
    mocks.OnCallOverload(pending_bet_svc, (is_exists_ptr)&pending_bet_service_i::is_exists)
        .With(gen_uuid("0"))
        .Return(true);
    mocks.ExpectCallOverload(pending_bet_svc, (get_ptr)&pending_bet_service_i::get_pending_bet)
        .With(gen_uuid("0"))
        .ReturnByRef(obj);

    cancel_pending_bets_evaluator ev(*dbs_factory, *betting_svc);
    BOOST_CHECK_THROW(ev.do_apply(op), fc::assert_exception);
}

SCORUM_TEST_CASE(should_cancel_bets)
{
    cancel_pending_bets_operation op;
    op.better = "better";
    op.bet_uuids = { gen_uuid("0"), gen_uuid("1") };

    // clang-format off
    auto obj1 = create_object<pending_bet_object>(shm, [&](pending_bet_object& o){ o.data.better = "better"; o.id = 0; o.data.uuid = gen_uuid("0"); });
    auto obj2 = create_object<pending_bet_object>(shm, [&](pending_bet_object& o){ o.data.better = "better"; o.id = 1; o.data.uuid = gen_uuid("1"); });

    mocks.OnCallOverload(acc_svc, (check_account_existence_ptr)&account_service_i::check_account_existence);
    mocks.OnCallOverload(pending_bet_svc, (is_exists_ptr)&pending_bet_service_i::is_exists).With(gen_uuid("0")).Return(true);
    mocks.OnCallOverload(pending_bet_svc, (is_exists_ptr)&pending_bet_service_i::is_exists).With(gen_uuid("1")).Return(true);
    mocks.OnCallOverload(pending_bet_svc, (get_ptr)&pending_bet_service_i::get_pending_bet).With(gen_uuid("0")).ReturnByRef(obj1);
    mocks.OnCallOverload(pending_bet_svc, (get_ptr)&pending_bet_service_i::get_pending_bet).With(gen_uuid("1")).ReturnByRef(obj2);
    mocks.ExpectCall(betting_svc, betting_service_i::cancel_pending_bet).With(0);
    mocks.ExpectCall(betting_svc, betting_service_i::cancel_pending_bet).With(1);
    // clang-format on

    cancel_pending_bets_evaluator ev(*dbs_factory, *betting_svc);
    BOOST_CHECK_NO_THROW(ev.do_apply(op));
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE(cancel_pending_bets_operation_validate_tests)

SCORUM_TEST_CASE(throw_exception_when_bet_uuids_is_empty)
{
    cancel_pending_bets_operation op;
    op.better = "better";
    op.bet_uuids = {};

    BOOST_CHECK_THROW(op.validate(), fc::assert_exception);
}

SCORUM_TEST_CASE(dont_throw_exception_when_bet_uuids_set)
{
    cancel_pending_bets_operation op;
    op.better = "better";
    op.bet_uuids = { gen_uuid("1") };

    BOOST_CHECK_NO_THROW(op.validate());
}

SCORUM_TEST_CASE(throw_exception_when_better_is_empty)
{
    cancel_pending_bets_operation op;
    op.better = "";
    op.bet_uuids = { gen_uuid("1") };

    BOOST_CHECK_THROW(op.validate(), fc::assert_exception);
}

SCORUM_TEST_CASE(throw_exception_when_bets_not_unique)
{
    cancel_pending_bets_operation op;
    op.better = "better";
    op.bet_uuids = { gen_uuid("1"), gen_uuid("1") };

    BOOST_CHECK_THROW(op.validate(), fc::assert_exception);
}

BOOST_AUTO_TEST_SUITE_END()
}
