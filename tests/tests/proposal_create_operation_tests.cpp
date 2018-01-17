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
#include <scorum/chain/global_property_object.hpp>

#include "defines.hpp"

using scorum::protocol::proposal_action;
using scorum::protocol::account_name_type;
using scorum::chain::proposal_object;
using scorum::chain::proposal_create_operation;
using scorum::chain::dynamic_global_property_object;

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
    struct proposal_t
    {
        account_name_type creator;
        fc::variant data;
        proposal_action action;
        fc::time_point_sec expiration;
        uint64_t quorum;
    };

public:
    proposal_service_mock()
        : _head_block_time(10)
    {
    }

    fc::time_point_sec head_block_time()
    {
        return _head_block_time;
    }

    void create(const account_name_type& name,
                const fc::variant& data,
                proposal_action action,
                const fc::time_point_sec& expiration,
                uint64_t quorum)
    {
        proposals.push_back({ name, data, action, expiration, quorum });
    }

    std::vector<proposal_t> proposals;

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

class properties_service_mock
{
public:
    properties_service_mock()
        : segment(bip::create_only, "TestSharedMemory", 65536)
        , allocator(segment.get_segment_manager())
        , properties([](const dynamic_global_property_object&) {}, allocator)
    {
    }

    const dynamic_global_property_object& get_dynamic_global_properties() const
    {
        return properties;
    }

    template <typename C> void modify(C&& c)
    {
        c(properties);
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
    scorum::chain::allocator<dynamic_global_property_object> allocator;
    dynamic_global_property_object properties;
};

typedef scorum::chain::proposal_create_evaluator_t<account_service_mock,
                                                   proposal_service_mock,
                                                   committee_service_mock,
                                                   properties_service_mock>
    proposal_create_evaluator_mocked;

class proposal_create_evaluator_fixture
{
public:
    proposal_create_evaluator_fixture()
        : lifetime_min(5)
        , lifetime_max(10)
        , evaluator(
              account_service, proposal_service, committee_service, properties_service, lifetime_min, lifetime_max)
    {
        committee_service.existent_accounts.insert("alice");
        committee_service.existent_accounts.insert("bob");
    }

    const uint32_t lifetime_min;
    const uint32_t lifetime_max;

    account_service_mock account_service;
    proposal_service_mock proposal_service;
    committee_service_mock committee_service;
    properties_service_mock properties_service;

