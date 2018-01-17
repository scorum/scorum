#ifdef IS_TEST_NET

#include <boost/test/unit_test.hpp>

#include "database_fixture.hpp"
#include "defines.hpp"

#include <scorum/chain/dbs_registration_committee.hpp>
#include <scorum/chain/dbs_proposal.hpp>
#include <scorum/chain/dbs_dynamic_global_property.hpp>

#include "actor.hpp"
#include "genesis.hpp"

namespace scorum {
namespace chain {

class proposal_fixture
{
public:
    using chain_type = timed_blocks_database_fixture;
    typedef std::shared_ptr<chain_type> chain_type_ptr;

    using committee_members = dbs_registration_committee::registration_committee_member_refs_type;

    class actor_actions
    {
    public:
        actor_actions(proposal_fixture& fix, const Actor& a)
            : f(fix)
            , actor(a)
        {
        }

        void transfer_to_vest(const Actor& a, asset amount)
        {
            f.chain().transfer_to_vest(actor.name, a.name, amount);
        }

        void give_power(const Actor& a)
        {
            transfer_to_vest(a, ASSET("0.100 SCR"));
        }

        proposal_id_type create_committee_proposal(const Actor& a, proposal_action action)
        {
            return create_proposal(action, fc::variant(a.name).as_string());
        }

        proposal_id_type create_quorum_change_proposal(uint64_t q, proposal_action action)
        {
            return create_proposal(action, fc::variant(SCORUM_PERCENT(q)).as_uint64());
        }

        proposal_id_type create_proposal(proposal_action action, const fc::variant& data)
        {
            proposal_create_operation op;
            op.creator = actor.name;
            op.data = data;
            op.action = action;
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
            auto proposal = create_committee_proposal(invitee, proposal_action::invite);
            return proposal._id;
        }

        uint64_t dropout_from_committee(const Actor& invitee)
        {
            auto proposal = create_committee_proposal(invitee, proposal_action::dropout);
            return proposal._id;
        }

        uint64_t change_invite_quorum(uint64_t quorum)
        {
            auto proposal = create_quorum_change_proposal(quorum, proposal_action::change_invite_quorum);
            return proposal._id;
        }

        uint64_t change_dropout_quorum(uint64_t quorum)
        {
            auto proposal = create_quorum_change_proposal(quorum, proposal_action::change_dropout_quorum);
            return proposal._id;
        }

        uint64_t change_quorum(uint64_t quorum)
        {
            auto proposal = create_quorum_change_proposal(quorum, proposal_action::change_quorum);
            return proposal._id;
        }

        void vote_for(uint64_t proposal)
        {
            proposal_vote_operation op;
            op.voting_account = actor.name;
            op.proposal_id = proposal;

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
        proposal_fixture& f;
        const Actor& actor;
    };

    proposal_fixture()
    {
    }

    chain_type& chain()
    {
        if (!_chain)
        {
            _chain = chain_type_ptr(new chain_type(genesis));
        }

        return *_chain;
    }

    proposal_id_type get_last_proposal_id()
    {
        dbs_proposal& proposal_service = chain().db.obtain_service<dbs_proposal>();
        std::vector<proposal_object::cref_type> proposals = proposal_service.get_proposals();

        BOOST_REQUIRE_GT(proposals.size(), static_cast<size_t>(0));

        return proposals[proposals.size() - 1].get().id;
    }

    committee_members get_committee_members()
    {
        dbs_registration_committee& committee_service = chain().db.obtain_service<dbs_registration_committee>();

        return committee_service.get_committee();
    }

    bool is_committee_member(const Actor& a)
    {
        dbs_registration_committee& committee_service = chain().db.obtain_service<dbs_registration_committee>();
        committee_members members = committee_service.get_committee();

        for (const registration_committee_member_object& member : members)
        {
            if (member.account == a.name)
                return true;
        }

        return false;
    }

    fc::optional<proposal_object::cref_type> get_proposal(int64_t id)
    {
        dbs_proposal& proposal_service = chain().db.obtain_service<dbs_proposal>();
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
        dbs_dynamic_global_property& prop_service = chain().db.obtain_service<dbs_dynamic_global_property>();

        auto& prop = prop_service.get_dynamic_global_properties();

        return prop.invite_quorum;
    }

