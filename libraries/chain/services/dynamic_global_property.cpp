#include <scorum/chain/services/dynamic_global_property.hpp>

#include <scorum/chain/database/database.hpp>

#include <scorum/chain/schema/dynamic_global_property_object.hpp>

namespace scorum {
namespace chain {

dbs_dynamic_global_property::dbs_dynamic_global_property(database& db)
    : base_service_type(db)
{
}

time_point_sec dbs_dynamic_global_property::get_genesis_time() const
{
    return db_impl().get_genesis_time();
}

fc::time_point_sec dbs_dynamic_global_property::head_block_time() const
{
    return get().time;
}

} // namespace scorum
} // namespace chain
