#include <scorum/chain/services/dbservice_dbs_factory.hpp>
#include <scorum/chain/services/dbs_base.hpp>
#include <scorum/chain/database/database.hpp>

namespace scorum {
namespace chain {

// dbservice_dbs_factory

dbservice_dbs_factory::dbservice_dbs_factory(database& db)
    : _db_core(db)
{
}

dbservice_dbs_factory::~dbservice_dbs_factory()
{
}
}
}
