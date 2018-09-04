#include <scorum/chain/database/block_tasks/process_games_startup.hpp>
#include <scorum/chain/services/dynamic_global_property.hpp>
#include <scorum/chain/services/game.hpp>
#include <boost/range/adaptor/filtered.hpp>

namespace scorum {
namespace chain {
namespace database_ns {

process_games_startup::process_games_startup(betting::betting_service_i& betting_service)
    : _betting_svc(betting_service)
{
}

void process_games_startup::on_apply(block_task_context& ctx)
{
    using namespace boost::adaptors;
    debug_log(ctx.get_block_info(), "process_games_startup BEGIN");

    auto& dprops_service = ctx.services().dynamic_global_property_service();
    auto& game_service = ctx.services().game_service();

    auto games = game_service.get_games(dprops_service.head_block_time());
    for (const auto& game : filter(games, [](const auto& g) { return g.get().status == game_status::created; }))
    {
        game_service.update(game, [](game_object& o) { o.status = game_status::started; });
        // TODO: cancel non-live pending bets
    }

    debug_log(ctx.get_block_info(), "process_games_startup END");
}
}
}
}
