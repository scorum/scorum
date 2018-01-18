#include <boost/variant.hpp>
#include <boost/test/unit_test.hpp>

#include "defines.hpp"

#include <scorum/protocol/scorum_operations.hpp>

#include <scorum/chain/proposal_create_evaluator_new.hpp>
#include <scorum/chain/proposal_object.hpp>
#include <scorum/chain/data_service_factory.hpp>

#include <hippomocks.h>

using namespace scorum::chain;
using namespace scorum::protocol;

struct proposal_create_fixture
{
    MockRepository mocks;

    data_service_factory_i* services = mocks.Mock<data_service_factory_i>();

    account_service_i* account_service = mocks.Mock<account_service_i>();
    proposal_service_i* proposal_service = mocks.Mock<proposal_service_i>();
    registration_committee_service_i* committee_service = mocks.Mock<registration_committee_service_i>();
    dynamic_global_property_service_i* property_service = mocks.Mock<dynamic_global_property_service_i>();

    proposal_object proposal;
    dynamic_global_property_object global_property;

    const fc::time_point_sec current_time = fc::time_point_sec();

    proposal_create_fixture()
    {
        global_property.invite_quorum = 71u;
        global_property.dropout_quorum = 72u;
        global_property.change_quorum = 73u;

        mocks.ExpectCall(services, data_service_factory_i::account_service).ReturnByRef(*account_service);
        mocks.ExpectCall(services, data_service_factory_i::proposal_service).ReturnByRef(*proposal_service);
        mocks.ExpectCall(services, data_service_factory_i::registration_committee_service)
            .ReturnByRef(*committee_service);
        mocks.ExpectCall(services, data_service_factory_i::dynamic_global_property_service)
            .ReturnByRef(*property_service);
    }

    void create_expectations(const proposal_create_operation& op)
    {
        const fc::time_point_sec expected_expiration = current_time + op.lifetime_sec;

        mocks.ExpectCall(committee_service, registration_committee_service_i::member_exists)
            .With(op.creator)
            .Return(true);

        mocks.ExpectCall(account_service, account_service_i::check_account_existence)
            .With(op.creator, fc::optional<const char*>());

        mocks.ExpectCall(property_service, dynamic_global_property_service_i::head_block_time).Return(current_time);

        mocks.ExpectCall(property_service, dynamic_global_property_service_i::get_dynamic_global_properties)
            .ReturnByRef(global_property);

        proposal_action action = *op.action;

        mocks.ExpectCall(proposal_service, proposal_service_i::create)
            .With(op.creator, _, action, expected_expiration, global_property.invite_quorum)
            .ReturnByRef(proposal);
    }
};

BOOST_AUTO_TEST_SUITE(proposal_create_evaluator_new_tests)

BOOST_FIXTURE_TEST_CASE(create_proposal, proposal_create_fixture)
{
    proposal_create_operation op;
    op.creator = "alice";
    op.lifetime_sec = SCORUM_PROPOSAL_LIFETIME_MIN_SECONDS;
    op.action = proposal_action::invite;
    op.data = "bob";

    create_expectations(op);

    proposal_create_evaluator_new evaluator(*services);

    BOOST_CHECK_NO_THROW(evaluator.do_apply(op));
}

BOOST_FIXTURE_TEST_CASE(throw_exception_if_lifetime_is_to_small, proposal_create_fixture)
{
    proposal_create_operation op;
    op.lifetime_sec = SCORUM_PROPOSAL_LIFETIME_MIN_SECONDS - 1;

    proposal_create_evaluator_new evaluator(*services);

    SCORUM_CHECK_EXCEPTION(evaluator.do_apply(op), fc::exception,
                           "Proposal life time is not in range of 86400 - 864000 seconds.");
}

BOOST_FIXTURE_TEST_CASE(throw_exception_if_lifetime_is_to_big, proposal_create_fixture)
{
    proposal_create_operation op;
    op.lifetime_sec = SCORUM_PROPOSAL_LIFETIME_MAX_SECONDS + 1;

    proposal_create_evaluator_new evaluator(*services);

    SCORUM_CHECK_EXCEPTION(evaluator.do_apply(op), fc::exception,
                           "Proposal life time is not in range of 86400 - 864000 seconds.");
}

BOOST_AUTO_TEST_SUITE_END()
