#include <scorum/chain/evaluators/create_game_evaluator.hpp>
#include <scorum/chain/data_service_factory.hpp>
#include <scorum/chain/services/account.hpp>
#include <scorum/chain/services/dynamic_global_property.hpp>
#include <scorum/chain/services/game.hpp>

#include <scorum/chain/dba/db_accessor.hpp>
#include <scorum/chain/betting/betting_service.hpp>

#include <scorum/utils/collect_range_adaptor.hpp>

namespace scorum {
namespace chain {
create_game_evaluator::create_game_evaluator(data_service_factory_i& services,
                                             betting_service_i& betting_service,
                                             dba::db_accessor<game_uuid_history_object>& uuid_hist_dba)
    : evaluator_impl<data_service_factory_i, create_game_evaluator>(services)
    , _account_service(services.account_service())
    , _dprops_service(services.dynamic_global_property_service())
    , _betting_service(betting_service)
    , _game_svc(services.game_service())
    , _uuid_hist_dba(uuid_hist_dba)
{
}

void create_game_evaluator::do_apply(const operation_type& op)
{
    FC_ASSERT(op.start_time > _dprops_service.head_block_time(), "Game should start after head block time");
    FC_ASSERT(!_uuid_hist_dba.is_exists_by<by_uuid>(op.uuid), "Game with uuid '${g}' already exists", ("g", op.uuid));

    _account_service.check_account_existence(op.moderator);
    FC_ASSERT(_betting_service.is_betting_moderator(op.moderator), "User ${u} isn't a betting moderator",
              ("u", op.moderator));

    fc::flat_set<market_type> markets(op.markets.begin(), op.markets.end());

    _game_svc.create_game(op.uuid, op.moderator, op.json_metadata, op.start_time, op.auto_resolve_delay_sec, op.game,
                          markets);
}
}
}
