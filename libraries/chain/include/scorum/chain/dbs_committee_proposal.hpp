#pragma once

#include <scorum/chain/dbs_base_impl.hpp>
#include <scorum/protocol/types.hpp>
#include <scorum/chain/proposal_vote_object.hpp>

namespace scorum {
namespace chain {

class proposal_vote_object;

class dbs_committee_proposal : public dbs_base
{
    friend class dbservice_dbs_factory;

protected:
    explicit dbs_committee_proposal(database& db);

public:
    using action_t = scorum::protocol::registration_committee_proposal_action;
    using lifetime_t = scorum::protocol::proposal_life_time;

    void
    create(const account_name_type& creator, const account_name_type& member, action_t action, lifetime_t lifetime);

    void remove(const proposal_vote_object& proposal);

    bool is_exist(const account_name_type& member);

    const proposal_vote_object& get(const account_name_type& member);

    void vote_for(const proposal_vote_object& proposal);

    bool is_expired(const proposal_vote_object& proposal);
};

bool check_quorum(const proposal_vote_object& proposal, uint32_t quorum, size_t members_count);

} // namespace scorum
} // namespace chain
