#include <scorum/chain/services/game.hpp>
#include <scorum/chain/database/database.hpp>
#include <scorum/chain/services/dynamic_global_property.hpp>

namespace scorum {
namespace chain {

dbs_game::dbs_game(database& db)
    : base_service_type(db)
    , _dprops_service(db.dynamic_global_property_service())
{
}

const game_object& dbs_game::create_game(const account_name_type& moderator,
                                         const std::string& game_name,
                                         fc::time_point_sec start,
                                         const betting::game_type& game,
                                         const fc::flat_set<betting::market_type>& markets)
{
    return dbs_service_base<game_service_i>::create([&](game_object& obj) {
        obj.moderator = moderator;
        fc::from_string(obj.name, game_name);
        obj.start = start;
        obj.game = game;
        obj.status = game_status::created;

        for (const auto& m : markets)
            obj.markets.emplace(m);
    });
}

void dbs_game::finish(const game_object& game, const fc::flat_set<betting::wincase_type>& wincases)
{
    update(game, [&](game_object& g) {
        g.finish = _dprops_service.head_block_time();
        g.status = game_status::finished;

        for (const auto& w : wincases)
            g.results.emplace(w);
    });
}

void dbs_game::update_markets(const game_object& game, const fc::flat_set<betting::market_type>& markets)
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

const game_object& dbs_game::get(const std::string& game_name) const
{
    return get_by<by_name>(game_name);
}

const game_object& dbs_game::get(int64_t game_id) const
{
    return get_by<by_id>(game_id);
}

dbs_game::view_type dbs_game::get_games() const
{
    auto& idx = db_impl().get_index<game_index, by_id>();
    return { idx.begin(), idx.end() };
}

} // namespace scorum
} // namespace chain
