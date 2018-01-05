#ifdef IS_TEST_NET

#include <boost/test/unit_test.hpp>

#include "database_fixture.hpp"

namespace scorum {
namespace chain {

class db_proposal_vote_fixture;

class Actor
{
public:
    Actor() = delete;

    Actor(db_proposal_vote_fixture& db, const std::string& name)
        : chain(db)
    {
    }

    void vote_for()
    {
    }

    void create_proposal()
    {
    }

private:
    db_proposal_vote_fixture& chain;
};

class db_proposal_vote_fixture : public clean_database_fixture
{
public:
    Actor creat_account(const std::string& name)
    {
        Actor a(*this, name);

        return a;
    }

    Actor create_committee_member(const std::string& name)
    {
        Actor a(*this, name);

        return a;
    }
};

// namespace scorum
// namespace chain

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
