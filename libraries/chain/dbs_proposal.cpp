#include <scorum/chain/dbs_proposal.hpp>
#include <scorum/chain/proposal_object.hpp>
#include <scorum/chain/database.hpp>

namespace scorum {
namespace chain {

dbs_proposal::dbs_proposal(database& db)
    : _base_type(db)
{
}

const proposal_object& dbs_proposal::create(const protocol::account_name_type& creator,
                                            const protocol::account_name_type& member,
                                            protocol::proposal_action action,
                                            const fc::time_point_sec& expiration,
                                            uint64_t quorum)
{
    const auto& proposal = db_impl().create<proposal_object>([&](proposal_object& proposal) {
        proposal.creator = creator;
        proposal.data = fc::variant(member).as_string();
        proposal.action = action;
        proposal.expiration = expiration;
        proposal.quorum_percent = quorum;
    });

    return proposal;
}

void dbs_proposal::remove(const proposal_object& proposal)
{
    db_impl().remove(proposal);
}

bool dbs_proposal::is_exists(proposal_id_type proposal_id)
{
    auto proposal = db_impl().find<proposal_object, by_id>(proposal_id);

    return (proposal == nullptr) ? false : true;
}

const proposal_object& dbs_proposal::get(proposal_id_type proposal_id)
{
    return db_impl().get<proposal_object, by_id>(proposal_id);
}

void dbs_proposal::vote_for(const protocol::account_name_type& voter, const proposal_object& proposal)
{
    db_impl().modify(proposal, [&](proposal_object& p) { p.voted_accounts.insert(voter); });
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
        db_impl().remove(*proposal_expiration_index.begin());
    }
}

void dbs_proposal::for_all_proposals_remove_from_voting_list(const account_name_type& member)
{
    auto& proposals = db_impl().get_index<proposal_object_index>().indices().get<by_id>();

    for (const proposal_object& proposal : proposals)
    {
        db_impl().modify(proposal, [&](proposal_object& p) {

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

    const auto& idx = db_impl().get_index<proposal_object_index>().indices();

    for (auto it = idx.cbegin(); it != idx.end(); ++it)
    {
        ret.push_back(std::cref(*it));
    }

    return ret;
}

} // namespace scorum
} // namespace chain
