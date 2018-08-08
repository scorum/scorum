#include <scorum/chain/evaluators/cancel_matched_bets_evaluator.hpp>
#include <scorum/chain/data_service_factory.hpp>
#include <scorum/chain/services/account.hpp>
#include <scorum/chain/services/betting_property.hpp>
#include <scorum/chain/services/bet.hpp>
#include <scorum/chain/services/pending_bet.hpp>
#include <scorum/chain/services/matched_bet.hpp>

#include <scorum/chain/betting/betting_service.hpp>

namespace scorum {
namespace chain {
cancel_matched_bets_evaluator::cancel_matched_bets_evaluator(data_service_factory_i& services,
                                                             betting::betting_service_i& betting_service)
    : evaluator_impl<data_service_factory_i, cancel_matched_bets_evaluator>(services)
    , _account_service(services.account_service())
    , _bet_service(services.bet_service())
    , _pending_bet_service(services.pending_bet_service())
    , _matched_bet_service(services.matched_bet_service())
    , _betting_service(betting_service)
{
}

void cancel_matched_bets_evaluator::do_apply(const operation_type& op)
{
    _account_service.check_account_existence(op.moderator);

    FC_ASSERT(_betting_service.is_betting_moderator(op.moderator), "Access denied");

    // TODO
}
}
}
