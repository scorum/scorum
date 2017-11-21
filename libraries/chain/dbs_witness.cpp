#include <scorum/chain/dbs_witness.hpp>
#include <scorum/chain/database.hpp>

#include <scorum/chain/witness_objects.hpp>

namespace scorum {
namespace chain {

dbs_witness::dbs_witness(database& db)
    : _base_type(db)
{
}

const witness_object& dbs_witness::get_witness(const account_name_type& name) const
{
    try
    {
        return db_impl().get<witness_object, by_name>(name);
    }
    FC_CAPTURE_AND_RETHROW((name))
}

const witness_schedule_object& dbs_witness::get_witness_schedule_object() const
{
    try
    {
        return db_impl().get<witness_schedule_object>();
    }
    FC_CAPTURE_AND_RETHROW()
}

const witness_object& dbs_witness::get_top_witness() const
{
    const auto& idx = db_impl().get_index<witness_index>().indices().get<by_vote_name>();
    FC_ASSERT(idx.begin() != idx.end(), "Empty witness_index by_vote_name.");
    return (*idx.begin());
}
}
}
