#include <boost/test/unit_test.hpp>

#include "database_trx_integration.hpp"
#include "defines.hpp"

#include <scorum/protocol/proposal_operations.hpp>

#include <scorum/chain/services/development_committee.hpp>
#include <scorum/chain/services/registration_committee.hpp>
#include <scorum/chain/services/proposal.hpp>
#include <scorum/chain/services/dynamic_global_property.hpp>

#include <scorum/chain/schema/proposal_object.hpp>
#include <scorum/chain/schema/dev_committee_object.hpp>
#include <scorum/chain/schema/registration_objects.hpp>

#include "actor.hpp"
#include "genesis.hpp"

namespace committee_proposal_tests {

using namespace database_fixture;

namespace protocol = scorum::protocol;

using chain_type = database_trx_integration_fixture;
using chain_type_ptr = std::shared_ptr<chain_type>;

template <typename AddMemberOperation,
          typename ExcludeMemberOperation,
          typename ChangeQuorumOperation,
          typename CommitteeMemberObject,
          typename DbsCommittee>
class fixture
{
public:
    using Fixture = fixture<AddMemberOperation,
                            ExcludeMemberOperation,
                            ChangeQuorumOperation,
                            CommitteeMemberObject,
                            DbsCommittee>;

    using proposal_add_member_operation = AddMemberOperation;
    using proposal_exclude_member_operation = ExcludeMemberOperation;
    using proposal_change_quorum_operation = ChangeQuorumOperation;
    using committee_member_object = CommitteeMemberObject;
    using dbs_committee = DbsCommittee;

    using committee_members = typename dbs_committee::committee_members_cref_type;

    class actor_actions
    {
    public:
        actor_actions(Fixture& fix, const Actor& a)
            : f(fix)
            , actor(a)
        {
        }

        void transfer_to_scorumpower(const Actor& a, asset amount)
        {
            f.chain().transfer_to_scorumpower(actor.name, a.name, amount);
        }

        void give_power(const Actor& a)
        {
            transfer_to_scorumpower(a, ASSET_SCR(100));
        }

        proposal_id_type create_proposal(const protocol::proposal_operation& operation)
        {
            proposal_create_operation op;
            op.creator = actor.name;
            op.operation = operation;
            op.lifetime_sec = SCORUM_PROPOSAL_LIFETIME_MIN_SECONDS;

            op.validate();

            signed_transaction tx;
            tx.operations.push_back(op);
            tx.set_expiration(f.chain().db.head_block_time() + SCORUM_MAX_TIME_UNTIL_EXPIRATION);

            f.chain().sign(tx, actor.private_key);
            tx.validate();

            f.chain().db.push_transaction(tx, 0);

            f.chain().validate_database();

            return f.get_last_proposal_id();
        }

        uint64_t invite_in_to_committee(const Actor& invitee)
        {
            proposal_add_member_operation operation;
            operation.account_name = invitee.name;

            auto proposal = create_proposal(operation);
            return proposal._id;
        }

        uint64_t dropout_from_committee(const Actor& invitee)
        {
            proposal_exclude_member_operation operation;
            operation.account_name = invitee.name;

            auto proposal = create_proposal(operation);
            return proposal._id;
        }

        uint64_t change_invite_quorum(uint64_t quorum)
        {
            proposal_change_quorum_operation operation;
            operation.quorum = quorum;
            operation.committee_quorum = protocol::add_member_quorum;

            auto proposal = create_proposal(operation);
            return proposal._id;
        }

        uint64_t change_dropout_quorum(uint64_t quorum)
        {
            proposal_change_quorum_operation operation;
            operation.quorum = quorum;
            operation.committee_quorum = protocol::exclude_member_quorum;

            auto proposal = create_proposal(operation);
            return proposal._id;
        }

        uint64_t change_quorum(uint64_t quorum)
        {
            proposal_change_quorum_operation operation;
            operation.quorum = quorum;
            operation.committee_quorum = protocol::base_quorum;

            auto proposal = create_proposal(operation);
            return proposal._id;
        }

