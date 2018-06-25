#include <boost/test/unit_test.hpp>

#include <scorum/chain/data_service_factory.hpp>
#include <scorum/chain/services/dev_pool.hpp>

#include <scorum/chain/evaluators/proposal_evaluators.hpp>

#include "defines.hpp"

#include "object_wrapper.hpp"

#include <hippomocks.h>

namespace development_committee_top_budgets_evaluator_tests {

using namespace scorum::chain;
using namespace scorum::protocol;

SCORUM_TEST_CASE(validate_development_committee_top_budgets_operaton)
{
    SCORUM_MAKE_TOP_BUDGET_AMOUNT_OPERATION_CLS_NAME(post)::operation_type op;

    SCORUM_REQUIRE_THROW(op.validate(), fc::assert_exception);

    op.amount = 0;

    SCORUM_REQUIRE_THROW(op.validate(), fc::assert_exception);

    op.amount = SCORUM_DEFAULT_TOP_BUDGETS_AMOUNT;

    BOOST_CHECK_NO_THROW(op.validate());

    op.amount = 111;

    BOOST_CHECK_NO_THROW(op.validate());
}

struct fixture : public shared_memory_fixture
{
    MockRepository mocks;

    data_service_factory_i* services = mocks.Mock<data_service_factory_i>();

    dev_pool_service_i* dev_pool_service = mocks.Mock<dev_pool_service_i>();

    fixture()
        : shared_memory_fixture()
    {
        mocks.ExpectCall(services, data_service_factory_i::dev_pool_service).ReturnByRef(*dev_pool_service);
    }
};

BOOST_FIXTURE_TEST_CASE(change_top_budgets_amount, fixture)
{
    static const budget_type testing_type = budget_type::post;
    static const uint16_t initial_amount = 111;
    static const uint16_t new_amount = 222;

    SCORUM_MAKE_TOP_BUDGET_AMOUNT_OPERATION_CLS_NAME(post)::operation_type op;

    BOOST_CHECK_NE(initial_amount, new_amount);

    op.amount = new_amount;

    SCORUM_MAKE_TOP_BUDGET_AMOUNT_EVALUATOR_CLS_NAME(post) evaluator(*services);

    dev_committee_object dev_committee = create_object<dev_committee_object>(shm, [](dev_committee_object& pool) {
        pool.top_budgets_amounts.insert(std::make_pair(testing_type, initial_amount));
    });

    BOOST_CHECK_EQUAL(dev_committee.top_budgets_amounts.at(testing_type), initial_amount);

    mocks
        .ExpectCallOverload(dev_pool_service,
                            (void (dev_pool_service_i::*)(const dev_pool_service_i::modifier_type&))
                                & dev_pool_service_i::update)
        .Do([&](const dev_pool_service_i::modifier_type& m) { m(dev_committee); });

    BOOST_CHECK_NO_THROW(evaluator.do_apply(op));

    BOOST_CHECK_EQUAL(dev_committee.top_budgets_amounts.at(testing_type), op.amount);
}

} // namespace development_committee_top_budgets_evaluator_tests
