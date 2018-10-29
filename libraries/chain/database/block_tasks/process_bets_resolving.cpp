#include <scorum/chain/database/block_tasks/process_bets_resolving.hpp>
#include <scorum/chain/betting/betting_resolver.hpp>
#include <scorum/chain/betting/betting_service.hpp>

#include <scorum/chain/schema/dynamic_global_property_object.hpp>
#include <scorum/chain/schema/game_object.hpp>
#include <scorum/chain/schema/bet_objects.hpp>
#include <scorum/chain/dba/db_accessor.hpp>
#include <scorum/chain/dba/db_accessor_helpers.hpp>

#include <scorum/utils/algorithm/foreach_mut.hpp>

namespace scorum {
namespace chain {
namespace database_ns {

process_bets_resolving::process_bets_resolving(betting_service_i& betting_svc,
                                               betting_resolver_i& resolver,
                                               database_virtual_operations_emmiter_i& vop_emitter,
                                               dba::db_accessor<game_object>& game_dba,
                                               dba::db_accessor<dynamic_global_property_object>& dprop_dba)
    : _betting_svc(betting_svc)
    , _resolver(resolver)
    , _vop_emitter(vop_emitter)
    , _game_dba(game_dba)
    , _dprop_dba(dprop_dba)
{
}

void process_bets_resolving::on_apply(block_task_context& ctx)
{
    using namespace dba;

    debug_log(ctx.get_block_info(), "process_bets_resolving BEGIN");

    auto head_time = _dprop_dba.get().time;
    auto games = _game_dba.get_range_by<by_bets_resolve_time>(unbounded, _x <= head_time);

    utils::foreach_mut(games, [&](const game_object& game) {

        auto uuid = game.uuid;
        auto old_status = game.status;

        fc::flat_set<wincase_type> results(game.results.begin(), game.results.end());

        _resolver.resolve_matched_bets(game.id, results);
        _betting_svc.cancel_pending_bets(game.id);
        _betting_svc.cancel_game(game.id);

        _vop_emitter.push_virtual_operation(game_status_changed_operation(uuid, old_status, game_status::resolved));
    });

    debug_log(ctx.get_block_info(), "process_bets_resolving END");
}
}
}
}
