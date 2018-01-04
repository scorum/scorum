#include <scorum/chain/dbs_proposal.hpp>
#include <scorum/chain/proposal_vote_object.hpp>
#include <scorum/chain/database.hpp>

namespace scorum {
namespace chain {

dbs_proposal::dbs_proposal(database& db)
    : _base_type(db)
{
}

void dbs_proposal::create(const protocol::account_name_type& creator,
                          const protocol::account_name_type& member,
                          protocol::proposal_action action,
                          fc::time_point_sec expiration,
                          uint64_t quorum)
{
    db_impl().create<proposal_vote_object>([&](proposal_vote_object& proposal) {
        proposal.creator = creator;
        proposal.member = member;
        proposal.action = action;
        proposal.expiration = expiration;
        proposal.quorum_percent = quorum;
    });
}

void dbs_proposal::remove(const proposal_vote_object& proposal)
{
    db_impl().remove(proposal);
}

bool dbs_proposal::is_exist(proposal_id_type proposal_id)
{
    auto proposal = db_impl().find<proposal_vote_object, by_id>(proposal_id);

    return (proposal == nullptr) ? true : false;
}

const proposal_vote_object& dbs_proposal::get(proposal_id_type proposal_id)
{
    auto proposal = db_impl().find<proposal_vote_object, by_id>(proposal_id);

    return *proposal;
}

void dbs_proposal::vote_for(const protocol::account_name_type& voter, const proposal_vote_object& proposal)
{
    db_impl().modify(proposal, [&](proposal_vote_object& p) { p.voted_accounts.insert(voter); });
}

size_t dbs_proposal::get_votes(const proposal_vote_object& proposal)
{
    return proposal.voted_accounts.size();
}

bool dbs_proposal::is_expired(const proposal_vote_object& proposal)
{
    return (head_block_time() > proposal.expiration) ? true : false;
}

void dbs_proposal::clear_expired_proposals()
{
    const auto& proposal_expiration_index = db_impl().get_index<proposal_vote_index>().indices().get<by_expiration>();

    while (!proposal_expiration_index.empty() && is_expired(*proposal_expiration_index.begin()))
    {
        db_impl().remove(*proposal_expiration_index.begin());
    }
}

} // namespace scorum
} // namespace chain
