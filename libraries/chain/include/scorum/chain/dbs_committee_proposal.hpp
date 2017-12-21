#pragma once

#include <scorum/chain/dbs_base_impl.hpp>
#include <scorum/chain/registration_objects.hpp>

namespace scorum {
namespace chain {

class dbs_committee_proposal : public dbs_base
{
    friend class dbservice_dbs_factory;

protected:
    explicit dbs_committee_proposal(database& db);

public:
    using action_t = scorum::protocol::registration_committee_proposal_action;

    void create(const account_name_type& creator, const account_name_type& member, action_t action);
    void vote_for(const account_name_type& member, bool approve);
};

} // namespace scorum
} // namespace chain
