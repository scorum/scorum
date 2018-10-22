#include <scorum/chain/database/block_tasks/process_bets_auto_resolving.hpp>
#include <scorum/chain/services/dynamic_global_property.hpp>
#include <scorum/chain/services/game.hpp>
#include <scorum/chain/betting/betting_resolver.hpp>
#include <scorum/chain/betting/betting_service.hpp>

#include <scorum/chain/dba/db_accessor.hpp>

#include <scorum/chain/schema/bet_objects.hpp>
#include <scorum/chain/database/database_virtual_operations.hpp>

namespace scorum {
namespace chain {
namespace database_ns {

process_bets_auto_resolving::process_bets_auto_resolving(betting_service_i& betting_svc,
                                                         database_virtual_operations_emmiter_i& virt_op_emitter)
    : _betting_svc(betting_svc)
    , _virt_op_emitter(virt_op_emitter)
{
}

void process_bets_auto_resolving::on_apply(block_task_context& ctx)
{
    debug_log(ctx.get_block_info(), "process_bets_auto_resolving BEGIN");

    auto& dprops_svc = ctx.services().dynamic_global_property_service();
    auto& game_svc = ctx.services().game_service();

    auto games = game_svc.get_games_to_auto_resolve(dprops_svc.head_block_time());

    for (const game_object& game : games)
    {
        auto game_uuid = game.uuid;
        auto old_status = game.status;

        _betting_svc.cancel_bets(game.id);
        _betting_svc.cancel_game(game.id);

        _virt_op_emitter.push_virtual_operation(
            game_status_changed_operation{ game_uuid, old_status, game_status::expired });
    }

    debug_log(ctx.get_block_info(), "process_bets_auto_resolving END");
}
}
}
}
