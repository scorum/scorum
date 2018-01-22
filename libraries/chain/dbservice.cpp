#include <scorum/chain/dbservice.hpp>
#include <chainbase/chainbase.hpp>

namespace scorum {
namespace chain {

dbservice::dbservice(database& db)
    : _base_type(db)
{
}

dbservice::~dbservice()
{
}
}
}
