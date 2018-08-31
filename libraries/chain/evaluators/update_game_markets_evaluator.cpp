#include <scorum/chain/evaluators/update_game_markets_evaluator.hpp>
#include <scorum/chain/data_service_factory.hpp>
#include <scorum/chain/services/account.hpp>
#include <scorum/chain/services/game.hpp>
#include <scorum/protocol/betting/invariants_validation.hpp>
#include <scorum/protocol/betting/wincase_comparison.hpp>
#include <scorum/utils/range_adaptors.hpp>

#include <boost/range/algorithm/sort.hpp>
#include <boost/range/algorithm/set_algorithm.hpp>

#include <scorum/chain/betting/betting_service.hpp>
#include <scorum/chain/betting/betting_resolver.hpp>

namespace scorum {
namespace chain {
update_game_markets_evaluator::update_game_markets_evaluator(data_service_factory_i& services,
                                                             betting::betting_service_i& betting_service,
                                                             betting::betting_resolver_i& betting_resolver)
    : evaluator_impl<data_service_factory_i, update_game_markets_evaluator>(services)
    , _account_service(services.account_service())
    , _betting_service(betting_service)
    , _betting_resolver(betting_resolver)
    , _game_service(services.game_service())
{
}

void update_game_markets_evaluator::do_apply(const operation_type& op)
{
    _account_service.check_account_existence(op.moderator);
    FC_ASSERT(_betting_service.is_betting_moderator(op.moderator), "User ${u} isn't a betting moderator",
              ("u", op.moderator));
    FC_ASSERT(_game_service.is_exists(op.game_id), "Game with id '${g}' doesn't exist", ("g", op.game_id));

    auto game = _game_service.get_game(op.game_id);
    FC_ASSERT(game.status != game_status::finished, "Cannot change the markets when game is finished");

    protocol::betting::validate_game(game.game, op.markets);

    auto get_wincases = [](const auto& market) {
        return market.visit([&](const auto& market_impl) { return market_impl.create_wincase_pairs(); });
    };
    auto old_wincases = utils::flatten(game.markets, get_wincases);
    auto new_wincases = utils::flatten(op.markets, get_wincases);

    boost::sort(old_wincases);
    boost::sort(new_wincases);

    std::vector<protocol::betting::wincase_pair> cancelled_wincases;
    boost::set_difference(old_wincases, new_wincases, std::back_inserter(cancelled_wincases));

    FC_ASSERT(game.status == game_status::created || cancelled_wincases.empty(),
              "Cannot cancel markets after game was started");

    auto bets = _betting_service.get_bets(game.id, cancelled_wincases);
    _betting_resolver.return_bets(bets);
    _betting_service.cancel_bets(bets);

    _game_service.update_markets(game, op.markets);
}
}
}
