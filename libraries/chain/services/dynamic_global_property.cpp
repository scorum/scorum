#include <scorum/chain/services/dynamic_global_property.hpp>

#include <scorum/chain/database/database.hpp>

#include <scorum/chain/schema/dynamic_global_property_object.hpp>

namespace scorum {
namespace chain {

dbs_dynamic_global_property::dbs_dynamic_global_property(dba::db_index& db)
    : base_service_type(db)
{
}

fc::time_point_sec dbs_dynamic_global_property::get_genesis_time() const
{
    return get().genesis_time;
}

fc::time_point_sec dbs_dynamic_global_property::head_block_time() const
{
    return get().time;
}

uint32_t dbs_dynamic_global_property::head_block_num() const
{
    return get().head_block_number;
}

} // namespace scorum
} // namespace chain
