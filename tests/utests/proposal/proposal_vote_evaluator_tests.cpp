#include <boost/test/unit_test.hpp>

#include <scorum/chain/data_service_factory.hpp>

#include <scorum/chain/services/account.hpp>
#include <scorum/chain/services/proposal.hpp>
#include <scorum/chain/services/proposal_executor.hpp>
#include <scorum/chain/services/registration_committee.hpp>
#include <scorum/chain/services/dynamic_global_property.hpp>

#include <scorum/chain/evaluators/proposal_vote_evaluator.hpp>

#include <scorum/chain/schema/proposal_object.hpp>

#include "defines.hpp"
#include "object_wrapper.hpp"

#include <hippomocks.h>

namespace proposal_vote_evaluator_tests {

using namespace scorum::chain;
using namespace scorum::protocol;

typedef void (account_service_i::*check_account_existence_signature)(const account_name_type& a,
                                                                     const fc::optional<const char*>& b) const;

struct fixture : public shared_memory_fixture
{
    MockRepository mocks;

    data_service_factory_i* services = mocks.Mock<data_service_factory_i>();

    account_service_i* account_service = mocks.Mock<account_service_i>();
    proposal_service_i* proposal_service = mocks.Mock<proposal_service_i>();
    proposal_executor_service_i* proposal_executor = mocks.Mock<proposal_executor_service_i>();
    registration_committee_service_i* committee_service = mocks.Mock<registration_committee_service_i>();
    dynamic_global_property_service_i* property_service = mocks.Mock<dynamic_global_property_service_i>();

    proposal_vote_operation operation;

