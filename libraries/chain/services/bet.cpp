#include <scorum/chain/services/bet.hpp>
#include <boost/lambda/lambda.hpp>

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

dbs_bet::view_type dbs_bet::get_bets(bet_id_type lower_bound) const
{
    auto& idx = db_impl().get_index<bet_index, by_id>();
    return { idx.lower_bound(lower_bound), idx.end() };
}

bool dbs_bet::is_exists(const bet_id_type& bet_id) const
{
    return find_by<by_id>(bet_id) != nullptr;
}

std::vector<dbs_bet::object_cref_type> dbs_bet::get_bets(const game_id_type& game_id) const
{
    try
    {
        // TODO: refactor later using db_accessors
        auto& idx = db_impl().get_index<bet_index, by_game_id>();
        auto from = idx.lower_bound(game_id);
        auto to = idx.upper_bound(game_id);
        return { from, to };
    }
    FC_CAPTURE_LOG_AND_RETHROW((game_id))
}
} // namespace chain
} // namespace scorum
