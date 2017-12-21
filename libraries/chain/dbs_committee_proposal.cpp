#include <scorum/chain/dbs_committee_proposal.hpp>
#include <scorum/chain/registration_objects.hpp>
#include <scorum/chain/database.hpp>

namespace scorum {
namespace chain {

using proposal_vote_object = scorum::chain::proposal_vote_object;

dbs_committee_proposal::dbs_committee_proposal(database& db)
    : _base_type(db)
{
}

void dbs_committee_proposal::create(const protocol::account_name_type& creator,
                                    const protocol::account_name_type& member,
                                    dbs_committee_proposal::action_t action)
{
    db_impl().create<proposal_vote_object>([&](proposal_vote_object& proposal) {
        proposal.creator = creator;
        proposal.member = member;
        proposal.action = action;
        proposal.votes = 0;
    });
}

const proposal_vote_object* dbs_committee_proposal::check_and_return_proposal(const account_name_type& member)
{
    auto proposal = db_impl().find<proposal_vote_object, by_member_name>(member);

    FC_ASSERT(proposal != nullptr, "There is no proposal for account name '${account}'", ("account", member));

    return proposal;
}

void dbs_committee_proposal::adjust_proposal_vote(const proposal_vote_object& proposal)
{
    db_impl().modify(proposal, [&](proposal_vote_object& p) { p.votes += 1; });
}

const proposal_vote_object& dbs_committee_proposal::vote_for(const protocol::account_name_type& member)
{
    auto proposal = check_and_return_proposal(member);

    adjust_proposal_vote(*proposal);

    return *proposal;
}

} // namespace scorum
} // namespace chain
