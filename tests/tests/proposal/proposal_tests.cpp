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

        proposal_id_type create_proposal(const Actor& invitee)
        {
            proposal_create_operation op;
            op.creator = actor.name;
            op.committee_member = invitee.name;
            op.action = proposal_action::invite;
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

        void invite_in_to_committee(const Actor invitee)
        {
            auto proposal = create_proposal(invitee);
            vote_for(proposal);
        }

        void vote_for(proposal_id_type proposal)
        {
            proposal_vote_operation op;
            op.voting_account = actor.name;
            op.proposal_id = proposal._id;

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

        FC_ASSERT("no such proposal");
    }

    committee_members get_committee_members()
    {
        dbs_registration_committee& committee_service = chain().db.obtain_service<dbs_registration_committee>();

        return committee_service.get_committee();
    }

    bool is_in_committee(const Actor& a)
    {
        committee_members members = get_committee_members();

        for (const registration_committee_member_object& member : members)
        {
            if (member.account == a.name)
                return true;
        }

        return false;
    }

    actor_actions actor(const Actor& a)
    {
        actor_actions c(*this, a);
        return c;
    }

    genesis_state_type genesis;

private:
    chain_type_ptr _chain;

protected:
    Actors actors;
};

BOOST_FIXTURE_TEST_SUITE(proposal_operation_tests, proposal_fixture)

// clang-format off
SCORUM_TEST_CASE(proposal)
{
    Actor initdelegate("initdelegate");
    Actor alice("alice");
    Actor bob("bob");

    genesis = Genesis::create(actors)
                            .accounts(bob, "jim", "joe", "hue", "liz")
                            .committee(alice)
                            .generate();

    chain().generate_block();

    actor(initdelegate).give_power(actors["alice"]);
    actor(initdelegate).give_power(actors["jim"]);
    actor(initdelegate).give_power(actors["joe"]);
    actor(initdelegate).give_power(actors["hue"]);
    actor(initdelegate).give_power(actors["liz"]);

    actor(alice).invite_in_to_committee(bob);

    BOOST_CHECK_EQUAL(2, get_committee_members().size());
    BOOST_CHECK(is_in_committee(bob) == true);
} // clang-format on

BOOST_AUTO_TEST_SUITE_END()

} // namespace scorum
} // namespace chain

#endif

// bob = create_account(bob);

// alice = create_committee_member("alice")
// liza = create_committee_member("liza")
// jim = create_committee_member("jim")
// joe = create_committee_member("joe")
// hue = create_committee_member("hue")

// commitee().quorum(60);

// drop_hue = alice.create_proposal(hue, dropout)

// alice.vote_for(drop_hue);
// jim.vote_for(drop_hue);

//// not enoght votes to drop hue
// check(hue).is_in_committee();

// drop_joe = alice.create_proposal(joe, dropout);

// alice.vote_for(drop_joe);
// jim.vote_for(drop_joe);
// liza.vote_for(drop_joe);

//// enoght votes to dropout joe
// check(joe).is_not_in_committee();

//// We drop one member, amount of needed votes reduced and we execute second drop
// check(hue).is_not_in_committee();

//{
//	invite_bob = alice.create_proposal(bob, invite);

//	alice.vote_for(invite_bob);
//	jim.vote_for(invite_bob);

//	check(bob).is_in_committee();
//}
