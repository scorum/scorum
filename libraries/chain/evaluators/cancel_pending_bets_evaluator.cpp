#include <scorum/chain/evaluators/cancel_pending_bets_evaluator.hpp>
#include <scorum/chain/data_service_factory.hpp>
#include <scorum/chain/services/account.hpp>
#include <scorum/chain/services/bet.hpp>
#include <scorum/chain/services/pending_bet.hpp>
#include <scorum/chain/services/matched_bet.hpp>

#include <scorum/chain/betting/betting_service.hpp>

namespace scorum {
namespace chain {
cancel_pending_bets_evaluator::cancel_pending_bets_evaluator(data_service_factory_i& services,
                                                             betting::betting_service_i& betting_service)
    : evaluator_impl<data_service_factory_i, cancel_pending_bets_evaluator>(services)
    , _account_service(services.account_service())
    , _bet_service(services.bet_service())
    , _pending_bet_service(services.pending_bet_service())
    , _betting_service(betting_service)
{
}

void cancel_pending_bets_evaluator::do_apply(const operation_type& op)
{
    try
    {
        _account_service.check_account_existence(op.better);

        for (const auto& bet_id : op.bet_ids)
        {
            FC_ASSERT(_bet_service.is_exists(bet_id), "Bet ${id} doesn't exist", ("id", bet_id));
            const auto& bet = _bet_service.get_bet(bet_id);
            FC_ASSERT(_betting_service.is_betting_moderator(op.better) || bet.better == op.better,
                      "Invalid better for bet ${id}", ("id", bet_id));

            const auto& better = _account_service.get_account(bet.better);

            _account_service.increase_balance(better, bet.rest_stake);

            const auto& pending_bet = _pending_bet_service.get_by_bet(bet_id);
            _pending_bet_service.remove(pending_bet);

            if (!_betting_service.is_bet_matched(bet))
            {
                _bet_service.remove(bet);
            }
            else
            {
                _bet_service.update(bet, [&](bet_object& obj) { obj.rest_stake.amount = 0; });
            }
        }
    }
    FC_CAPTURE_LOG_AND_RETHROW((op))
}
}
}
