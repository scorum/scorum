#include <scorum/chain/evaluators/update_game_start_time_evaluator.hpp>
#include <scorum/chain/data_service_factory.hpp>
#include <scorum/chain/services/account.hpp>
#include <scorum/chain/services/dynamic_global_property.hpp>
#include <scorum/chain/services/betting_service.hpp>
#include <scorum/chain/services/game.hpp>

namespace scorum {
namespace chain {
update_game_start_time_evaluator::update_game_start_time_evaluator(data_service_factory_i& services)
    : evaluator_impl<data_service_factory_i, update_game_start_time_evaluator>(services)
    , _account_service(services.account_service())
    , _dprops_service(services.dynamic_global_property_service())
    , _betting_service(services.betting_service())
    , _game_service(services.game_service())
{
}

void update_game_start_time_evaluator::do_apply(const operation_type& op)
{
    FC_ASSERT(op.start > _dprops_service.head_block_time(), "Game should start after head block time");
    _account_service.check_account_existence(op.moderator);

    FC_ASSERT(_betting_service.is_betting_moderator(op.moderator), "User ${u} isn't a betting moderator",
              ("u", op.moderator));

    FC_ASSERT(_game_service.is_exists(op.game_id), "Game with id '${g}' doesn't exist", ("g", op.game_id));
    auto game_obj = _game_service.get(op.game_id);

    FC_ASSERT(game_obj.status == game_status::created, "Cannot change the start time when game is started");

    _game_service.update(game_obj, [&](game_object& g) { g.start = op.start; });
}
}
}
