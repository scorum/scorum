#include <boost/test/unit_test.hpp>

#include "defines.hpp"

#include <scorum/protocol/scorum_operations.hpp>

#include <scorum/chain/proposal_create_evaluator_new.hpp>
#include <scorum/chain/proposal_object.hpp>
#include <scorum/chain/data_service_factory.hpp>

#include <hippomocks.h>

using namespace scorum::chain;
using namespace scorum::protocol;

BOOST_AUTO_TEST_CASE(test_one)
{
    MockRepository mocks;

    account_service_i* account_service_ptr = mocks.Mock<account_service_i>();
    proposal_service_i* proposal_service_ptr = mocks.Mock<proposal_service_i>();
    committee_service_i* committee_service_ptr = mocks.Mock<committee_service_i>();

    data_service_factory_i* services_ptr = mocks.Mock<data_service_factory_i>();

    proposal_create_operation op;
    op.creator = "alice";
    op.lifetime_sec = SCORUM_PROPOSAL_LIFETIME_MIN_SECONDS;
    op.action = proposal_action::invite;
    op.committee_member = "bob";

    fc::time_point_sec current_time = fc::time_point_sec();
    fc::time_point_sec expiration = current_time + op.lifetime_sec;

    proposal_create_evaluator_new<scorum::protocol::operation> evaluator(*services_ptr);

    mocks.ExpectCall(services_ptr, data_service_factory_i::account_service).ReturnByRef(*account_service_ptr);
    mocks.ExpectCall(services_ptr, data_service_factory_i::proposal_service).ReturnByRef(*proposal_service_ptr);
    mocks.ExpectCall(services_ptr, data_service_factory_i::committee_service).ReturnByRef(*committee_service_ptr);

    mocks.ExpectCall(committee_service_ptr, committee_service_i::member_exists).With(op.creator).Return(true);

    mocks.ExpectCall(account_service_ptr, account_service_i::check_account_existence)
        .With(op.creator, fc::optional<const char*>());
    mocks.ExpectCall(account_service_ptr, account_service_i::check_account_existence)
        .With(op.committee_member, fc::optional<const char*>());

    mocks.ExpectCall(services_ptr, data_service_factory_i::head_block_time).Return(current_time);

    //    proposal_object p;

    mocks.ExpectCall(proposal_service_ptr, proposal_service_i::create)
        .With(op.creator, op.committee_member, proposal_action::invite, expiration, SCORUM_COMMITTEE_QUORUM_PERCENT);

    evaluator.do_apply(op);
}