    proposal_create_evaluator_mocked evaluator;
};

BOOST_FIXTURE_TEST_SUITE(proposal_create_evaluator_tests, proposal_create_evaluator_fixture)

SCORUM_TEST_CASE(throw_exception_if_lifetime_is_to_small)
{
    proposal_create_operation op;
    op.creator = "alice";
    op.data = "bob";
    op.lifetime_sec = lifetime_min - 1;

    auto test = [&](proposal_action action) {
        op.action = action;

        SCORUM_CHECK_EXCEPTION(evaluator.do_apply(op), fc::exception,
                               "Proposal life time is not in range of 5 - 10 seconds.");
    };

    test(proposal_action::invite);
    test(proposal_action::dropout);
    test(proposal_action::change_quorum);
    test(proposal_action::change_invite_quorum);
    test(proposal_action::change_dropout_quorum);
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

    auto test = [&](proposal_action action) {
        op.action = action;
        SCORUM_CHECK_EXCEPTION(evaluator.do_apply(op), fc::exception, "Account \"joe\" is not in committee.");
    };

    test(proposal_action::invite);
    test(proposal_action::dropout);
    test(proposal_action::change_quorum);
    test(proposal_action::change_invite_quorum);
    test(proposal_action::change_dropout_quorum);
}

SCORUM_TEST_CASE(create_one_invite_proposal)
{
    proposal_create_operation op;
    op.creator = "alice";
    op.data = "bob";
    op.lifetime_sec = lifetime_min + 1;
    op.action = proposal_action::invite;

    evaluator.do_apply(op);

    BOOST_CHECK_EQUAL(proposal_service.proposals[0].action, proposal_action::invite);
}

SCORUM_TEST_CASE(create_one_dropout_proposal)
{
    proposal_create_operation op;
    op.creator = "alice";
    op.data = "bob";
    op.lifetime_sec = lifetime_min + 1;
    op.action = proposal_action::dropout;

    evaluator.do_apply(op);

    BOOST_REQUIRE_EQUAL(proposal_service.proposals.size(), 1u);
    BOOST_CHECK_EQUAL(proposal_service.proposals[0].action, proposal_action::dropout);
}

SCORUM_TEST_CASE(create_one_change_invite_quorum_proposal)
{
    proposal_create_operation op;
    op.creator = "alice";
    op.lifetime_sec = lifetime_min + 1;
    op.action = proposal_action::change_invite_quorum;
    op.data = 60;

    evaluator.do_apply(op);

    BOOST_REQUIRE_EQUAL(proposal_service.proposals.size(), 1u);
    BOOST_CHECK_EQUAL(proposal_service.proposals[0].action, proposal_action::change_invite_quorum);
}

SCORUM_TEST_CASE(change_default_invite_quorum)
{
    const uint64_t expected_quorum = 80;
    BOOST_REQUIRE_NE(properties_service.get_dynamic_global_properties().change_quorum, expected_quorum);

    proposal_create_operation op;
    op.creator = "alice";
    op.data = "bob";
    op.lifetime_sec = lifetime_min + 1;
    op.action = proposal_action::invite;

    properties_service.modify([](dynamic_global_property_object& p) { p.invite_quorum = expected_quorum; });

    evaluator.do_apply(op);

    BOOST_REQUIRE_EQUAL(proposal_service.proposals.size(), 1u);

    BOOST_CHECK_EQUAL(proposal_service.proposals[0].quorum, expected_quorum);
}

SCORUM_TEST_CASE(change_default_dropout_quorum)
{
    const uint64_t expected_quorum = 80;
    BOOST_REQUIRE_NE(properties_service.get_dynamic_global_properties().change_quorum, expected_quorum);

    proposal_create_operation op;
    op.creator = "alice";
    op.data = "bob";
    op.lifetime_sec = lifetime_min + 1;
    op.action = proposal_action::dropout;

    properties_service.modify([](dynamic_global_property_object& p) { p.dropout_quorum = expected_quorum; });

    evaluator.do_apply(op);

    BOOST_REQUIRE_EQUAL(proposal_service.proposals.size(), 1u);

    BOOST_CHECK_EQUAL(proposal_service.proposals[0].quorum, expected_quorum);
}

SCORUM_TEST_CASE(change_default_change_quorum)
{
    const uint64_t expected_quorum = 80;
    BOOST_REQUIRE_NE(properties_service.get_dynamic_global_properties().change_quorum, expected_quorum);

    proposal_create_operation op;
    op.creator = "alice";
    op.data = 60;
    op.lifetime_sec = lifetime_min + 1;
    op.action = proposal_action::change_invite_quorum;

    properties_service.modify([](dynamic_global_property_object& p) { p.change_quorum = expected_quorum; });

    evaluator.do_apply(op);

    BOOST_REQUIRE_EQUAL(proposal_service.proposals.size(), 1u);

    BOOST_CHECK_EQUAL(proposal_service.proposals[0].quorum, expected_quorum);
}

SCORUM_TEST_CASE(create_one_change_dropout_quorum_proposal)
{
    proposal_create_operation op;
    op.creator = "alice";
    op.lifetime_sec = lifetime_min + 1;
    op.action = proposal_action::change_dropout_quorum;
    op.data = 60;

    evaluator.do_apply(op);

    BOOST_REQUIRE_EQUAL(proposal_service.proposals.size(), 1u);
    BOOST_CHECK_EQUAL(proposal_service.proposals[0].action, proposal_action::change_dropout_quorum);
}

SCORUM_TEST_CASE(create_one_change_quorum_proposal)
{
    proposal_create_operation op;
    op.creator = "alice";
    op.lifetime_sec = lifetime_min + 1;
    op.action = proposal_action::change_quorum;
    op.data = 60;

    evaluator.do_apply(op);

    BOOST_REQUIRE_EQUAL(proposal_service.proposals.size(), 1u);
    BOOST_CHECK_EQUAL(proposal_service.proposals[0].action, proposal_action::change_quorum);
}

SCORUM_TEST_CASE(expiration_time_is_sum_of_head_block_time_and_lifetime)
{
    proposal_create_operation op;
    op.creator = "alice";
    op.data = "bob";
    op.lifetime_sec = lifetime_min + 1;
    op.action = proposal_action::invite;

    evaluator.do_apply(op);

    BOOST_REQUIRE_EQUAL(proposal_service.proposals.size(), 1u);

    BOOST_CHECK(proposal_service.proposals[0].expiration == (proposal_service._head_block_time + op.lifetime_sec));
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
