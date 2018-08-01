#include <scorum/chain/services/matched_bet.hpp>

namespace scorum {
namespace chain {

dbs_matched_bet::dbs_matched_bet(database& db)
    : base_service_type(db)
{
}

const matched_bet_object& dbs_matched_bet::get_matched_bets(const matched_bet_id_type& obj_id) const
{
    try
    {
        return get_by<by_id>(obj_id);
    }
    FC_CAPTURE_LOG_AND_RETHROW((obj_id))
}
}
}
