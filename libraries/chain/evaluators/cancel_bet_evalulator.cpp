#include <scorum/chain/evaluators/cancel_bet_evalulator.hpp>
#include <scorum/chain/data_service_factory.hpp>
#include <scorum/chain/services/account.hpp>
#include <scorum/chain/services/betting_property.hpp>
#include <scorum/chain/services/bet.hpp>
#include <scorum/chain/services/matched_bet.hpp>
#include <scorum/chain/services/matched_bet.hpp>

namespace scorum {
namespace chain {
cancel_bet_evaluator::cancel_bet_evaluator(data_service_factory_i& services,
                                           betting::betting_service_i& betting_service)
    : evaluator_impl<data_service_factory_i, cancel_bet_evaluator>(services)
    , _account_service(services.account_service())
    , _betting_service(betting_service)
{
}

void cancel_bet_evaluator::do_apply(const operation_type& op)
{
    boost::ignore_unused_variable_warning(op);

    // TODO
}
}
}
