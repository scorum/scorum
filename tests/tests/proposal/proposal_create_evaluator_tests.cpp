#include <boost/variant.hpp>
#include <boost/test/unit_test.hpp>

#include "defines.hpp"

#include <scorum/protocol/scorum_operations.hpp>

#include <scorum/chain/proposal_create_evaluator.hpp>
#include <scorum/chain/schema/proposal_object.hpp>
#include <scorum/chain/schema/dynamic_global_property_object.hpp>
#include <scorum/chain/data_service_factory.hpp>

#include <scorum/chain/services/registration_committee.hpp>
#include <scorum/chain/services/account.hpp>
#include <scorum/chain/services/dynamic_global_property.hpp>
#include <scorum/chain/services/proposal.hpp>

#include <hippomocks.h>

using namespace scorum::chain;
using namespace scorum::protocol;

struct proposal_create_evaluator_fixture
{
    MockRepository mocks;

    data_service_factory_i* services = mocks.Mock<data_service_factory_i>();

    account_service_i* account_service = mocks.Mock<account_service_i>();
    proposal_service_i* proposal_service = mocks.Mock<proposal_service_i>();
    registration_committee_service_i* committee_service = mocks.Mock<registration_committee_service_i>();
    dynamic_global_property_service_i* property_service = mocks.Mock<dynamic_global_property_service_i>();

    proposal_create_evaluator_fixture()
    {
        mocks.ExpectCall(services, data_service_factory_i::account_service).ReturnByRef(*account_service);
        mocks.ExpectCall(services, data_service_factory_i::proposal_service).ReturnByRef(*proposal_service);
        mocks.ExpectCall(services, data_service_factory_i::registration_committee_service)
            .ReturnByRef(*committee_service);
        mocks.ExpectCall(services, data_service_factory_i::dynamic_global_property_service)
            .ReturnByRef(*property_service);
    }
};

struct create_proposal_fixture : public proposal_create_evaluator_fixture
{
    proposal_object proposal;
    dynamic_global_property_object global_property;

    const fc::time_point_sec current_time = fc::time_point_sec();

    uint64_t expected_quorum = 0;

    typedef void (account_service_i::*check_account_existence_signature)(const account_name_type& a,
                                                                         const fc::optional<const char*>& b) const;

    create_proposal_fixture()
    {
        global_property.invite_quorum = 71u;
        global_property.dropout_quorum = 72u;
        global_property.change_quorum = 73u;
    }

    void create_expectations(const proposal_create_operation& op)
    {
        mocks.ExpectCall(committee_service, registration_committee_service_i::is_exists).With(op.creator).Return(true);

        mocks
            .ExpectCallOverload(account_service,
                                (check_account_existence_signature)&account_service_i::check_account_existence)
            .With(op.creator, _);

        mocks.ExpectCall(property_service, dynamic_global_property_service_i::head_block_time).Return(current_time);

        mocks.ExpectCall(property_service, dynamic_global_property_service_i::get_dynamic_global_properties)
            .ReturnByRef(global_property);
    }

    void create_proposal(proposal_action expected_action)
    {
        proposal_create_operation op;
        op.creator = "alice";
        op.data = "bob";
        op.lifetime_sec = SCORUM_PROPOSAL_LIFETIME_MIN_SECONDS + 1;
        op.action = expected_action;

        create_expectations(op);

        mocks.ExpectCall(proposal_service, proposal_service_i::create)
            .With(_, _, expected_action, _, _)
            .ReturnByRef(proposal);

        proposal_create_evaluator evaluator(*services);

        evaluator.do_apply(op);
    }

    uint64_t get_quorum(proposal_action action)
    {
        mocks.ExpectCall(property_service, dynamic_global_property_service_i::get_dynamic_global_properties)
            .ReturnByRef(global_property);

        proposal_create_evaluator evaluator(*services);

        return evaluator.get_quorum(action);
    }
};

BOOST_AUTO_TEST_SUITE(proposal_create_evaluator_tests)

BOOST_FIXTURE_TEST_CASE(expiration_time_is_sum_of_head_block_time_and_lifetime, create_proposal_fixture)
{
    proposal_create_operation op;
    op.creator = "alice";
    op.data = "bob";
    op.lifetime_sec = SCORUM_PROPOSAL_LIFETIME_MIN_SECONDS + 1;
    op.action = proposal_action::invite;

    create_expectations(op);

    const fc::time_point_sec expected_expiration = current_time + op.lifetime_sec;

    mocks.ExpectCall(proposal_service, proposal_service_i::create)
        .With(_, _, _, expected_expiration, _)
        .ReturnByRef(proposal);

    proposal_create_evaluator evaluator(*services);

    evaluator.do_apply(op);
}

