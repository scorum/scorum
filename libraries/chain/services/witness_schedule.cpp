#include <scorum/chain/database/database.hpp>
#include <scorum/chain/services/witness_schedule.hpp>
#include <scorum/chain/schema/witness_objects.hpp>

namespace scorum {
namespace chain {

dbs_witness_schedule::dbs_witness_schedule(database& db)
    : _base_type(db)
{
}

const witness_schedule_object& dbs_witness_schedule::create(const modifier_type& modifier)
{
    return db_impl().create<witness_schedule_object>([&](witness_schedule_object& o) { modifier(o); });
}

const witness_schedule_object& dbs_witness_schedule::get() const
{
    try
    {
        return db_impl().get<witness_schedule_object>();
    }
    FC_CAPTURE_AND_RETHROW()
}

void dbs_witness_schedule::update(const modifier_type& modifier)
{
    db_impl().modify(get(), [&](witness_schedule_object& c) { modifier(c); });
}

void dbs_witness_schedule::remove()
{
    db_impl().remove(get());
}

bool dbs_witness_schedule::is_exists() const
{
    return nullptr != db_impl().find<witness_schedule_object>();
}

} // namespace chain
} // namespace scorum
