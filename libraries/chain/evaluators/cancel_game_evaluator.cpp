#include <scorum/chain/evaluators/cancel_game_evaluator.hpp>
#include <scorum/chain/data_service_factory.hpp>
#include <scorum/chain/services/account.hpp>
#include <scorum/chain/services/betting_service.hpp>
#include <scorum/chain/services/game.hpp>

namespace scorum {
namespace chain {
cancel_game_evaluator::cancel_game_evaluator(data_service_factory_i& services)
    : evaluator_impl<data_service_factory_i, cancel_game_evaluator>(services)
    , _account_service(services.account_service())
    , _betting_service(services.betting_service())
    , _game_service(services.game_service())
{
}

void cancel_game_evaluator::do_apply(const operation_type& op)
{
    _account_service.check_account_existence(op.moderator);
    auto is_moder = _betting_service.is_betting_moderator(op.moderator);
    FC_ASSERT(is_moder, "User ${u} isn't a betting moderator", ("u", op.moderator));

    auto game_obj = _game_service.find(op.game_id);
    FC_ASSERT(game_obj, "Game with id '${g}' doesn't exist", ("g", op.game_id));

    _betting_service.return_unresolved_bets(*game_obj);

    _betting_service.remove_disputs(*game_obj);
    _betting_service.remove_bets(*game_obj);
    _game_service.remove(*game_obj);
}
}
}