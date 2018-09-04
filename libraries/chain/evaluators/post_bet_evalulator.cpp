#include <scorum/chain/evaluators/post_bet_evalulator.hpp>
#include <scorum/chain/data_service_factory.hpp>
#include <scorum/chain/services/account.hpp>
#include <scorum/chain/services/game.hpp>
#include <scorum/chain/schema/bet_objects.hpp>

#include <scorum/chain/betting/betting_service.hpp>
#include <scorum/chain/betting/betting_matcher.hpp>

#include <scorum/protocol/betting/invariants_validation.hpp>

namespace scorum {
namespace chain {
post_bet_evaluator::post_bet_evaluator(data_service_factory_i& services,
                                       betting::betting_service_i& betting_service,
                                       betting::betting_matcher_i& betting_matcher)
    : evaluator_impl<data_service_factory_i, post_bet_evaluator>(services)
    , _account_service(services.account_service())
    , _game_service(services.game_service())
    , _betting_service(betting_service)
    , _betting_matcher(betting_matcher)
{
}

void post_bet_evaluator::do_apply(const operation_type& op)
{
    FC_ASSERT(_game_service.is_exists(op.game_id));

    auto game_obj = _game_service.get_game(op.game_id);

    protocol::betting::validate_if_wincase_in_game(game_obj.game, op.wincase);

    FC_ASSERT(game_obj.status != game_status::finished, "Cannot post bet for game that is finished");

    _account_service.check_account_existence(op.better);

    const auto& better = _account_service.get_account(op.better);

    FC_ASSERT(better.balance >= op.stake, "Insufficient funds");

    const auto& bet_obj = _betting_service.create_bet(op.better, op.game_id, op.wincase,
                                                      odds(op.odds.numerator, op.odds.denominator), op.stake);

    _account_service.decrease_balance(better, op.stake);

    auto kind = op.keep //
        ? pending_bet_kind::live
        : pending_bet_kind::non_live;
    _betting_matcher.match(bet_obj, kind);
}
}
}
