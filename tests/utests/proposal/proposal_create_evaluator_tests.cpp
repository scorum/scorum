#include <boost/test/unit_test.hpp>
#include <boost/test/parameterized_test.hpp>

#include "defines.hpp"

#include <scorum/protocol/scorum_operations.hpp>

#include <scorum/chain/evaluators/proposal_create_evaluator.hpp>
#include <scorum/chain/schema/proposal_object.hpp>
#include <scorum/chain/schema/dynamic_global_property_object.hpp>
#include <scorum/chain/schema/registration_objects.hpp>
#include <scorum/chain/data_service_factory.hpp>

#include <scorum/chain/services/registration_committee.hpp>
#include <scorum/chain/services/account.hpp>
#include <scorum/chain/services/dynamic_global_property.hpp>
#include <scorum/chain/services/proposal.hpp>

#include "object_wrapper.hpp"

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
        mocks.ExpectCall(services, data_service_factory_i::dynamic_global_property_service)
            .ReturnByRef(*property_service);
    }
};

struct create_proposal_fixture : public shared_memory_fixture, public proposal_create_evaluator_fixture
{
    proposal_object proposal;
    registration_pool_object registration_pool;
    dynamic_global_property_object global_property;

    const fc::time_point_sec current_time = fc::time_point_sec();

    uint64_t expected_quorum = 0;

    typedef void (account_service_i::*check_account_existence_signature)(const account_name_type& a,
                                                                         const fc::optional<const char*>& b) const;

    create_proposal_fixture()
        : shared_memory_fixture()
        , proposal(create_object<proposal_object>(shm))
        , registration_pool(create_object<registration_pool_object>(shm))
        , global_property(create_object<dynamic_global_property_object>(shm))
    {
        registration_pool.invite_quorum = 71u;
        registration_pool.dropout_quorum = 72u;
        registration_pool.change_quorum = 73u;
    }

    void create_expectations()
    {
        mocks.OnCall(services, data_service_factory_i::registration_committee_service).ReturnByRef(*committee_service);
        mocks.OnCall(committee_service, committee_i::is_exists).With(_).Return(true);

        mocks
            .OnCallOverload(account_service,
                            (check_account_existence_signature)&account_service_i::check_account_existence)
            .With(_, _);

        mocks.OnCall(property_service, dynamic_global_property_service_i::head_block_time).Return(current_time);
        mocks.OnCall(services, data_service_factory_i::registration_committee_service).ReturnByRef(*committee_service);
        mocks.OnCall(committee_service, committee_i::get_add_member_quorum).Return(0u);
    }
};

BOOST_AUTO_TEST_SUITE(proposal_create_evaluator_tests)