        void vote_for(uint64_t proposal_id)
        {
            proposal_vote_operation op;
            op.voting_account = actor.name;
            op.proposal_id = proposal_id;

            op.validate();

            signed_transaction tx;
            tx.operations.push_back(op);
            tx.set_expiration(f.chain().db.head_block_time() + SCORUM_MAX_TIME_UNTIL_EXPIRATION);

            f.chain().sign(tx, actor.private_key);
            tx.validate();

            f.chain().db.push_transaction(tx, 0);

            f.chain().validate_database();
        }

    private:
        Fixture& f;
        const Actor& actor;
    };

    fixture()
    {
    }

    chain_type& chain()
    {
        if (!_chain)
        {
            _chain = chain_type_ptr(new chain_type());
            _chain->open_database(genesis);
        }

        return *_chain;
    }

    proposal_id_type get_last_proposal_id()
    {
        auto& proposal_service = _chain->db.obtain_service<dbs_proposal>();
        std::vector<proposal_object::cref_type> proposals = proposal_service.get_proposals();

        BOOST_REQUIRE_GT(proposals.size(), static_cast<size_t>(0));

        return proposals[proposals.size() - 1].get().id;
    }

    committee_members get_committee_members()
    {
        dbs_committee& committee_service = _chain->db.obtain_service<dbs_committee>();

        return committee_service.get_committee();
    }

    bool is_committee_member(const Actor& a)
    {
        dbs_committee& committee_service = _chain->db.obtain_service<dbs_committee>();
        committee_members members = committee_service.get_committee();

        for (const committee_member_object& member : members)
        {
            if (member.account == a.name)
                return true;
        }

        return false;
    }

    fc::optional<proposal_object::cref_type> get_proposal(int64_t id)
    {
        dbs_proposal& proposal_service = _chain->db.obtain_service<dbs_proposal>();
        std::vector<proposal_object::cref_type> proposals = proposal_service.get_proposals();

        for (proposal_object::cref_type p : proposals)
        {
            if (p.get().id._id == id)
            {
                return fc::optional<proposal_object::cref_type>(p);
            }
        }

        return fc::optional<proposal_object::cref_type>();
    }

    uint64_t get_invite_quorum()
    {
        auto& service = _chain->db.obtain_service<dbs_committee>();
        return service.get_add_member_quorum();
    }

    uint64_t get_dropout_quorum()
    {
        auto& service = _chain->db.obtain_service<dbs_committee>();
        return service.get_exclude_member_quorum();
    }

    uint64_t get_change_quorum()
    {
        auto& service = _chain->db.obtain_service<dbs_committee>();
        return service.get_base_quorum();
    }

    actor_actions actor(const Actor& a)
    {
        actor_actions c(*this, a);
        return c;
    }

    genesis_state_type genesis;

