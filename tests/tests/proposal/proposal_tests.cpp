#ifdef IS_TEST_NET

#include <boost/test/unit_test.hpp>

#include "database_fixture.hpp"
#include "defines.hpp"

#include <scorum/chain/dbs_registration_committee.hpp>
#include <scorum/chain/dbs_proposal.hpp>

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

        proposal_id_type create_proposal(const Actor& invitee, proposal_action action)
        {
            proposal_create_operation op;
            op.creator = actor.name;
            op.committee_member = invitee.name;
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

            return f.get_proposal_id(invitee.name);
        }

        uint64_t invite_in_to_committee(const Actor invitee)
        {
            auto proposal = create_proposal(invitee, proposal_action::invite);
            return proposal._id;
        }

        uint64_t dropout_from_committee(const Actor invitee)
        {
            auto proposal = create_proposal(invitee, proposal_action::dropout);
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

    proposal_id_type get_proposal_id(const std::string& name)
    {
        dbs_proposal& proposal_service = chain().db.obtain_service<dbs_proposal>();
        std::vector<proposal_object::ref_type> proposals = proposal_service.get_proposals();

        for (const proposal_object& p : proposals)
        {
            if (p.data.as_string() == name)
            {
                return p.id;
            }
        }

        throw fc::exception(fc::exception_code::unspecified_exception_code, "", "no such proposal");
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

    fc::optional<proposal_object::ref_type> get_proposal(int64_t id)
    {
        dbs_proposal& proposal_service = chain().db.obtain_service<dbs_proposal>();
        std::vector<proposal_object::ref_type> proposals = proposal_service.get_proposals();

        for (proposal_object::ref_type p : proposals)
        {
            if (p.get().id._id == id)
            {
                return fc::optional<proposal_object::ref_type>(p);
            }
        }

        return fc::optional<proposal_object::ref_type>();
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

    {
        actor(alice).vote_for(jim_invitation);
        BOOST_CHECK_EQUAL(2, get_committee_members().size());
        BOOST_CHECK(is_committee_member(jim) == true);
    }

    {
        actor(alice).vote_for(joe_invitation);
        BOOST_CHECK_EQUAL(3, get_committee_members().size());
        BOOST_CHECK(is_committee_member(joe) == true);
    }

    {
        actor(alice).vote_for(hue_invitation);
        BOOST_CHECK_EQUAL(4, get_committee_members().size());
        BOOST_CHECK(is_committee_member(hue) == true);
    }

    {
        actor(alice).vote_for(liz_invitation);

        // not enough votes to add liz
        BOOST_CHECK_EQUAL(4, get_committee_members().size());

        actor(jim).vote_for(liz_invitation);
        BOOST_CHECK_EQUAL(5, get_committee_members().size());
    }
    // end setup committee

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

        BOOST_CHECK_EQUAL(3, get_committee_members().size());
    }

    {
        fc::time_point_sec expected_expiration = chain().db.head_block_time() + SCORUM_PROPOSAL_LIFETIME_MIN_SECONDS;

        auto bob_invitation = actor(alice).invite_in_to_committee(bob);

        auto p = get_proposal(bob_invitation);

        BOOST_REQUIRE(p.valid());

        BOOST_CHECK(expected_expiration == p->get().expiration);
    }

} // clang-format on

BOOST_AUTO_TEST_SUITE_END()

} // namespace scorum
} // namespace chain

#endif
