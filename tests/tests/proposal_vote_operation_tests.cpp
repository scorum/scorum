#include <boost/test/unit_test.hpp>

#include <scorum/protocol/exceptions.hpp>

#include <scorum/protocol/types.hpp>
#include <scorum/chain/dbs_proposal.hpp>
#include <scorum/chain/proposal_vote_evaluator.hpp>
#include <scorum/chain/proposal_vote_object.hpp>

#include "defines.hpp"

namespace tests {

using scorum::protocol::account_name_type;
using scorum::chain::proposal_vote_object;
using scorum::chain::proposal_vote_operation;
using scorum::protocol::proposal_action;
using scorum::chain::proposal_id_type;

class account_service_mock
{
public:
    bool is_exists(const account_name_type& account)
    {
        return existent_accounts.count(account) == 1 ? true : false;
    }

    std::set<account_name_type> existent_accounts;
};

class proposal_service_mock
{
public:
    using action_t = scorum::protocol::proposal_action;

    proposal_service_mock()
    {
    }

    void remove(const proposal_vote_object& proposal)
    {
        removed_proposals.push_back(proposal.id);
    }

    bool is_exist(const uint64_t id)
    {
        for (size_t i = 0; i < proposals.size(); ++i)
        {
            if (proposals[i].id == id)
                return true;
        }
        return false;
    }

    const proposal_vote_object& get(const uint64_t id)
    {
        for (proposal_vote_object& p : proposals)
        {
            if (p.id == id)
                return p;
        }

        BOOST_THROW_EXCEPTION(std::out_of_range("no such proposal"));
    }

    void vote_for(const proposal_vote_object& proposal)
    {
        voted_proposal.push_back(proposal.id);
    }

    bool is_expired(const proposal_vote_object& proposal)
    {
        for (proposal_id_type id : expired)
        {
            if (id == proposal.id)
                return true;
        }

        return false;
    }

    std::vector<proposal_vote_object> proposals;

    std::vector<proposal_id_type> removed_proposals;
    std::vector<proposal_id_type> voted_proposal;
    std::vector<proposal_id_type> expired;
};

class committee_service_mock
{
public:
    void add_member(const account_name_type& account)
    {
        added_members.push_back(account);
    }

    void exclude_member(const account_name_type& account)
    {
        excluded_members.push_back(account);
    }

    uint64_t get_quorum(uint64_t)
    {
        return quorum_votes;
    }

    bool member_exists(const account_name_type& account) const
    {
        return existent_accounts.count(account) == 1 ? true : false;
    }

    std::set<account_name_type> existent_accounts;

    std::vector<account_name_type> added_members;
    std::vector<account_name_type> excluded_members;

    uint64_t quorum_votes = 1;
};

typedef scorum::chain::proposal_vote_evaluator_t<account_service_mock, proposal_service_mock, committee_service_mock>
    evaluator_mocked;

class proposal_vote_evaluator_fixture
{
public:
    proposal_vote_evaluator_fixture()
        : evaluator(account_service, proposal_service, committee_service, 10)
    {
        account_service.existent_accounts.insert("alice");
        account_service.existent_accounts.insert("bob");

        committee_service.existent_accounts.insert("alice");
        committee_service.existent_accounts.insert("bob");

        proposal_vote_object proposal;
        proposal.creator = "alice";
        proposal.member = "bob";
        proposal.votes = 1;

        add(proposal);

        op.voting_account = "alice";
        op.proposal_id = proposal.id._id;
    }

    void add(proposal_vote_object& p)
    {
        p.id = proposal_service.proposals.size() + 1;
        proposal_service.proposals.push_back(p);
    }

    ~proposal_vote_evaluator_fixture()
    {
    }

    void apply()
    {
        evaluator.do_apply(op);
    }

    proposal_vote_object& proposal(size_t index = 0)
    {
        BOOST_REQUIRE(proposal_service.proposals.size() > 0);
        BOOST_REQUIRE(index < proposal_service.proposals.size());

        return proposal_service.proposals[index];
    }

