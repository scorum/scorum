#include <scorum/chain/database/block_tasks/process_bets_auto_resolving.hpp>
#include <scorum/chain/services/dynamic_global_property.hpp>
#include <scorum/chain/services/game.hpp>
#include <scorum/chain/betting/betting_resolver.hpp>
#include <scorum/chain/betting/betting_service.hpp>

#include <scorum/chain/dba/db_accessor.hpp>

#include <scorum/chain/schema/bet_objects.hpp>
#include <scorum/chain/schema/dynamic_global_property_object.hpp>
#include <scorum/chain/schema/game_object.hpp>
#include <scorum/chain/database/database_virtual_operations.hpp>

namespace scorum {
namespace chain {
namespace database_ns {

process_bets_auto_resolving::process_bets_auto_resolving(betting_service_i& betting_svc,
                                                         database_virtual_operations_emmiter_i& vop_emitter,
                                                         dba::db_accessor<game_object>& game_dba,
                                                         dba::db_accessor<dynamic_global_property_object>& dprop_dba)
    : _betting_svc(betting_svc)
    , _vop_emitter(vop_emitter)
    , _game_dba(game_dba)
    , _dprop_dba(dprop_dba)
{
}

void process_bets_auto_resolving::on_apply(block_task_context& ctx)
{
    using namespace dba;

    debug_log(ctx.get_block_info(), "process_bets_auto_resolving BEGIN");

    auto head_time = _dprop_dba.get().time;
    auto games = _game_dba.get_range_by<by_auto_resolve_time>(unbounded, _x <= head_time);

    utils::foreach_mut(games, [&](const game_object& game) {

        auto uuid = game.uuid;
        auto old_status = game.status;

        _betting_svc.cancel_bets(uuid);
        _betting_svc.cancel_game(uuid);

        _vop_emitter.push_virtual_operation(game_status_changed_operation{ uuid, old_status, game_status::expired });
    });

    debug_log(ctx.get_block_info(), "process_bets_auto_resolving END");
}
}
}
}
