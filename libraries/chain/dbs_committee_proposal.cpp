#include <scorum/chain/dbs_committee_proposal.hpp>
#include <scorum/chain/proposal_vote_object.hpp>
#include <scorum/chain/database.hpp>

namespace scorum {
namespace chain {

dbs_committee_proposal::dbs_committee_proposal(database& db)
    : _base_type(db)
{
}

void dbs_committee_proposal::create(const protocol::account_name_type& creator,
                                    const protocol::account_name_type& member,
                                    dbs_committee_proposal::action_t action,
                                    lifetime_t lifetime)
{
    db_impl().create<proposal_vote_object>([&](proposal_vote_object& proposal) {
        proposal.creator = creator;
        proposal.member = member;
        proposal.action = action;
        proposal.votes = 0;
        proposal.created = db_impl().head_block_time();
        proposal.lifetime = lifetime;
    });
}

void dbs_committee_proposal::remove(const proposal_vote_object& proposal)
{
    db_impl().remove(proposal);
}

bool dbs_committee_proposal::is_exist(const account_name_type& member)
{
    auto proposal = db_impl().find<proposal_vote_object, by_member_name>(member);

    return (proposal == nullptr) ? true : false;
}

const proposal_vote_object& dbs_committee_proposal::get(const account_name_type& member)
{
    auto proposal = db_impl().find<proposal_vote_object, by_member_name>(member);

    return *proposal;
}

void dbs_committee_proposal::vote_for(const proposal_vote_object& proposal)
{
    db_impl().modify(proposal, [&](proposal_vote_object& p) { p.votes += 1; });
}

bool dbs_committee_proposal::is_expired(const proposal_vote_object& proposal)
{
    return (proposal.expiration < this->_get_now()) ? false : true;
}

bool check_quorum(uint32_t votes, uint32_t quorum, size_t members_count)
{
    const uint32_t needed_votes = (members_count * quorum) / SCORUM_100_PERCENT;

    return (votes >= needed_votes) ? true : false;
}

} // namespace scorum
} // namespace chain
