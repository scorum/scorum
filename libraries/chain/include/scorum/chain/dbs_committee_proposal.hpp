#pragma once

#include <scorum/chain/dbs_base_impl.hpp>
#include <scorum/protocol/types.hpp>

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

    void create(const account_name_type& creator, const account_name_type& member, action_t action);
    const proposal_vote_object& vote_for(const account_name_type& member);

private:
    const proposal_vote_object* check_and_return_proposal(const account_name_type& member);
    void adjust_proposal_vote(const proposal_vote_object& member);
};

} // namespace scorum
} // namespace chain
