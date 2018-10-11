#include <scorum/chain/evaluators/update_game_markets_evaluator.hpp>
#include <scorum/chain/data_service_factory.hpp>
#include <scorum/chain/services/account.hpp>
#include <scorum/chain/services/game.hpp>
#include <scorum/protocol/betting/invariants_validation.hpp>
#include <scorum/protocol/betting/market.hpp>
#include <scorum/utils/range_adaptors.hpp>

#include <boost/range/algorithm/sort.hpp>
#include <boost/range/algorithm/set_algorithm.hpp>

#include <scorum/chain/betting/betting_service.hpp>
#include <scorum/chain/betting/betting_resolver.hpp>

namespace scorum {
namespace chain {

update_game_markets_evaluator::update_game_markets_evaluator(data_service_factory_i& services,
                                                             betting_service_i& betting_service)
    : evaluator_impl<data_service_factory_i, update_game_markets_evaluator>(services)
    , _account_service(services.account_service())
    , _betting_service(betting_service)
    , _game_service(services.game_service())
{
}

void update_game_markets_evaluator::do_apply(const operation_type& op)
{
    _account_service.check_account_existence(op.moderator);
    FC_ASSERT(_betting_service.is_betting_moderator(op.moderator), "User ${u} isn't a betting moderator",
              ("u", op.moderator));
    FC_ASSERT(_game_service.is_exists(op.uuid), "Game with uuid '${g}' doesn't exist", ("g", op.uuid));

    const auto& game = _game_service.get_game(op.uuid);
    FC_ASSERT(game.status != game_status::finished, "Cannot change the markets when game is finished");

    validate_game(game.game, op.markets);

    fc::flat_set<market_type> cancelled_markets;
    boost::set_difference(game.markets, op.markets, std::inserter(cancelled_markets, cancelled_markets.end()));

    FC_ASSERT(game.status == game_status::created || cancelled_markets.empty(),
              "Cannot cancel markets after game was started");

    _betting_service.cancel_bets(game.id, cancelled_markets);

    _game_service.update_markets(game, op.markets);
}
}
}
