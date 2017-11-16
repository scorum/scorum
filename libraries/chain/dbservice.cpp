#include <scorum/chain/dbservice.hpp>
#include <chainbase/chainbase.hpp>

namespace scorum {
namespace chain {

dbservice::dbservice(database& db)
    : _BaseClass(db)
{
}

dbservice::~dbservice() {}

// for TODO only:
chainbase::database& dbservice::_temporary_public_impl() { return dynamic_cast<chainbase::database&>(*this); }
}
}
