#pragma once

#include <scorum/chain/dbs_base_impl.hpp>
#include <scorum/protocol/types.hpp>
#include <scorum/chain/proposal_object.hpp>

namespace scorum {
namespace chain {

class dbs_proposal : public dbs_base
{
    friend class dbservice_dbs_factory;

protected:
    explicit dbs_proposal(database& db);

public:
    const proposal_object& create(const account_name_type& creator,
                                  const account_name_type& member,
                                  scorum::protocol::proposal_action action,
                                  fc::time_point_sec expiration,
                                  uint64_t quorum);

    void remove(const proposal_object& proposal);

    bool is_exists(proposal_id_type proposal_id);

    const proposal_object& get(proposal_id_type proposal_id);

    void vote_for(const account_name_type& voter, const proposal_object& proposal);

    size_t get_votes(const proposal_object& proposal);

    bool is_expired(const proposal_object& proposal);

    void clear_expired_proposals();

    void remove_voter_in_proposals(const account_name_type& voter);

    std::vector<proposal_object::ref_type> for_all_proposals_remove_from_voting_list(const account_name_type& member);

    std::vector<proposal_object::ref_type> get_proposals();
};

} // namespace scorum
} // namespace chain
