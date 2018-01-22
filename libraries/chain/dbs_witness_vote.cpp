#include <scorum/chain/dbs_witness_vote.hpp>

#include <scorum/chain/database.hpp>

namespace scorum {
namespace chain {

dbs_witness_vote::dbs_witness_vote(database& db)
    : _base_type(db)
{
}

void dbs_witness_vote::create(witness_id_type witness_id, account_id_type vouter_id)
{
    db_impl().create<witness_vote_object>([&](witness_vote_object& v) {
        v.witness = witness_id;
        v.account = vouter_id;
    });
}

bool dbs_witness_vote::is_exists(witness_id_type witness_id, account_id_type vouter_id) const
{
    const witness_vote_object* vote
        = db_impl().find<witness_vote_object, by_account_witness>(boost::make_tuple(vouter_id, witness_id));
    return vote == nullptr ? false : true;
}

const witness_vote_object& dbs_witness_vote::get(witness_id_type witness_id, account_id_type vouter_id)
{
    try
    {
        return db_impl().get<witness_vote_object, by_account_witness>(boost::make_tuple(vouter_id, witness_id));
    }
    FC_CAPTURE_AND_RETHROW((witness_id)(vouter_id))
}

void dbs_witness_vote::remove(const witness_vote_object& witness_vote)
{
    db_impl().remove(witness_vote);
}

} // namespace chain
} // namespace scorum
