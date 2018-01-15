#include <boost/test/unit_test.hpp>

#include <boost/interprocess/managed_shared_memory.hpp>
#include <boost/interprocess/allocators/allocator.hpp>

#include <scorum/protocol/exceptions.hpp>

#include <scorum/protocol/types.hpp>
#include <scorum/chain/dbs_proposal.hpp>

#include <scorum/chain/proposal_create_evaluator.hpp>
#include <scorum/chain/proposal_vote_evaluator.hpp>
#include <scorum/chain/proposal_object.hpp>
#include <scorum/chain/registration_objects.hpp>

#include "defines.hpp"

using scorum::protocol::proposal_action;
using scorum::protocol::account_name_type;
using scorum::chain::proposal_object;
using scorum::chain::proposal_create_operation;
using scorum::chain::registration_pool_object;

namespace bip = boost::interprocess;

class account_service_mock
{
public:
    void check_account_existence(const account_name_type&)
    {
    }
};

class proposal_service_mock
{
public:
    proposal_service_mock()
        : _head_block_time(10)
    {
    }

    fc::time_point_sec head_block_time()
    {
        return _head_block_time;
    }

    void create(const account_name_type&,
                const fc::variant&,
                proposal_action,
                const fc::time_point_sec& expiration,
                uint64_t quorum)
    {
        ++proposals_created;
        this->expiration = expiration;
        this->quorum = quorum;
    }

    uint32_t proposals_created = 0;
    fc::time_point_sec expiration;
    uint64_t quorum;
    const fc::time_point_sec _head_block_time;
};

class committee_service_mock
{
public:
    bool member_exists(const account_name_type& account) const
    {
        return existent_accounts.count(account) == 1 ? true : false;
    }

    uint64_t get_quorum(uint64_t)
    {
        return quorum_votes;
    }

    std::set<account_name_type> existent_accounts;
    uint64_t quorum_votes = 1;
};

class pool_service_mock
{
public:
    pool_service_mock()
        : segment(bip::create_only, "TestSharedMemory", 65536)
        , allocator(segment.get_segment_manager())
        , pool([](const registration_pool_object&) {}, allocator)
    {
    }

    const registration_pool_object& get_pool() const
    {
        return pool;
    }

private:
    struct shm_remove
    {
        shm_remove()
        {
            bip::shared_memory_object::remove("TestSharedMemory");
        }
        ~shm_remove()
        {
            bip::shared_memory_object::remove("TestSharedMemory");
        }
    } remover;

    bip::managed_shared_memory segment;
    scorum::chain::allocator<registration_pool_object> allocator;
    registration_pool_object pool;
};

typedef scorum::chain::
    proposal_create_evaluator_t<account_service_mock, proposal_service_mock, committee_service_mock, pool_service_mock>
        proposal_create_evaluator_mocked;

class proposal_create_evaluator_fixture
{
public:
    proposal_create_evaluator_fixture()
        : lifetime_min(5)
        , lifetime_max(10)
        , evaluator(account_service, proposal_service, committee_service, pool_service, lifetime_min, lifetime_max)
    {
        committee_service.existent_accounts.insert("alice");
        committee_service.existent_accounts.insert("bob");
    }

    const uint32_t lifetime_min;
    const uint32_t lifetime_max;

    account_service_mock account_service;
    proposal_service_mock proposal_service;
    committee_service_mock committee_service;
    pool_service_mock pool_service;

    proposal_create_evaluator_mocked evaluator;
};

BOOST_FIXTURE_TEST_SUITE(proposal_create_evaluator_tests, proposal_create_evaluator_fixture)

SCORUM_TEST_CASE(throw_exception_if_lifetime_is_to_small)
{
    proposal_create_operation op;
    op.creator = "alice";
    op.data = "bob";
    op.lifetime_sec = lifetime_min - 1;
    op.action = proposal_action::dropout;

    SCORUM_CHECK_EXCEPTION(evaluator.do_apply(op), fc::exception,
                           "Proposal life time is not in range of 5 - 10 seconds.");
}

SCORUM_TEST_CASE(throw_exception_if_lifetime_is_to_big)
{
    proposal_create_operation op;
    op.creator = "alice";
    op.data = "bob";
    op.lifetime_sec = lifetime_max + 1;

    SCORUM_CHECK_EXCEPTION(evaluator.do_apply(op), fc::exception,
                           "Proposal life time is not in range of 5 - 10 seconds.");
}

SCORUM_TEST_CASE(throw_when_creator_is_not_in_committee)
{
    proposal_create_operation op;
    op.creator = "joe";
    op.data = "bob";
    op.lifetime_sec = lifetime_min + 1;
    op.action = proposal_action::invite;

    SCORUM_CHECK_EXCEPTION(evaluator.do_apply(op), fc::exception, "Account \"joe\" is not in committee.");
}

SCORUM_TEST_CASE(create_one_invite_proposal)
{
    proposal_create_operation op;
    op.creator = "alice";
    op.data = "bob";
    op.lifetime_sec = lifetime_min + 1;
    op.action = proposal_action::invite;

    evaluator.do_apply(op);

    BOOST_CHECK_EQUAL(proposal_service.proposals_created, 1);
}

SCORUM_TEST_CASE(create_one_dropout_proposal)
{
    proposal_create_operation op;
    op.creator = "alice";
    op.data = "bob";
    op.lifetime_sec = lifetime_min + 1;
    op.action = proposal_action::invite;

    evaluator.do_apply(op);

    BOOST_REQUIRE_EQUAL(proposal_service.proposals_created, 1);
}

SCORUM_TEST_CASE(expiration_time_is_sum_of_head_block_time_and_lifetime)
{
    proposal_create_operation op;
    op.creator = "alice";
    op.data = "bob";
    op.lifetime_sec = lifetime_min + 1;
    op.action = proposal_action::invite;

    evaluator.do_apply(op);

    BOOST_CHECK(proposal_service.expiration == (proposal_service._head_block_time + op.lifetime_sec));
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
    op.action = proposal_action::invite;

    SCORUM_CHECK_EXCEPTION(op.validate(), fc::exception, "Account name  is invalid");
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
}

BOOST_AUTO_TEST_SUITE_END()
