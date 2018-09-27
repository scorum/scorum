#include <boost/test/unit_test.hpp>

#include <scorum/chain/data_service_factory.hpp>
#include <scorum/chain/services/advertising_property.hpp>

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
    development_committee_change_post_budgets_auction_properties_operation op;

    SCORUM_REQUIRE_THROW(op.validate(), fc::assert_exception);

    op.auction_coefficients = {};

    SCORUM_REQUIRE_THROW(op.validate(), fc::assert_exception);

    op.auction_coefficients = SCORUM_DEFAULT_BUDGETS_AUCTION_SET;

    BOOST_CHECK_NO_THROW(op.validate());

    op.auction_coefficients = { 90, 50 };

    BOOST_CHECK_NO_THROW(op.validate());
}

struct fixture : public shared_memory_fixture
{
    MockRepository mocks;

    data_service_factory_i* services = mocks.Mock<data_service_factory_i>();

    advertising_property_service_i* advertising_property_service = mocks.Mock<advertising_property_service_i>();

    using position_weights_type = std::vector<percent_type>;

    fixture()
        : shared_memory_fixture()
    {
        mocks.ExpectCall(services, data_service_factory_i::advertising_property_service)
            .ReturnByRef(*advertising_property_service);
    }
};

BOOST_FIXTURE_TEST_CASE(change_top_budgets_amount, fixture)
{
    static const position_weights_type initial_auction_coeffs(SCORUM_DEFAULT_BUDGETS_AUCTION_SET);
    static const position_weights_type new_auction_coeffs{ 90, 50 };

    development_committee_change_post_budgets_auction_properties_operation op;

    BOOST_REQUIRE(!std::equal(std::begin(initial_auction_coeffs), std::end(initial_auction_coeffs),
                              std::begin(new_auction_coeffs)));

    op.auction_coefficients = new_auction_coeffs;

    development_committee_change_top_post_budgets_amount_evaluator evaluator(*services);

    advertising_property_object adv_property
        = create_object<advertising_property_object>(shm, [](advertising_property_object& pool) {
              std::copy(std::begin(initial_auction_coeffs), std::end(initial_auction_coeffs),
                        std::back_inserter(pool.auction_post_coefficients));
          });

    BOOST_REQUIRE_EQUAL_COLLECTIONS(std::begin(adv_property.auction_post_coefficients),
                                    std::end(adv_property.auction_post_coefficients),
                                    std::begin(initial_auction_coeffs), std::end(initial_auction_coeffs));

    mocks
        .ExpectCallOverload(
            advertising_property_service,
            (void (advertising_property_service_i::*)(const advertising_property_service_i::modifier_type&))
                & advertising_property_service_i::update)
        .Do([&](const advertising_property_service_i::modifier_type& m) { m(adv_property); });

    BOOST_REQUIRE_NO_THROW(evaluator.do_apply(op));

    BOOST_REQUIRE_EQUAL_COLLECTIONS(std::begin(adv_property.auction_post_coefficients),
                                    std::end(adv_property.auction_post_coefficients),
                                    std::begin(op.auction_coefficients), std::end(op.auction_coefficients));
}

} // namespace development_committee_top_budgets_evaluator_tests