    fixture()
        : shared_memory_fixture()
    {
        mocks.ExpectCall(services, data_service_factory_i::account_service).ReturnByRef(*account_service);
        mocks.ExpectCall(services, data_service_factory_i::proposal_service).ReturnByRef(*proposal_service);
        mocks.ExpectCall(services, data_service_factory_i::dynamic_global_property_service)
            .ReturnByRef(*property_service);
        mocks.ExpectCall(services, data_service_factory_i::proposal_executor_service).ReturnByRef(*proposal_executor);

        operation.proposal_id = 1;
        operation.voting_account = "bob";
    }
};

BOOST_FIXTURE_TEST_SUITE(proposal_vote_evaluator_tests, fixture)

SCORUM_TEST_CASE(check_proposal_id_and_throw_when_it_return_false)
{
    mocks.ExpectCall(proposal_service, proposal_service_i::is_exists).With(operation.proposal_id).Return(false);

    proposal_vote_evaluator evaluator(*services);
    BOOST_CHECK_THROW(evaluator.do_apply(operation), fc::assert_exception);
}

SCORUM_TEST_CASE(get_committee_service_if_proposal_exists_and_fail_on_voting_account_existence_check)
{
    proposal_object proposal = create_object<proposal_object>(shm, [](proposal_object& p) {
        registration_committee_add_member_operation op;
        op.account_name = "karl";

        p.creator = "alice";
        p.operation = op;
    });

    mocks.ExpectCall(proposal_service, proposal_service_i::is_exists).With(operation.proposal_id).Return(true);
    mocks.ExpectCall(proposal_service, proposal_service_i::get).With(operation.proposal_id).ReturnByRef(proposal);
    mocks.ExpectCall(services, data_service_factory_i::registration_committee_service).ReturnByRef(*committee_service);
    mocks.ExpectCall(committee_service, committee_i::is_exists).With(operation.voting_account).Return(false);

    proposal_vote_evaluator evaluator(*services);
    BOOST_CHECK_THROW(evaluator.do_apply(operation), fc::assert_exception);
}

SCORUM_TEST_CASE(check_voting_account_existence)
{
    proposal_object proposal = create_object<proposal_object>(shm, [&](proposal_object& p) {
        registration_committee_add_member_operation op;
        op.account_name = "karl";

        p.creator = "alice";
        p.operation = op;

        p.voted_accounts.insert(operation.voting_account);
    });

    mocks.OnCall(proposal_service, proposal_service_i::is_exists).With(operation.proposal_id).Return(true);
    mocks.OnCall(proposal_service, proposal_service_i::get).With(operation.proposal_id).ReturnByRef(proposal);
    mocks.OnCall(services, data_service_factory_i::registration_committee_service).ReturnByRef(*committee_service);
    mocks.ExpectCall(committee_service, committee_i::is_exists).With(operation.voting_account).Return(true);

    mocks
        .ExpectCallOverload(account_service,
                            (check_account_existence_signature)&account_service_i::check_account_existence)
        .With(operation.voting_account, _);

    proposal_vote_evaluator evaluator(*services);
    BOOST_CHECK_THROW(evaluator.do_apply(operation), fc::assert_exception);
}

SCORUM_TEST_CASE(throw_on_check_voting_account_existence_when_account_is_already_voted)
{
    proposal_object proposal = create_object<proposal_object>(shm, [&](proposal_object& p) {
        registration_committee_add_member_operation op;
        op.account_name = "karl";
        p.creator = "alice";
        p.operation = op;

        p.voted_accounts.insert(operation.voting_account);
    });

    mocks.OnCall(proposal_service, proposal_service_i::is_exists).With(operation.proposal_id).Return(true);
    mocks.OnCall(proposal_service, proposal_service_i::get).With(operation.proposal_id).ReturnByRef(proposal);
    mocks.OnCall(services, data_service_factory_i::registration_committee_service).ReturnByRef(*committee_service);
    mocks.ExpectCall(committee_service, committee_i::is_exists).With(operation.voting_account).Return(true);

    mocks
        .ExpectCallOverload(account_service,
                            (check_account_existence_signature)&account_service_i::check_account_existence)
        .With(operation.voting_account, _);

    proposal_vote_evaluator evaluator(*services);
    BOOST_CHECK_THROW(evaluator.do_apply(operation), fc::assert_exception);
}

SCORUM_TEST_CASE(throw_when_proposal_expired)
{
    proposal_object proposal = create_object<proposal_object>(shm, [](proposal_object& p) {
        registration_committee_add_member_operation op;
        op.account_name = "karl";
        p.creator = "alice";
        p.operation = op;
    });

    mocks.OnCall(proposal_service, proposal_service_i::is_exists).With(operation.proposal_id).Return(true);
    mocks.OnCall(proposal_service, proposal_service_i::get).With(operation.proposal_id).ReturnByRef(proposal);
    mocks.OnCall(services, data_service_factory_i::registration_committee_service).ReturnByRef(*committee_service);
    mocks.ExpectCall(committee_service, committee_i::is_exists).With(operation.voting_account).Return(true);

    mocks
        .ExpectCallOverload(account_service,
                            (check_account_existence_signature)&account_service_i::check_account_existence)
        .With(operation.voting_account, _);

    mocks.ExpectCall(proposal_service, proposal_service_i::is_expired).With(_).Return(true);

    proposal_vote_evaluator evaluator(*services);
    BOOST_CHECK_THROW(evaluator.do_apply(operation), fc::assert_exception);
}

SCORUM_TEST_CASE(vote_for_proposal_if_it_is_not_expired_and_execute)
{
    const proposal_object proposal = create_object<proposal_object>(shm, [](proposal_object& p) {
        registration_committee_add_member_operation op;
        op.account_name = "karl";
        p.creator = "alice";
        p.operation = op;
    });

    mocks.OnCall(proposal_service, proposal_service_i::is_exists).With(operation.proposal_id).Return(true);
    mocks.OnCall(proposal_service, proposal_service_i::get).With(operation.proposal_id).ReturnByRef(proposal);
    mocks.OnCall(services, data_service_factory_i::registration_committee_service).ReturnByRef(*committee_service);
    mocks.ExpectCall(committee_service, committee_i::is_exists).With(operation.voting_account).Return(true);

    mocks
        .ExpectCallOverload(account_service,
                            (check_account_existence_signature)&account_service_i::check_account_existence)
        .With(operation.voting_account, _);

    mocks.ExpectCall(proposal_service, proposal_service_i::is_expired).With(_).Return(false);

    mocks.ExpectCall(proposal_service, proposal_service_i::vote_for).With(operation.voting_account, _);

    mocks.ExpectCall(proposal_executor, proposal_executor_service_i::operator()).With(_);

    proposal_vote_evaluator evaluator(*services);
    evaluator.do_apply(operation);
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE(test_is_quorum)

BOOST_AUTO_TEST_CASE(needs_six_and_above_votes)
{
    BOOST_CHECK_EQUAL(true, scorum::chain::utils::is_quorum(6, 10, 60));
    BOOST_CHECK_EQUAL(false, scorum::chain::utils::is_quorum(5, 10, 60));
}

BOOST_AUTO_TEST_CASE(needs_five_and_above_votes_for_quourum)
{
    BOOST_CHECK_EQUAL(false, scorum::chain::utils::is_quorum(4, 8, 60));
    BOOST_CHECK_EQUAL(true, scorum::chain::utils::is_quorum(5, 8, 60));
}

BOOST_AUTO_TEST_SUITE_END()

} // namespace proposal_vote_evaluator_tests
