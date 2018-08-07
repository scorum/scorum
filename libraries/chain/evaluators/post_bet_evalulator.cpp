#include <scorum/chain/evaluators/post_bet_evalulator.hpp>
#include <scorum/chain/data_service_factory.hpp>
#include <scorum/chain/services/account.hpp>
#include <scorum/chain/services/game.hpp>

#include <scorum/chain/betting/betting_service.hpp>
#include <scorum/chain/betting/betting_matcher.hpp>

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
    boost::ignore_unused_variable_warning(op);

    // TODO
}
}
}
