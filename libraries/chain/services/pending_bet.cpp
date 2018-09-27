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
        auto& idx = db_impl().get_index<pending_bet_index, by_game_id_kind>();
        auto from = idx.lower_bound(game_id);
        auto to = idx.upper_bound(game_id);

        for (auto it = from; it != to; ++it)
        {
            if (!call(*it))
                break;
        }
    }
    FC_CAPTURE_LOG_AND_RETHROW((game_id))
}

bool dbs_pending_bet::is_exists(const pending_bet_id_type& id) const
{
    return find_by<by_id>(id) != nullptr;
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

std::vector<dbs_pending_bet::object_cref_type> dbs_pending_bet::get_bets(game_id_type game_id) const
{
    try
    {
        // TODO: refactor later using db_accessors

        auto& idx = db_impl().get_index<pending_bet_index, by_game_id_market>();
        auto from = idx.lower_bound(game_id);
        auto to = idx.upper_bound(game_id);
        return { from, to };
    }
    FC_CAPTURE_LOG_AND_RETHROW((game_id))
}

std::vector<dbs_pending_bet::object_cref_type> dbs_pending_bet::get_bets(game_id_type game_id,
                                                                         pending_bet_kind kind) const
{
    try
    {
        auto key = std::make_tuple(game_id, kind);
        return get_range_by<by_game_id_kind>(key <= ::boost::lambda::_1, ::boost::lambda::_1 <= key);
    }
    FC_CAPTURE_LOG_AND_RETHROW((game_id))
}

std::vector<dbs_pending_bet::object_cref_type> dbs_pending_bet::get_bets(game_id_type game_id,
                                                                         account_name_type better) const
{
    try
    {
        // TODO: refactor later using db_accessors

        auto& idx = db_impl().get_index<pending_bet_index, by_game_id_better>();
        auto from = idx.lower_bound(std::make_tuple(game_id, better));
        auto to = idx.upper_bound(std::make_tuple(game_id, better));
        return { from, to };
    }
    FC_CAPTURE_LOG_AND_RETHROW((game_id))
}

std::vector<dbs_pending_bet::object_cref_type> dbs_pending_bet::get_bets(game_id_type game_id,
                                                                         fc::time_point_sec created_from) const
{
    try
    {
        // TODO: refactor later using db_accessors

        auto& idx = db_impl().get_index<pending_bet_index, by_game_id_created>();
        auto from = idx.lower_bound(std::make_tuple(game_id, created_from));
        auto to = idx.upper_bound(game_id);
        return { from, to };
    }
    FC_CAPTURE_LOG_AND_RETHROW((game_id))
}
} // namespace chain
} // namespace scorum