    void run_test()
    {
        Actor initdelegate(TEST_INIT_DELEGATE_NAME);
        initdelegate.scorum(TEST_ACCOUNTS_INITIAL_SUPPLY);

        Actor alice("alice");
        Actor bob("bob");
        Actor jim("jim");
        Actor joe("joe");
        Actor hue("hue");
        Actor liz("liz");

        static const asset registration_bonus = ASSET_SCR(100);
        registration_stage single_stage{ 1u, 1u, 100u };
        genesis = Genesis::create()
                      .accounts_supply(TEST_ACCOUNTS_INITIAL_SUPPLY)
                      .rewards_supply(TEST_REWARD_INITIAL_SUPPLY)
                      .witnesses(initdelegate)
                      .accounts(bob, jim, joe, hue, liz)
                      .registration_supply(registration_bonus * 100)
                      .registration_bonus(registration_bonus)
                      .registration_schedule(single_stage)
                      .committee(alice)
                      .dev_committee(alice)
                      .generate();

        chain().generate_block();

        actor(initdelegate).give_power(alice);
        actor(initdelegate).give_power(jim);
        actor(initdelegate).give_power(joe);
        actor(initdelegate).give_power(hue);
        actor(initdelegate).give_power(liz);

        // setup committee
        auto jim_invitation = actor(alice).invite_in_to_committee(jim);
        auto joe_invitation = actor(alice).invite_in_to_committee(joe);
        auto hue_invitation = actor(alice).invite_in_to_committee(hue);
        auto liz_invitation = actor(alice).invite_in_to_committee(liz);
        auto bob_invitation = actor(alice).invite_in_to_committee(bob);

        BOOST_CHECK_EQUAL(1u, get_committee_members().size());

        {
            fc::time_point_sec expected_expiration
                = chain().db.head_block_time() + SCORUM_PROPOSAL_LIFETIME_MIN_SECONDS;

            auto p = get_proposal(bob_invitation);

            BOOST_REQUIRE(p.valid());
            BOOST_CHECK(expected_expiration == p->get().expiration);
        }

        {
            actor(alice).vote_for(jim_invitation);
            BOOST_CHECK_EQUAL(2u, get_committee_members().size());
            BOOST_CHECK_EQUAL(true, is_committee_member(jim));
        }

        {
            actor(alice).vote_for(joe_invitation);
            actor(jim).vote_for(joe_invitation);
            BOOST_CHECK_EQUAL(3u, get_committee_members().size());
            BOOST_CHECK_EQUAL(true, is_committee_member(joe));
        }

        {
            actor(alice).vote_for(hue_invitation);
            actor(jim).vote_for(hue_invitation);
            BOOST_CHECK_EQUAL(4u, get_committee_members().size());
            BOOST_CHECK_EQUAL(true, is_committee_member(hue));
        }

        {
            actor(alice).vote_for(liz_invitation);
            actor(joe).vote_for(liz_invitation);

            // not enough votes to add liz
            BOOST_CHECK_EQUAL(4u, get_committee_members().size());

            // needs 3 votes to add liz
            actor(jim).vote_for(liz_invitation);
            BOOST_CHECK_EQUAL(5u, get_committee_members().size());
        }
        // end setup committee

        // check that voted_accounts list changed
        {
            actor(joe).vote_for(bob_invitation);
            auto p = get_proposal(bob_invitation);

            BOOST_REQUIRE(p.valid());

            BOOST_CHECK_EQUAL(1u, p->get().voted_accounts.size());
            BOOST_CHECK_EQUAL(1u, p->get().voted_accounts.count("joe"));
        }

        // check that drop_hue proposal executed with drop_joe because of committee members size change
        {
            auto drop_hue = actor(alice).dropout_from_committee(hue);
            actor(alice).vote_for(drop_hue);
            actor(jim).vote_for(drop_hue);

            // not enoght votes to drop hue
            BOOST_CHECK_EQUAL(true, is_committee_member(hue));

            auto drop_joe = actor(alice).dropout_from_committee(joe);
            actor(alice).vote_for(drop_joe);
            actor(jim).vote_for(drop_joe);
            actor(liz).vote_for(drop_joe);

            // three votes is enoght to dropout joe
            BOOST_CHECK_EQUAL(false, is_committee_member(joe));

            // We droped one member, amount of needed votes reduced but not enough to 'drop_hue' executed automatically
            BOOST_CHECK_EQUAL(true, is_committee_member(hue));

            BOOST_CHECK_EQUAL(4u, get_committee_members().size());

            // Lets drop out one more committee member
            auto drop_liz = actor(alice).dropout_from_committee(liz);
            actor(alice).vote_for(drop_liz);
            actor(jim).vote_for(drop_liz);
            actor(hue).vote_for(drop_liz);

            // three votes is enoght to dropout liz
            BOOST_CHECK_EQUAL(false, is_committee_member(liz));

            // We droped one member, amount of needed votes reduced and 'drop_hue' executed automatically
            BOOST_CHECK_EQUAL(false, is_committee_member(hue));

            BOOST_CHECK_EQUAL(2u, get_committee_members().size());
        }

        // check that member removed from voted_accounts after committee member removing
        {
            auto p = get_proposal(bob_invitation);

            BOOST_REQUIRE(p.valid());

            BOOST_CHECK_EQUAL(0u, p->get().voted_accounts.size());
        }

        // check default value
        {
            BOOST_CHECK_EQUAL(SCORUM_COMMITTEE_ADD_EXCLUDE_QUORUM_PERCENT, get_invite_quorum());
            BOOST_CHECK_EQUAL(SCORUM_COMMITTEE_ADD_EXCLUDE_QUORUM_PERCENT, get_dropout_quorum());
            BOOST_CHECK_EQUAL(SCORUM_COMMITTEE_QUORUM_PERCENT, get_change_quorum());
        }

        {
            auto proposal = actor(alice).change_invite_quorum(50);
            actor(alice).vote_for(proposal);
            actor(jim).vote_for(proposal);

            proposal = actor(alice).change_dropout_quorum(51);
            actor(alice).vote_for(proposal);
            actor(jim).vote_for(proposal);

            proposal = actor(alice).change_quorum(100);
            actor(alice).vote_for(proposal);
            actor(jim).vote_for(proposal);

            BOOST_CHECK_EQUAL(50u, get_invite_quorum());
            BOOST_CHECK_EQUAL(51u, get_dropout_quorum());
            BOOST_CHECK_EQUAL(100u, get_change_quorum());
        }
    }

private:
    chain_type_ptr _chain;
};

BOOST_AUTO_TEST_SUITE(committee_proposals_test)

using registration_committee_fixture = fixture<registration_committee_add_member_operation,
                                               registration_committee_exclude_member_operation,
                                               registration_committee_change_quorum_operation,
                                               registration_committee_member_object,
                                               dbs_registration_committee>;

using development_committee_fixture = fixture<development_committee_add_member_operation,
                                              development_committee_exclude_member_operation,
                                              development_committee_change_quorum_operation,
                                              dev_committee_member_object,
                                              dbs_development_committee>;

SCORUM_TEST_CASE(registration_committee_operations_test)
{
    registration_committee_fixture().run_test();
}

SCORUM_TEST_CASE(development_committee_operations_test)
{
    development_committee_fixture().run_test();
}

BOOST_AUTO_TEST_SUITE_END()

class dev_committee_fixture : public database_fixture::database_trx_integration_fixture
{
public:
    dev_committee_fixture()
    {
        initdelegate.scorum(TEST_ACCOUNTS_INITIAL_SUPPLY);
        alice.scorum(TEST_ACCOUNTS_INITIAL_SUPPLY);
        bob.scorum(TEST_ACCOUNTS_INITIAL_SUPPLY);

        auto genesis = Genesis::create()
                           .accounts_supply(TEST_ACCOUNTS_INITIAL_SUPPLY * 3)
                           .rewards_supply(TEST_REWARD_INITIAL_SUPPLY)
                           .witnesses(initdelegate)
                           .accounts(alice, bob)
                           .dev_committee(alice)
                           .development_scr_supply(asset(1e5, SCORUM_SYMBOL))
                           .development_sp_supply(asset(1e5, SP_SYMBOL))
                           .generate();
        open_database(genesis);

        actor(initdelegate).give_sp(alice, 1e5);
        actor(initdelegate).give_sp(bob, 1e5);
    }

