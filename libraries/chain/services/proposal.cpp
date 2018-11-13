#include <scorum/chain/services/proposal.hpp>
#include <scorum/chain/database/database.hpp>

namespace scorum {
namespace chain {

dbs_proposal::dbs_proposal(database& db)
    : base_service_type(db)
{
}

const proposal_object& dbs_proposal::create_proposal(const account_name_type& creator,
                                                     const protocol::proposal_operation& operation,
                                                     const fc::time_point_sec& expiration,
                                                     uint64_t quorum)
{
    return create([&](proposal_object& proposal) {
        proposal.creator = creator;
        proposal.operation = operation;
        proposal.created = head_block_time();
        proposal.expiration = expiration;
        proposal.quorum_percent = quorum;
    });
}

bool dbs_proposal::is_exists(proposal_id_type proposal_id)
{
    return find_by<by_id>(proposal_id) != nullptr;
}

const proposal_object& dbs_proposal::get(proposal_id_type proposal_id)
{
    return get_by<by_id>(proposal_id);
}

void dbs_proposal::vote_for(const protocol::account_name_type& voter, const proposal_object& proposal)
{
    update(proposal, [&](proposal_object& p) { p.voted_accounts.insert(voter); });
}

size_t dbs_proposal::get_votes(const proposal_object& proposal)
{
    return proposal.voted_accounts.size();
}

bool dbs_proposal::is_expired(const proposal_object& proposal)
{
    return (head_block_time() > proposal.expiration) ? true : false;
}

void dbs_proposal::clear_expired_proposals()
{
    const auto& proposal_expiration_index = db_impl().get_index<proposal_object_index>().indices().get<by_expiration>();

    while (!proposal_expiration_index.empty() && is_expired(*proposal_expiration_index.begin()))
    {
        remove(*proposal_expiration_index.begin());
    }
}

void dbs_proposal::for_all_proposals_remove_from_voting_list(const account_name_type& member)
{
    auto& proposals = db_impl().get_index<proposal_object_index>().indices().get<by_id>();

    for (const proposal_object& proposal : proposals)
    {
        update(proposal, [&](proposal_object& p) {

            auto it = p.voted_accounts.find(member);

            if (it != p.voted_accounts.end())
            {
                p.voted_accounts.erase(it);
            }
        });
    }
}

std::vector<proposal_object::cref_type> dbs_proposal::get_proposals()
{
    std::vector<proposal_object::cref_type> ret;

    const auto& idx = db_impl().get_index<proposal_object_index>().indices().get<by_created>();

    for (auto it = idx.cbegin(); it != idx.end(); ++it)
    {
        ret.push_back(std::cref(*it));
    }

    return ret;
}

} // namespace scorum
} // namespace chain
