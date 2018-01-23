#include <scorum/chain/services/dbs_base.hpp>

namespace scorum {
namespace chain {

// dbs_base

dbs_base::dbs_base(database& db)
    : _db_core(db)
{
}

dbs_base::~dbs_base()
{
}

dbservice_dbs_factory& dbs_base::db()
{
    return static_cast<dbservice_dbs_factory&>(_db_core);
}

fc::time_point_sec dbs_base::head_block_time()
{
    return db_impl().head_block_time();
}

database& dbs_base::db_impl()
{
    return _db_core;
}

const database& dbs_base::db_impl() const
{
    return _db_core;
}
}
}