    Actor alice = "alice";
    Actor bob = "bob";
};

struct dev_committee_operation_visitor
{
    using result_type = void;

    void operator()(const proposal_virtual_operation& op)
    {
        op.proposal_op.weak_visit([&](const development_committee_transfer_operation& op) {
            transfer_ops.emplace_back(op.to_account, op.amount);
        });
    }

    void operator()(const devpool_to_devpool_vesting_withdraw_operation& op)
    {
        withdraw_ops.emplace_back(op.withdrawn);
    }

    template <typename Op> void operator()(Op&&) const
    {
    } /// ignore all other ops

    std::vector<std::tuple<account_name_type, asset>> transfer_ops;
    std::vector<asset> withdraw_ops;
};

BOOST_FIXTURE_TEST_SUITE(dev_committee_tests, dev_committee_fixture)

SCORUM_TEST_CASE(dev_committee_transfer_operation_circulating_capital_should_increase)
{
    unsigned transfer_amount = 1e5;

    development_committee_transfer_operation proposal_inner_op;
    proposal_inner_op.amount = asset(transfer_amount, SCORUM_SYMBOL);
    proposal_inner_op.to_account = bob.name;

    proposal_create_operation proposal_create_op;
    proposal_create_op.creator = alice.name;
    proposal_create_op.lifetime_sec = SCORUM_PROPOSAL_LIFETIME_MIN_SECONDS;
    proposal_create_op.operation = proposal_inner_op;

    push_operation(proposal_create_op, initdelegate.private_key);

    const auto& dpo = db.dynamic_global_property_service().get();
    auto circulating_capital_before = dpo.circulating_capital;

    proposal_vote_operation proposal_vote_op;
    proposal_vote_op.voting_account = alice.name;
    proposal_vote_op.proposal_id = 0;

    push_operation(proposal_vote_op, initdelegate.private_key);

    auto circulating_capital_after = dpo.circulating_capital;

    auto active_sp_holder_reward = 10;
    BOOST_REQUIRE_EQUAL(circulating_capital_before + transfer_amount + active_sp_holder_reward,
                        circulating_capital_after);
}

SCORUM_TEST_CASE(dev_committee_withdraw_operation_circulating_capital_should_not_increase)
{
    unsigned transfer_amount = 1e5;

    development_committee_withdraw_vesting_operation proposal_inner_op;
    proposal_inner_op.vesting_shares = asset(transfer_amount, SP_SYMBOL);

    proposal_create_operation proposal_create_op;
    proposal_create_op.creator = alice.name;
    proposal_create_op.lifetime_sec = SCORUM_PROPOSAL_LIFETIME_MIN_SECONDS;
    proposal_create_op.operation = proposal_inner_op;

    push_operation(proposal_create_op, initdelegate.private_key);

    const auto& dpo = db.dynamic_global_property_service().get();
    auto circulating_capital_before = dpo.circulating_capital;

    proposal_vote_operation proposal_vote_op;
    proposal_vote_op.voting_account = alice.name;
    proposal_vote_op.proposal_id = 0;

    push_operation(proposal_vote_op, initdelegate.private_key, false);
    generate_blocks(db.head_block_time() + fc::seconds(SCORUM_VESTING_WITHDRAW_INTERVAL_SECONDS));

    auto circulating_capital_after = dpo.circulating_capital;

    auto active_sp_holder_reward = 10;
    // circulating capital do not increate on 'transfer_amount' cuz withdraw was from devpool to devpool
    BOOST_REQUIRE_EQUAL(circulating_capital_before + 2 * active_sp_holder_reward, circulating_capital_after);
}

SCORUM_TEST_CASE(dev_committee_withdraw_operation_not_enough_sp)
{
    auto transfer = genesis_state.development_sp_supply + 1;

    development_committee_withdraw_vesting_operation proposal_inner_op;
    proposal_inner_op.vesting_shares = transfer;

    proposal_create_operation proposal_create_op;
    proposal_create_op.creator = alice.name;
    proposal_create_op.lifetime_sec = SCORUM_PROPOSAL_LIFETIME_MIN_SECONDS;
    proposal_create_op.operation = proposal_inner_op;

    push_operation(proposal_create_op, initdelegate.private_key);

    proposal_vote_operation proposal_vote_op;
    proposal_vote_op.voting_account = alice.name;
    proposal_vote_op.proposal_id = 0;

    BOOST_REQUIRE_THROW(push_operation(proposal_vote_op, initdelegate.private_key, false), fc::assert_exception);
}

SCORUM_TEST_CASE(dev_committee_transfer_virtual_op_should_be_raised)
{
    dev_committee_operation_visitor visitor;

    db.post_apply_operation.connect([&](const operation_notification& note) { note.op.visit(visitor); });

    unsigned transfer_amount = 1e5;

    development_committee_transfer_operation proposal_inner_op;
    proposal_inner_op.amount = asset(transfer_amount, SCORUM_SYMBOL);
    proposal_inner_op.to_account = bob.name;

    proposal_create_operation proposal_create_op;
    proposal_create_op.creator = alice.name;
    proposal_create_op.lifetime_sec = SCORUM_PROPOSAL_LIFETIME_MIN_SECONDS;
    proposal_create_op.operation = proposal_inner_op;

    BOOST_REQUIRE_EQUAL(visitor.transfer_ops.size(), 0u);

    push_operation(proposal_create_op, initdelegate.private_key);

    BOOST_REQUIRE_EQUAL(visitor.transfer_ops.size(), 0u);

    proposal_vote_operation proposal_vote_op;
    proposal_vote_op.voting_account = alice.name;
    proposal_vote_op.proposal_id = 0;

    push_operation(proposal_vote_op, initdelegate.private_key);

    BOOST_REQUIRE_EQUAL(visitor.transfer_ops.size(), 3u); // 1-push_tx; 2-gen_block(check for except); 3-apply_block
    BOOST_REQUIRE_EQUAL(std::get<0>(visitor.transfer_ops[0]), bob.name);
    BOOST_REQUIRE_EQUAL(std::get<1>(visitor.transfer_ops[0]).amount, transfer_amount);
}

SCORUM_TEST_CASE(dev_committee_withdraw_virtual_op_should_be_raised)
{
    dev_committee_operation_visitor visitor;

    db.post_apply_operation.connect([&](const operation_notification& note) { note.op.visit(visitor); });

    unsigned withdraw_amount = 1e4;

    development_committee_withdraw_vesting_operation proposal_inner_op;
    proposal_inner_op.vesting_shares = asset(withdraw_amount, SP_SYMBOL);

    proposal_create_operation proposal_create_op;
    proposal_create_op.creator = alice.name;
    proposal_create_op.lifetime_sec = SCORUM_PROPOSAL_LIFETIME_MIN_SECONDS;
    proposal_create_op.operation = proposal_inner_op;

    BOOST_REQUIRE_EQUAL(visitor.withdraw_ops.size(), 0u);

    push_operation(proposal_create_op, initdelegate.private_key);

    BOOST_REQUIRE_EQUAL(visitor.withdraw_ops.size(), 0u);

    generate_blocks(db.head_block_time() + fc::seconds(SCORUM_VESTING_WITHDRAW_INTERVAL_SECONDS));

    BOOST_REQUIRE_EQUAL(visitor.withdraw_ops.size(), 0u);

    proposal_vote_operation proposal_vote_op;
    proposal_vote_op.voting_account = alice.name;
    proposal_vote_op.proposal_id = 0;

    push_operation(proposal_vote_op, initdelegate.private_key, false);
    generate_blocks(db.head_block_time() + fc::seconds(SCORUM_VESTING_WITHDRAW_INTERVAL_SECONDS));

    BOOST_REQUIRE_EQUAL(visitor.withdraw_ops.size(), 1u);
    BOOST_REQUIRE_EQUAL(visitor.withdraw_ops[0].amount, withdraw_amount / SCORUM_VESTING_WITHDRAW_INTERVALS);
}

BOOST_AUTO_TEST_SUITE_END()

} // namespace proposal_tests