    proposal_vote_operation op;

    account_service_mock account_service;
    proposal_service_mock proposal_service;
    committee_service_mock committee_service;

    evaluator_mocked evaluator;
};

BOOST_FIXTURE_TEST_SUITE(proposal_vote_evaluator_tests, proposal_vote_evaluator_fixture)

SCORUM_TEST_CASE(throw_when_creator_account_does_not_exists)
{
    account_service.existent_accounts.erase(account_service.existent_accounts.find("alice"));

    SCORUM_CHECK_EXCEPTION(apply(), fc::exception, "Account \"alice\" must exist.");
}

SCORUM_TEST_CASE(throw_when_creator_is_not_in_committee)
{
    committee_service.existent_accounts.erase(committee_service.existent_accounts.find("alice"));

    SCORUM_CHECK_EXCEPTION(apply(), fc::exception, "Account \"alice\" is not in commitee.");
}

SCORUM_TEST_CASE(throw_when_proposal_does_not_exists)
{
    op.proposal_id = 100;

    SCORUM_CHECK_EXCEPTION(apply(), fc::exception, "There is no proposal with id '100'");
}

SCORUM_TEST_CASE(add_one_new_member)
{
    proposal().action = proposal_action::invite;

    apply();

    BOOST_REQUIRE_EQUAL(committee_service.added_members.size(), 1);
    BOOST_CHECK_EQUAL(committee_service.added_members.front(), "bob");
}

SCORUM_TEST_CASE(dont_add_member_if_not_enough_quorum)
{
    proposal().action = proposal_action::invite;

    proposal().votes = 1;
    committee_service.quorum_votes = 2;

    apply();

    BOOST_REQUIRE_EQUAL(committee_service.added_members.size(), 0);
}

SCORUM_TEST_CASE(dont_dropout_if_not_enough_quorum)
{
    proposal().action = proposal_action::dropout;
    proposal().votes = 1;
    committee_service.quorum_votes = 2;

    apply();

    BOOST_REQUIRE_EQUAL(committee_service.excluded_members.size(), 0);
}

SCORUM_TEST_CASE(during_adding_we_do_not_remove_any_member)
{
    proposal().action = proposal_action::invite;

    apply();

    BOOST_CHECK_EQUAL(committee_service.excluded_members.size(), 0);
}

SCORUM_TEST_CASE(dropout_one_member)
{
    proposal().action = proposal_action::dropout;

    apply();

    BOOST_REQUIRE_EQUAL(committee_service.excluded_members.size(), 1);
    BOOST_CHECK_EQUAL(committee_service.excluded_members.front(), "bob");
}

SCORUM_TEST_CASE(during_dropping_we_do_not_add_new_members)
{
    proposal().action = proposal_action::dropout;

    apply();

    BOOST_CHECK_EQUAL(committee_service.added_members.size(), 0);
}

SCORUM_TEST_CASE(proposal_removed_after_droping_out_member)
{
    proposal().action = proposal_action::dropout;

    apply();

    BOOST_REQUIRE_EQUAL(proposal_service.removed_proposals.size(), 1);
    BOOST_CHECK_EQUAL(proposal_service.removed_proposals.front()._id, op.proposal_id);
}

SCORUM_TEST_CASE(throw_exception_if_proposal_expired)
{
    proposal_service.expired.push_back(proposal().id);

    BOOST_CHECK_THROW(apply(), fc::exception);

    SCORUM_CHECK_EXCEPTION(apply(), fc::exception, "Proposal '1' is expired.");
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE(test_get_quorum)

BOOST_AUTO_TEST_CASE(sixty_percent_from_ten_is_six_votes)
{
    BOOST_CHECK_EQUAL(6, scorum::chain::utils::get_quorum(10, 60 * SCORUM_1_PERCENT));
}

BOOST_AUTO_TEST_SUITE_END()

} // namespace tests
