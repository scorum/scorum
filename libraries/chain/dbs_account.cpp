#include <scorum/chain/dbs_account.hpp>
#include <scorum/chain/database.hpp>

namespace scorum {
namespace chain {

dbs_account::dbs_account(dbservice &db): _db(static_cast<database &>(db))
{
}

}
}
