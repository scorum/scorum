#include <boost/test/unit_test.hpp>

#include <scorum/chain/data_service_factory.hpp>
#include <scorum/chain/services/dev_pool.hpp>

#include <scorum/protocol/types.hpp>

#include <scorum/chain/evaluators/proposal_evaluators.hpp>

#include "defines.hpp"

#include "object_wrapper.hpp"

#include <hippomocks.h>

#include <vector>

namespace development_committee_top_budgets_evaluator_tests {

using namespace scorum::chain;
using namespace scorum::protocol;

SCORUM_TEST_CASE(validate_development_committee_top_budgets_operaton)
{
    development_committee_change_post_budgets_vcg_properties_operation op;

    SCORUM_REQUIRE_THROW(op.validate(), fc::assert_exception);

    op.vcg_coefficients = {};

    SCORUM_REQUIRE_THROW(op.validate(), fc::assert_exception);

    op.vcg_coefficients = SCORUM_DEFAULT_BUDGETS_VCG_SET;

    BOOST_CHECK_NO_THROW(op.validate());

    op.vcg_coefficients = { 90, 50 };

    BOOST_CHECK_NO_THROW(op.validate());
}

struct fixture : public shared_memory_fixture
{
    MockRepository mocks;

    data_service_factory_i* services = mocks.Mock<data_service_factory_i>();

    dev_pool_service_i* dev_pool_service = mocks.Mock<dev_pool_service_i>();

    using position_weights_type = std::vector<percent_type>;

    fixture()
        : shared_memory_fixture()
    {
        mocks.ExpectCall(services, data_service_factory_i::dev_pool_service).ReturnByRef(*dev_pool_service);
    }
};

BOOST_FIXTURE_TEST_CASE(change_top_budgets_amount, fixture)
{
    static const position_weights_type initial_vcg_coeffs(SCORUM_DEFAULT_BUDGETS_VCG_SET);
    static const position_weights_type new_vcg_coeffs{ 90, 50 };

    development_committee_change_post_budgets_vcg_properties_operation op;

    BOOST_REQUIRE(
        !std::equal(std::begin(initial_vcg_coeffs), std::end(initial_vcg_coeffs), std::begin(new_vcg_coeffs)));

    op.vcg_coefficients = new_vcg_coeffs;

    development_committee_change_top_post_budgets_amount_evaluator evaluator(*services);

    dev_committee_object dev_committee = create_object<dev_committee_object>(shm, [](dev_committee_object& pool) {
        std::copy(std::begin(initial_vcg_coeffs), std::end(initial_vcg_coeffs),
                  std::back_inserter(pool.vcg_post_coefficients));
    });

    BOOST_REQUIRE(std::equal(std::begin(dev_committee.vcg_post_coefficients),
                             std::end(dev_committee.vcg_post_coefficients), std::begin(initial_vcg_coeffs)));

    mocks
        .ExpectCallOverload(dev_pool_service,
                            (void (dev_pool_service_i::*)(const dev_pool_service_i::modifier_type&))
                                & dev_pool_service_i::update)
        .Do([&](const dev_pool_service_i::modifier_type& m) { m(dev_committee); });

    BOOST_REQUIRE_NO_THROW(evaluator.do_apply(op));

    BOOST_REQUIRE(std::equal(std::begin(dev_committee.vcg_post_coefficients),
                             std::end(dev_committee.vcg_post_coefficients), std::begin(op.vcg_coefficients)));
}

} // namespace development_committee_top_budgets_evaluator_tests
