#include <scorum/chain/services/pending_bet.hpp>
#include <boost/lambda/lambda.hpp>

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

bool dbs_pending_bet::is_exists(const bet_id_type& bet_obj_id) const
{
    return find_by<by_bet_id>(bet_obj_id) != nullptr;
}

const pending_bet_object& dbs_pending_bet::get_by_bet(const bet_id_type& bet_obj_id) const
{
    try
    {
        return get_by<by_bet_id>(bet_obj_id);
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

dbs_pending_bet::view_type dbs_pending_bet::get_bets(pending_bet_id_type lower_bound) const
{
    auto& idx = db_impl().get_index<pending_bet_index, by_id>();
    return { idx.lower_bound(lower_bound), idx.end() };
}

std::vector<dbs_pending_bet::object_cref_type> dbs_pending_bet::get_bets(const game_id_type& game_id) const
{
    try
    {
        return get_range_by<by_game_id>(game_id <= ::boost::lambda::_1, ::boost::lambda::_1 <= game_id);
    }
    FC_CAPTURE_LOG_AND_RETHROW((game_id))
}
} // namespace chain
} // namespace scorum
