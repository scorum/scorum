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

#if 0
SCORUM_TEST_CASE(validate_development_committee_top_budgets_operaton)
{
    development_committee_change_top_post_budgets_amount_operation op;

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

    development_committee_change_top_post_budgets_amount_operation op;

    BOOST_CHECK_NE(initial_amount, new_amount);

    op.amount = new_amount;

    development_committee_change_top_post_budgets_amount_evaluator evaluator(*services);

    dev_committee_object dev_committee = create_object<dev_committee_object>(shm, [](dev_committee_object& pool) {
            std::vector<percent_type> vcg_coeffs(SCORUM_DEFAULT_BUDGETS_VCG_SET);
            std::copy(vcg_coeffs.cbegin(), vcg_coeffs.cend(), std::back_inserter(pool.vcg_post_coefficients));
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
#endif

} // namespace development_committee_top_budgets_evaluator_tests
