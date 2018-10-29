#include <scorum/chain/services/dbs_base.hpp>

namespace scorum {
namespace chain {

// dbs_base

dbs_base::dbs_base(dba::db_index& db)
    : _db_core(db)
{
}

dbs_base::~dbs_base()
{
}

fc::time_point_sec dbs_base::head_block_time()
{
    return _db_core.get(dynamic_global_property_object::id_type(0)).time;
}

dba::db_index& dbs_base::db_impl()
{
    return _db_core;
}

const dba::db_index& dbs_base::db_impl() const
{
    return _db_core;
}
}
}