    uint64_t get_dropout_quorum()
    {
        dbs_dynamic_global_property& prop_service = chain().db.obtain_service<dbs_dynamic_global_property>();

        auto& prop = prop_service.get_dynamic_global_properties();

        return prop.dropout_quorum;
    }

    uint64_t get_change_quorum()
    {
        dbs_dynamic_global_property& prop_service = chain().db.obtain_service<dbs_dynamic_global_property>();

        auto& prop = prop_service.get_dynamic_global_properties();

        return prop.change_quorum;
    }

    actor_actions actor(const Actor& a)
    {
        actor_actions c(*this, a);
        return c;
    }

    genesis_state_type genesis;

private:
    chain_type_ptr _chain;
};

BOOST_FIXTURE_TEST_SUITE(proposal_operation_tests, proposal_fixture)

// clang-format off
SCORUM_TEST_CASE(proposal)
{
    Actor initdelegate("initdelegate");
    Actor alice("alice");
    Actor bob("bob");
    Actor jim("jim");
    Actor joe("joe");
    Actor hue("hue");
    Actor liz("liz");

    genesis = Genesis::create()
                            .accounts(bob, jim, joe, hue, liz)
                            .committee(alice)
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

    {
        fc::time_point_sec expected_expiration = chain().db.head_block_time() + SCORUM_PROPOSAL_LIFETIME_MIN_SECONDS;

        auto p = get_proposal(bob_invitation);

        BOOST_REQUIRE(p.valid());
        BOOST_CHECK(expected_expiration == p->get().expiration);
    }

    {
        actor(alice).vote_for(jim_invitation);
        BOOST_CHECK_EQUAL(2u, get_committee_members().size());
        BOOST_CHECK(is_committee_member(jim) == true);
    }

    {
        actor(alice).vote_for(joe_invitation);
        BOOST_CHECK_EQUAL(3u, get_committee_members().size());
        BOOST_CHECK(is_committee_member(joe) == true);
    }

    {
        actor(alice).vote_for(hue_invitation);
        BOOST_CHECK_EQUAL(4u, get_committee_members().size());
        BOOST_CHECK(is_committee_member(hue) == true);
    }

    {
        actor(alice).vote_for(liz_invitation);

        // not enough votes to add liz
        BOOST_CHECK_EQUAL(4u, get_committee_members().size());

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

        // We droped one member, amount of needed votes reduced and 'drop_hue' executed automatically
        BOOST_CHECK_EQUAL(false, is_committee_member(hue));

        BOOST_CHECK_EQUAL(3u, get_committee_members().size());
    }

    // check that member removed from voted_accounts after committee member removing
    {
        auto p = get_proposal(bob_invitation);

        BOOST_REQUIRE(p.valid());

        BOOST_CHECK_EQUAL(p->get().voted_accounts.size(), 0u);
    }

    // check default value
    {
        BOOST_CHECK_EQUAL(SCORUM_COMMITTEE_QUORUM_PERCENT, get_invite_quorum());
        BOOST_CHECK_EQUAL(SCORUM_COMMITTEE_QUORUM_PERCENT, get_dropout_quorum());
        BOOST_CHECK_EQUAL(SCORUM_COMMITTEE_QUORUM_PERCENT, get_change_quorum());
    }

    {
        auto proposal = actor(alice).change_invite_quorum(50);
        actor(alice).vote_for(proposal);

        proposal = actor(alice).change_dropout_quorum(51);
        actor(alice).vote_for(proposal);

        proposal = actor(alice).change_quorum(100);
        actor(alice).vote_for(proposal);

        BOOST_CHECK_EQUAL(SCORUM_PERCENT(50), get_invite_quorum());
        BOOST_CHECK_EQUAL(SCORUM_PERCENT(51), get_dropout_quorum());
        BOOST_CHECK_EQUAL(SCORUM_PERCENT(100), get_change_quorum());
    }

} // clang-format on

BOOST_AUTO_TEST_SUITE_END()

} // namespace scorum
} // namespace chain

#endif
