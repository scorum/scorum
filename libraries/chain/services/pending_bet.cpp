#include <scorum/chain/services/pending_bet.hpp>

namespace scorum {
namespace chain {

dbs_pending_bet::dbs_pending_bet(database& db)
    : base_service_type(db)
{
}

void dbs_pending_bet::foreach_pending_bets(const game_id_type& game_id, dbs_pending_bet::pending_bet_call_type&& call)
{
    const auto& idx = db_impl().get_index<pending_bet_index>().indices().get<by_game_id>().equal_range(game_id);
    for (auto it = idx.first; it != idx.second; ++it)
    {
        if (!call(*it))
            break;
    }
}
}
}
