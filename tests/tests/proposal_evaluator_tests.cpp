#include <boost/test/unit_test.hpp>

#include <scorum/protocol/exceptions.hpp>

#include <scorum/protocol/types.hpp>
#include <scorum/chain/dbs_committee_proposal.hpp>
#include <scorum/chain/proposal_vote_evaluator.hpp>
#include <scorum/chain/proposal_vote_object.hpp>

#include "defines.hpp"

namespace tests {

using scorum::protocol::account_name_type;
using scorum::chain::proposal_vote_object;
using scorum::chain::proposal_vote_operation;
using scorum::protocol::registration_committee_proposal_action;

class account_service_mock
{
public:
    bool check_account_existence()
    {
        b_check_account_existence = true;

        return true;
    }

    bool b_check_account_existence = false;
};

class proposal_service_mock
{
public:
    void vote_for()
    {
        b_vote_for = true;
    }

    void remove()
    {
        b_remove = true;
    }

    bool b_vote_for = false;
    bool b_remove = false;

    proposal_vote_object& proposal;
};

class committee_service_mock
{
public:
    void add_member(const account_name_type&)
    {
        b_add_member = true;
    }

    void exclude_member(const account_name_type&)
    {
        b_exclude_member = true;
    }

    bool b_add_member = false;
    bool b_exclude_member = false;
};

typedef scorum::chain::proposal_vote_evaluator_t<account_service_mock, proposal_service_mock, committee_service_mock>
    evaluator_mocked;

class proposal_evaluator_fixture
{
public:
    proposal_evaluator_fixture()
        : evaluator(account_service, proposal_service, committee_service, 10)
    {
    }

    ~proposal_evaluator_fixture()
    {
    }

    proposal_vote_object& proposal()
    {
        return proposal_service.proposal;
    }

    proposal_vote_operation op;

    account_service_mock account_service;
    proposal_service_mock proposal_service;
    committee_service_mock committee_service;

    void apply()
    {
        evaluator.do_apply(op);
    }

    evaluator_mocked evaluator;
};

BOOST_FIXTURE_TEST_SUITE(proposal_vote_evaluator_tests, proposal_evaluator_fixture)

SCORUM_TEST_CASE(add_new_member)
{
    proposal().creator = "alice";
    proposal().member = "bob";
    proposal().action = registration_committee_proposal_action::invite;

    op.voting_account = "alice";
    op.committee_member = "bob";

    apply();
}

SCORUM_TEST_CASE(dropout_member)
{
}

SCORUM_TEST_CASE(proposal_removed_after_adding_member)
{
}

SCORUM_TEST_CASE(proposal_removed_after_droping_out_member)
{
}

SCORUM_TEST_CASE(fail_on_voting_account_unexistent)
{
}

SCORUM_TEST_CASE(fail_on_proposal_expired)
{
}

BOOST_AUTO_TEST_SUITE_END()

} // namespace tests