BOOST_FIXTURE_TEST_CASE(get_quorum_for_invite_proposal, create_proposal_fixture)
{
    BOOST_CHECK_EQUAL(global_property.invite_quorum, get_quorum(proposal_action::invite));
}

BOOST_FIXTURE_TEST_CASE(get_quorum_for_dropout_proposal, create_proposal_fixture)
{
    BOOST_CHECK_EQUAL(global_property.dropout_quorum, get_quorum(proposal_action::dropout));
}

BOOST_FIXTURE_TEST_CASE(get_quorum_for_change_invite_quorum_proposal, create_proposal_fixture)
{
    BOOST_CHECK_EQUAL(global_property.change_quorum, get_quorum(proposal_action::change_invite_quorum));
}

BOOST_FIXTURE_TEST_CASE(get_quorum_for_change_dropout_quorum_proposal, create_proposal_fixture)
{
    BOOST_CHECK_EQUAL(global_property.change_quorum, get_quorum(proposal_action::change_dropout_quorum));
}

BOOST_FIXTURE_TEST_CASE(get_quorum_for_change_quorum_proposal, create_proposal_fixture)
{
    BOOST_CHECK_EQUAL(global_property.change_quorum, get_quorum(proposal_action::change_quorum));
}

BOOST_FIXTURE_TEST_CASE(create_invite_proposal, create_proposal_fixture)
{
    BOOST_CHECK_NO_THROW(create_proposal(proposal_action::invite));
}

BOOST_FIXTURE_TEST_CASE(create_dropout_proposal, create_proposal_fixture)
{
    BOOST_CHECK_NO_THROW(create_proposal(proposal_action::dropout));
}

BOOST_FIXTURE_TEST_CASE(create_change_invite_quorum_proposal, create_proposal_fixture)
{
    BOOST_CHECK_NO_THROW(create_proposal(proposal_action::change_invite_quorum));
}

BOOST_FIXTURE_TEST_CASE(create_change_dropout_quorum_proposal, create_proposal_fixture)
{
    BOOST_CHECK_NO_THROW(create_proposal(proposal_action::change_dropout_quorum));
}

BOOST_FIXTURE_TEST_CASE(create_change_quorum_proposal, create_proposal_fixture)
{
    BOOST_CHECK_NO_THROW(create_proposal(proposal_action::change_quorum));
}

BOOST_FIXTURE_TEST_CASE(throw_exception_if_lifetime_is_to_small, proposal_create_evaluator_fixture)
{
    proposal_create_operation op;
    op.lifetime_sec = SCORUM_PROPOSAL_LIFETIME_MIN_SECONDS - 1;

    proposal_create_evaluator evaluator(*services);

    SCORUM_CHECK_EXCEPTION(evaluator.do_apply(op), fc::exception,
                           "Proposal life time is not in range of 86400 - 864000 seconds.");
}

BOOST_FIXTURE_TEST_CASE(throw_exception_if_lifetime_is_to_big, proposal_create_evaluator_fixture)
{
    proposal_create_operation op;
    op.lifetime_sec = SCORUM_PROPOSAL_LIFETIME_MAX_SECONDS + 1;

    proposal_create_evaluator evaluator(*services);

    SCORUM_CHECK_EXCEPTION(evaluator.do_apply(op), fc::exception,
                           "Proposal life time is not in range of 86400 - 864000 seconds.");
}