BOOST_FIXTURE_TEST_CASE(expiration_time_is_sum_of_head_block_time_and_lifetime, create_proposal_fixture)
{
    proposal_create_operation op;
    op.creator = "alice";
    op.lifetime_sec = SCORUM_PROPOSAL_LIFETIME_MIN_SECONDS + 1;

    registration_committee_add_member_operation add_member_operation;
    add_member_operation.account_name = "bob";

    op.operation = add_member_operation;

    create_expectations();
    const fc::time_point_sec expected_expiration = current_time + op.lifetime_sec;

    mocks.ExpectCall(proposal_service, proposal_service_i::create)
        .With(_, _, expected_expiration, _)
        .ReturnByRef(proposal);

    proposal_create_evaluator evaluator(*services);
    evaluator.do_apply(op);
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

    mocks.OnCall(services, data_service_factory_i::registration_committee_service).ReturnByRef(*committee_service);
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

// clang-format off
typedef boost::mpl::list<registration_committee_add_member_operation,
                         registration_committee_exclude_member_operation,
                         development_committee_add_member_operation,
                         development_committee_exclude_member_operation>
    proposal_operations_on_members;

typedef boost::mpl::list<registration_committee_change_quorum_operation,
                         development_committee_change_quorum_operation>
    proposal_operations_on_quorum;
// clang-format on

BOOST_AUTO_TEST_CASE_TEMPLATE(dont_throw_exception_if_account_name_is_set_and_valid, T, proposal_operations_on_members)
{
    proposal_create_operation op;
    op.creator = "alice";

    T operation;
    operation.account_name = "bob";
    op.operation = operation;

    BOOST_CHECK_NO_THROW(op.validate());
}

BOOST_AUTO_TEST_CASE_TEMPLATE(throw_exception_if_account_name_is_not_set, T, proposal_operations_on_members)
{
    proposal_create_operation op;
    op.creator = "alice";

    T operation;
    op.operation = operation;

    SCORUM_CHECK_EXCEPTION(op.validate(), fc::assert_exception, "Account name  is invalid");
}

BOOST_AUTO_TEST_CASE_TEMPLATE(throw_exception_if_quorum_type_is_not_set, T, proposal_operations_on_quorum)
{
    proposal_create_operation op;
    op.creator = "alice";

    T operation;
    op.operation = operation;

    SCORUM_CHECK_EXCEPTION(op.validate(), fc::assert_exception, "Quorum type is not set.");
}

BOOST_AUTO_TEST_CASE_TEMPLATE(throw_exception_if_quorum_is_not_set, T, proposal_operations_on_quorum)
{
    auto test = [](quorum_type q) {
        T operation;
        operation.committee_quorum = q;

        proposal_create_operation op;
        op.creator = "alice";
        op.operation = operation;

        SCORUM_CHECK_EXCEPTION(op.validate(), fc::assert_exception, "Quorum is to small.");
    };

    test(add_member_quorum);
    test(exclude_member_quorum);
    test(base_quorum);
}

BOOST_AUTO_TEST_CASE_TEMPLATE(throw_exception_if_quorum_is_to_small, T, proposal_operations_on_quorum)
{
    auto test = [](quorum_type q) {
        T operation;
        operation.committee_quorum = q;
        operation.quorum = SCORUM_MIN_QUORUM_VALUE_PERCENT - 1;

        proposal_create_operation op;
        op.creator = "alice";
        op.operation = operation;

        SCORUM_CHECK_EXCEPTION(op.validate(), fc::assert_exception, "Quorum is to small.");
    };

    test(add_member_quorum);
    test(exclude_member_quorum);
    test(base_quorum);
}

BOOST_AUTO_TEST_CASE_TEMPLATE(throw_exception_if_quorum_is_to_large, T, proposal_operations_on_quorum)
{
    auto test = [](quorum_type q) {
        T operation;
        operation.committee_quorum = q;
        operation.quorum = SCORUM_MAX_QUORUM_VALUE_PERCENT + 1;

        proposal_create_operation op;
        op.creator = "alice";
        op.operation = operation;

        SCORUM_CHECK_EXCEPTION(op.validate(), fc::assert_exception, "Quorum is to large.");
    };

    test(add_member_quorum);
    test(exclude_member_quorum);
    test(base_quorum);
}

BOOST_AUTO_TEST_CASE_TEMPLATE(dont_throw_when_quorum_and_quorum_type_valid, T, proposal_operations_on_quorum)
{
    auto test = [](quorum_type q) {
        T operation;
        operation.committee_quorum = q;
        operation.quorum = SCORUM_MIN_QUORUM_VALUE_PERCENT + 1;

        proposal_create_operation op;
        op.creator = "alice";
        op.operation = operation;
        BOOST_CHECK_NO_THROW(op.validate());
    };

    test(add_member_quorum);
    test(exclude_member_quorum);
    test(base_quorum);
}

BOOST_AUTO_TEST_CASE_TEMPLATE(dont_throw_on_min_quorum_value, T, proposal_operations_on_quorum)
{
    auto test = [](quorum_type q) {
        T operation;
        operation.committee_quorum = q;
        operation.quorum = SCORUM_MIN_QUORUM_VALUE_PERCENT;

        proposal_create_operation op;
        op.creator = "alice";
        op.operation = operation;

        BOOST_CHECK_NO_THROW(op.validate());
    };

    test(add_member_quorum);
    test(exclude_member_quorum);
    test(base_quorum);
}

BOOST_AUTO_TEST_CASE_TEMPLATE(dont_throw_on_max_quorum_value, T, proposal_operations_on_quorum)
{
    auto test = [](quorum_type q) {
        T operation;
        operation.committee_quorum = q;
        operation.quorum = SCORUM_MIN_QUORUM_VALUE_PERCENT;

        proposal_create_operation op;
        op.creator = "alice";
        op.operation = operation;

        BOOST_CHECK_NO_THROW(op.validate());
    };

    test(add_member_quorum);
    test(exclude_member_quorum);
    test(base_quorum);
}

BOOST_AUTO_TEST_SUITE_END()
