#include <scorum/chain/evaluators/update_game_markets_evaluator.hpp>
#include <scorum/chain/data_service_factory.hpp>
#include <scorum/chain/services/account.hpp>
#include <scorum/chain/services/betting_service.hpp>
#include <scorum/chain/services/game.hpp>
#include <scorum/protocol/betting/invariants_validation.hpp>

namespace scorum {
namespace chain {
update_game_markets_evaluator::update_game_markets_evaluator(data_service_factory_i& services)
    : evaluator_impl<data_service_factory_i, update_game_markets_evaluator>(services)
    , _account_service(services.account_service())
    , _betting_service(services.betting_service())
    , _game_service(services.game_service())
{
}

void update_game_markets_evaluator::do_apply(const operation_type& op)
{
    _account_service.check_account_existence(op.moderator);
    auto is_moder = _betting_service.is_betting_moderator(op.moderator);
    FC_ASSERT(is_moder, "User ${u} isn't a betting moderator", ("u", op.moderator));

    auto game_obj = _game_service.find(op.game_id);
    FC_ASSERT(game_obj, "Game with id '${g}' doesn't exist", ("g", op.game_id));

    betting::validate_game(game_obj->game, op.markets);

    _game_service.update(*game_obj, [&](game_object& g) {
        g.markets.clear();
        for (const auto& m : op.markets)
            g.markets.emplace_back(m);
    });
}
}
}