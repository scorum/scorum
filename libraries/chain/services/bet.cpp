#include <scorum/chain/services/bet.hpp>

namespace scorum {
namespace chain {

dbs_bet::dbs_bet(database& db)
    : base_service_type(db)
{
}

const bet_object& dbs_bet::get_bet(const bet_id_type& bet_id) const
{
    try
    {
        return get_by<by_id>(bet_id);
    }
    FC_CAPTURE_LOG_AND_RETHROW((bet_id))
}
}
}
