#include <scorum/chain/evaluators/cancel_game_evaluator.hpp>
#include <scorum/chain/data_service_factory.hpp>
#include <scorum/chain/services/account.hpp>
#include <scorum/chain/services/game.hpp>

#include <scorum/chain/betting/betting_service.hpp>
#include <scorum/chain/betting/betting_resolver.hpp>

namespace scorum {
namespace chain {
cancel_game_evaluator::cancel_game_evaluator(data_service_factory_i& services,
                                             betting::betting_service_i& betting_service,
                                             betting::betting_resolver_i& betting_resolver)
    : evaluator_impl<data_service_factory_i, cancel_game_evaluator>(services)
    , _account_service(services.account_service())
    , _betting_service(betting_service)
    , _game_service(services.game_service())
{
}

void cancel_game_evaluator::do_apply(const operation_type& op)
{
    _account_service.check_account_existence(op.moderator);
    FC_ASSERT(_betting_service.is_betting_moderator(op.moderator), "User ${u} isn't a betting moderator",
              ("u", op.moderator));
    FC_ASSERT(_game_service.is_exists(op.game_id), "Game with id '${g}' doesn't exist", ("g", op.game_id));

    auto game_obj = _game_service.get_game(op.game_id);

    FC_ASSERT(game_obj.status != game_status::finished, "Cannot cancel the game after it is finished");

    _betting_service.return_unresolved_bets(game_obj);

    _betting_service.cancel_game(game_obj.id);
}
}
}
