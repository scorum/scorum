#include <scorum/chain/services/betting_service.hpp>

#include <scorum/chain/services/betting_property.hpp>

namespace scorum {
namespace chain {
dbs_betting::dbs_betting(database& db)
    : _base_type(db)
    , _betting_property(db.betting_property_service())
{
}

bool dbs_betting::is_betting_moderator(const account_name_type& account_name) const
{
    try
    {
        return _betting_property.get().moderator == account_name;
    }
    FC_CAPTURE_LOG_AND_RETHROW((account_name))
}
}
}
