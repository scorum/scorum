#include <scorum/chain/services/game.hpp>
#include <scorum/chain/database/database.hpp>
#include <scorum/chain/services/dynamic_global_property.hpp>
#include <scorum/chain/services/betting_property.hpp>
#include <boost/lambda/lambda.hpp>

namespace scorum {
namespace chain {

dbs_game::dbs_game(database& db)
    : base_service_type(db)
    , _dprops_service(db.dynamic_global_property_service())
    , _betting_props_service(db.betting_property_service())
{
}

const game_object& dbs_game::create_game(const account_name_type& moderator,
                                         const std::string& game_name,
                                         fc::time_point_sec start,
                                         const game_type& game,
                                         const fc::flat_set<market_type>& markets)
{
    return dbs_service_base<game_service_i>::create([&](game_object& obj) {
        fc::from_string(obj.name, game_name);
        obj.start = start;
        obj.original_start = start;
        obj.last_update = _dprops_service.head_block_time();
        obj.game = game;
        obj.status = game_status::created;

        for (const auto& m : markets)
            obj.markets.emplace(m);
    });
}

void dbs_game::finish(const game_object& game, const fc::flat_set<wincase_type>& wincases)
{
    update(game, [&](game_object& g) {
        if (g.status == game_status::started)
            g.bets_resolve_time = _dprops_service.head_block_time() + _betting_props_service.get().resolve_delay_sec;
        g.status = game_status::finished;
        g.last_update = _dprops_service.head_block_time();

        g.results.clear();
        for (const auto& w : wincases)
            g.results.emplace(w);
    });
}

void dbs_game::update_markets(const game_object& game, const fc::flat_set<market_type>& markets)
{
    update(game, [&](game_object& g) {
        g.markets.clear();
        for (const auto& m : markets)
            g.markets.emplace(m);
    });
}

bool dbs_game::is_exists(const std::string& game_name) const
{
    return find_by<by_name>(game_name) != nullptr;
}

bool dbs_game::is_exists(int64_t game_id) const
{
    return find_by<by_id>(game_id) != nullptr;
}

const game_object& dbs_game::get_game(const std::string& game_name) const
{
    return get_by<by_name>(game_name);
}

const game_object& dbs_game::get_game(int64_t game_id) const
{
    return get_by<by_id>(game_id);
}
std::vector<dbs_game::object_cref_type> dbs_game::get_games(fc::time_point_sec start) const
{
    return get_range_by<by_start_time>(start <= boost::lambda::_1, boost::lambda::_1 <= start);
}

dbs_game::view_type dbs_game::get_games() const
{
    auto& idx = db_impl().get_index<game_index, by_id>();
    return { idx.begin(), idx.end() };
}

std::vector<dbs_game::object_cref_type> dbs_game::get_games_to_resolve(fc::time_point_sec resolve_time) const
{
    return get_range_by<by_resolve_time>(resolve_time <= boost::lambda::_1, boost::lambda::_1 <= resolve_time);
}

} // namespace scorum
} // namespace chain
