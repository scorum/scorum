#include <scorum/chain/evaluators/update_game_markets_evaluator.hpp>
#include <scorum/chain/data_service_factory.hpp>
#include <scorum/chain/services/account.hpp>
#include <scorum/chain/services/betting_service.hpp>
#include <scorum/chain/services/game.hpp>
#include <scorum/protocol/betting/invariants_validation.hpp>
#include <scorum/protocol/betting/wincase_comparison.hpp>
#include <scorum/utils/range_adaptors.hpp>

#include <boost/range/algorithm/sort.hpp>
#include <boost/range/algorithm/set_algorithm.hpp>

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
    FC_ASSERT(_betting_service.is_betting_moderator(op.moderator), "User ${u} isn't a betting moderator",
              ("u", op.moderator));
    FC_ASSERT(_game_service.is_exists(op.game_id), "Game with id '${g}' doesn't exist", ("g", op.game_id));

    auto game_obj = _game_service.get(op.game_id);
    FC_ASSERT(game_obj.status != game_status::finished, "Cannot change the markets when game is finished");

    betting::validate_game(game_obj.game, op.markets);

    auto old_wincases = utils::flatten(game_obj.markets, [](const auto& x) { return x.wincases; });
    auto new_wincases = utils::flatten(op.markets, [](const auto& x) { return x.wincases; });

    boost::sort(old_wincases);
    boost::sort(new_wincases);

    std::vector<betting::wincase_pair> cancelled_wincases;
    boost::set_difference(old_wincases, new_wincases, std::back_inserter(cancelled_wincases));

    _betting_service.return_bets(game_obj, cancelled_wincases);

    _game_service.update_markets(game_obj, op.markets);
}
}
}
