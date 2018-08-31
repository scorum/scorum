#include <scorum/chain/database/block_tasks/process_games_startup.hpp>
#include <scorum/chain/services/dynamic_global_property.hpp>
#include <scorum/chain/services/game.hpp>

namespace scorum {
namespace chain {
namespace database_ns {

void process_games_startup::on_apply(block_task_context& ctx)
{
    debug_log(ctx.get_block_info(), "process_games_startup BEGIN");

    auto& dprops_service = ctx.services().dynamic_global_property_service();
    auto& game_service = ctx.services().game_service();

    auto games = game_service.get_games(dprops_service.head_block_time());
    for (const auto& game : games)
    {
        game_service.update(game, [](game_object& o) { o.status = game_status::started; });
    }

    debug_log(ctx.get_block_info(), "process_games_startup END");
}
}
}
}