BOOST_FIXTURE_TEST_CASE(throw_when_creator_is_not_in_committee, proposal_create_evaluator_fixture)
{
    proposal_create_operation op;
    op.creator = "sam";
    op.lifetime_sec = SCORUM_PROPOSAL_LIFETIME_MIN_SECONDS + 1;

    mocks.ExpectCall(committee_service, registration_committee_service_i::is_exists).With(op.creator).Return(false);

    proposal_create_evaluator evaluator(*services);

    SCORUM_CHECK_EXCEPTION(evaluator.do_apply(op), fc::exception, "Account \"sam\" is not in committee.");
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE(proposal_create_operation_validate_tests)

BOOST_AUTO_TEST_CASE(throw_exception_if_creator_is_not_set)
{
    proposal_create_operation op;

    SCORUM_CHECK_EXCEPTION(op.validate(), fc::exception, "Account name  is invalid");
}

BOOST_AUTO_TEST_CASE(throw_exception_if_member_is_not_set)
{
    proposal_create_operation op;
    op.creator = "alice";

    auto test = [&](proposal_action action) {
        op.action = action;
        SCORUM_CHECK_EXCEPTION(op.validate(), fc::exception, "Account name  is invalid");
    };

    test(proposal_action::invite);
    test(proposal_action::dropout);
}

BOOST_AUTO_TEST_CASE(throw_exception_if_action_is_not_set)
{
    proposal_create_operation op;
    op.creator = "alice";

    SCORUM_CHECK_EXCEPTION(op.validate(), fc::exception, "Proposal is not set.");
}

BOOST_AUTO_TEST_CASE(pass_when_all_set)
{
    proposal_create_operation op;
    op.creator = "alice";
    op.data = "bob";

    op.action = proposal_action::invite;
    BOOST_CHECK_NO_THROW(op.validate());

    op.action = proposal_action::dropout;
    BOOST_CHECK_NO_THROW(op.validate());
}

BOOST_AUTO_TEST_CASE(throw_when_change_quorum_and_data_is_not_set)
{
    proposal_create_operation op;
    op.creator = "alice";

    op.action = proposal_action::change_quorum;
    BOOST_CHECK_THROW(op.validate(), fc::exception);

    op.action = proposal_action::change_invite_quorum;
    BOOST_CHECK_THROW(op.validate(), fc::exception);

    op.action = proposal_action::change_dropout_quorum;
    BOOST_CHECK_THROW(op.validate(), fc::exception);
}

BOOST_AUTO_TEST_CASE(throw_when_change_quorum_and_data_is_not_uint64)
{
    proposal_create_operation op;
    op.creator = "alice";
    op.data = "bob";

    op.action = proposal_action::change_quorum;
    SCORUM_CHECK_THROW(op.validate(), fc::exception);

    op.action = proposal_action::change_invite_quorum;
    SCORUM_CHECK_THROW(op.validate(), fc::exception);

    op.action = proposal_action::change_dropout_quorum;
    SCORUM_CHECK_THROW(op.validate(), fc::exception);
}

SCORUM_TEST_CASE(pass_when_change_quorum_and_data_is_uint64)
{
    proposal_create_operation op;
    op.creator = "alice";
    op.data = 60;

    op.action = proposal_action::change_quorum;
    BOOST_CHECK_NO_THROW(op.validate());

    op.action = proposal_action::change_invite_quorum;
    BOOST_CHECK_NO_THROW(op.validate());

    op.action = proposal_action::change_dropout_quorum;
    BOOST_CHECK_NO_THROW(op.validate());
}

SCORUM_TEST_CASE(throw_when_quorum_is_to_small)
{
    proposal_create_operation op;
    op.creator = "alice";
    op.data = SCORUM_MIN_QUORUM_VALUE_PERCENT;

    op.action = proposal_action::change_quorum;
    SCORUM_CHECK_EXCEPTION(op.validate(), fc::assert_exception, "Quorum is to small.");

    op.action = proposal_action::change_invite_quorum;
    SCORUM_CHECK_EXCEPTION(op.validate(), fc::assert_exception, "Quorum is to small.");

    op.action = proposal_action::change_dropout_quorum;
    SCORUM_CHECK_EXCEPTION(op.validate(), fc::assert_exception, "Quorum is to small.");
}

SCORUM_TEST_CASE(validate_fail_on_max_quorum_value)
{
    proposal_create_operation op;
    op.creator = "alice";
    op.data = SCORUM_MAX_QUORUM_VALUE_PERCENT;

    op.action = proposal_action::change_quorum;
    BOOST_CHECK_NO_THROW(op.validate());

    op.action = proposal_action::change_invite_quorum;
    BOOST_CHECK_NO_THROW(op.validate());

    op.action = proposal_action::change_dropout_quorum;
    BOOST_CHECK_NO_THROW(op.validate());
}

SCORUM_TEST_CASE(throw_when_quorum_is_to_large)
{
    proposal_create_operation op;
    op.creator = "alice";
    op.data = SCORUM_MAX_QUORUM_VALUE_PERCENT + 1;

    op.action = proposal_action::change_quorum;
    SCORUM_CHECK_EXCEPTION(op.validate(), fc::assert_exception, "Quorum is to large.");

    op.action = proposal_action::change_invite_quorum;
    SCORUM_CHECK_EXCEPTION(op.validate(), fc::assert_exception, "Quorum is to large.");

    op.action = proposal_action::change_dropout_quorum;
    SCORUM_CHECK_EXCEPTION(op.validate(), fc::assert_exception, "Quorum is to large.");
}

BOOST_AUTO_TEST_SUITE_END()
