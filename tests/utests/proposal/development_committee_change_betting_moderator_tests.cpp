#include <boost/test/unit_test.hpp>

#include <scorum/chain/data_service_factory.hpp>
#include <scorum/chain/services/betting_property.hpp>

#include <scorum/protocol/types.hpp>

#include <scorum/chain/evaluators/proposal_evaluators.hpp>

#include "defines.hpp"

#include "service_wrappers.hpp"

#include <hippomocks.h>

#include <vector>

namespace development_committee_chnage_betting_moderator_tests {

using namespace scorum::chain;
using namespace service_wrappers;

SCORUM_TEST_CASE(validate_development_committee_chnage_betting_moderator_operaton)
{
    development_committee_empower_betting_moderator_operation op;

    SCORUM_REQUIRE_THROW(op.validate(), fc::assert_exception);

    op.account = "smit";

    BOOST_CHECK_NO_THROW(op.validate());
}

struct development_committee_chnage_betting_moderator_fixture : public shared_memory_fixture
{
    const account_name_type old_moderator = "jack";
    const account_name_type new_moderator = "smit";

    MockRepository mocks;

    data_service_factory_i* services = mocks.Mock<data_service_factory_i>();

    service_base_wrapper<betting_property_service_i> betting_property;

    development_committee_chnage_betting_moderator_fixture()
        : betting_property(*this, mocks, [&](betting_property_object& bp) { bp.moderator = old_moderator; })
    {
        mocks.OnCall(services, data_service_factory_i::betting_property_service)
            .ReturnByRef(betting_property.service());
    }
};

BOOST_FIXTURE_TEST_CASE(change_betting_moderator, development_committee_chnage_betting_moderator_fixture)
{
    development_committee_empower_betting_moderator_evaluator evaluator(*services);

    BOOST_REQUIRE_EQUAL(betting_property.get().moderator, old_moderator);

    development_committee_empower_betting_moderator_operation op;

    op.account = new_moderator;

    BOOST_REQUIRE_NO_THROW(evaluator.do_apply(op));

    BOOST_REQUIRE_EQUAL(betting_property.get().moderator, new_moderator);
}

} // namespace validate_development_committee_chnage_betting_moderator_operaton
