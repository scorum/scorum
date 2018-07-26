#include <boost/test/unit_test.hpp>

#include <scorum/chain/data_service_factory.hpp>
#include <scorum/chain/services/account.hpp>
#include <scorum/chain/services/dev_pool.hpp>
#include <scorum/chain/services/dynamic_global_property.hpp>

#include <scorum/chain/evaluators/proposal_evaluators.hpp>

#include <scorum/chain/schema/account_objects.hpp>
#include <scorum/chain/schema/dynamic_global_property_object.hpp>

#include "defines.hpp"

#include "object_wrapper.hpp"

#include <hippomocks.h>

namespace development_committee_transfer_evaluator_tests {

using namespace scorum::chain;
using namespace scorum::protocol;

struct fixture : public shared_memory_fixture
{
    MockRepository mocks;

    data_service_factory_i* services = mocks.Mock<data_service_factory_i>();

    account_service_i* account_service = mocks.Mock<account_service_i>();
    dev_pool_service_i* dev_pool_service = mocks.Mock<dev_pool_service_i>();

    using dgps_t = dynamic_global_property_service_i;

    fixture()
        : shared_memory_fixture()
    {
        mocks.ExpectCall(services, data_service_factory_i::account_service).ReturnByRef(*account_service);
        mocks.ExpectCall(services, data_service_factory_i::dev_pool_service).ReturnByRef(*dev_pool_service);
    }
};

BOOST_FIXTURE_TEST_CASE(throw_when_amount_gt_balance, fixture)
{
    mocks.ExpectCall(dev_pool_service, dev_pool_service_i::get_scr_balace).Return(asset(10, SCORUM_SYMBOL));

    development_committee_transfer_evaluator::operation_type op;
    op.amount = asset(20, SCORUM_SYMBOL);

    development_committee_transfer_evaluator evaluator(*services);

    SCORUM_CHECK_EXCEPTION(evaluator.do_apply(op), fc::assert_exception, "Not enough SCR in dev pool.");
}

BOOST_FIXTURE_TEST_CASE(decrease_dev_pool_scr_balance, fixture)
{
    development_committee_transfer_evaluator::operation_type op;
    op.amount = asset(20, SCORUM_SYMBOL);

    development_committee_transfer_evaluator evaluator(*services);

    mocks.OnCall(dev_pool_service, dev_pool_service_i::get_scr_balace).Return(asset(30, SCORUM_SYMBOL));

    mocks.ExpectCall(dev_pool_service, dev_pool_service_i::decrease_scr_balance).With(op.amount);

    account_object account = create_object<account_object>(shm);

    mocks.OnCall(account_service, account_service_i::get_account).With(_).ReturnByRef(account);
    mocks.OnCall(account_service, account_service_i::increase_balance).With(_, _);

    BOOST_CHECK_NO_THROW(evaluator.do_apply(op));
}

BOOST_FIXTURE_TEST_CASE(increase_account_balance, fixture)
{
    development_committee_transfer_evaluator::operation_type op;
    op.amount = asset(20, SCORUM_SYMBOL);
    op.to_account = "jim";

    development_committee_transfer_evaluator evaluator(*services);

    mocks.OnCall(dev_pool_service, dev_pool_service_i::get_scr_balace).Return(asset(30, SCORUM_SYMBOL));

    mocks.OnCall(dev_pool_service, dev_pool_service_i::decrease_scr_balance).With(_);

    account_object account = create_object<account_object>(shm);

    mocks.ExpectCall(account_service, account_service_i::get_account).With("jim").ReturnByRef(account);
    mocks.ExpectCall(account_service, account_service_i::increase_balance).With(_, op.amount);

    BOOST_CHECK_NO_THROW(evaluator.do_apply(op));
}

} // namespace development_committee_transfer_evaluator_tests
