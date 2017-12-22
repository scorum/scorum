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
    const proposal_vote_object& vote_for(const account_name_type& member);

    void remove(const proposal_vote_object& proposal);

private:
    const proposal_vote_object* check_and_return_proposal(const account_name_type& member);
    void adjust_proposal_vote(const proposal_vote_object& member);
};

bool check_quorum(const proposal_vote_object& proposal, uint32_t quorum, size_t members_count);

} // namespace scorum
} // namespace chain
