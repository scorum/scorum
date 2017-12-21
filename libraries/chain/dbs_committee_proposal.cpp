#include <scorum/chain/dbs_committee_proposal.hpp>

namespace scorum {
namespace chain {

dbs_committee_proposal::dbs_committee_proposal(database& db)
    : _base_type(db)
{
}

void dbs_committee_proposal::create(const protocol::account_name_type& creator,
                                    const protocol::account_name_type& member,
                                    dbs_committee_proposal::action_t action)
{
}

void dbs_committee_proposal::vote_for(const protocol::account_name_type& member, bool approve)
{
}

} // namespace scorum
} // namespace chain
