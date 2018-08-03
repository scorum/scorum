#include <scorum/chain/services/hardfork_property.hpp>
#include <scorum/chain/database/database.hpp>

namespace scorum {
namespace chain {
dbs_hardfork_property::dbs_hardfork_property(database& db)
    : base_service_type(db)
{
}

bool dbs_hardfork_property::has_hardfork(uint32_t hardfork) const
{
    return db_impl().has_hardfork(hardfork);
}
}
}