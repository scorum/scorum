#include <scorum/chain/dbs_base_impl.hpp>
#include <scorum/chain/database.hpp>

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

dbservice& dbs_base::db()
{
    return static_cast<dbservice&>(_db_core);
}

database& dbs_base::db_impl()
{
    return _db_core;
}

const database& dbs_base::db_impl() const
{
    return _db_core;
}

time_point_sec dbs_base::_get_now(const optional<time_point_sec>& now)
{
    time_point_sec ret;
    if (now.valid())
    {
        ret = (*now);
    }
    else
    {
        ret = db_impl().head_block_time();
    }
    return ret;
}

// dbservice

dbservice_dbs_factory::dbservice_dbs_factory(database& db)
    : _db_core(db)
{
}

dbservice_dbs_factory::~dbservice_dbs_factory()
{
}
}
}
