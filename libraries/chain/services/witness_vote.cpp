#include <scorum/chain/services/witness_vote.hpp>

#include <scorum/chain/database/database.hpp>

#include <scorum/chain/schema/witness_objects.hpp>

namespace scorum {
namespace chain {

dbs_witness_vote::dbs_witness_vote(database& db)
    : base_service_type(db)
{
}

bool dbs_witness_vote::is_exists(witness_id_type witness_id, account_id_type vouter_id) const
{
    return find_by<by_account_witness>(boost::make_tuple(vouter_id, witness_id));
}

const witness_vote_object& dbs_witness_vote::get(witness_id_type witness_id, account_id_type vouter_id)
{
    try
    {
        return get_by<by_account_witness>(boost::make_tuple(vouter_id, witness_id));
    }
    FC_CAPTURE_AND_RETHROW((vouter_id)(witness_id))
}

} // namespace chain
} // namespace scorum
