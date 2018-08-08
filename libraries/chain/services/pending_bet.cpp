#include <scorum/chain/services/pending_bet.hpp>

namespace scorum {
namespace chain {

dbs_pending_bet::dbs_pending_bet(database& db)
    : base_service_type(db)
{
}

void dbs_pending_bet::foreach_pending_bets(const game_id_type& game_id, dbs_pending_bet::pending_bet_call_type call)
{
    try
    {
        const auto& idx = db_impl().get_index<pending_bet_index>().indices().get<by_game_id>().equal_range(game_id);
        for (auto it = idx.first; it != idx.second; ++it)
        {
            if (!call(*it))
                break;
        }
    }
    FC_CAPTURE_LOG_AND_RETHROW((game_id))
}

void dbs_pending_bet::remove_by_bet(const bet_id_type& bet_obj_id)
{
    try
    {
        remove(get_by<by_bet_id>(bet_obj_id));
    }
    FC_CAPTURE_LOG_AND_RETHROW((bet_obj_id))
}

const pending_bet_object& dbs_pending_bet::get_pending_bet(const pending_bet_id_type& obj_id) const
{
    try
    {
        return get_by<by_id>(obj_id);
    }
    FC_CAPTURE_LOG_AND_RETHROW((obj_id))
}
}
}
