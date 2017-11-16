#include <scorum/chain/dbs_database_index.hpp>

#include <scorum/chain/database.hpp>

namespace scorum {
namespace chain {

db_database_index::db_database_index(dbservice& db)
{
    _db = *static_cast<database*>(&db);
}
}
}
