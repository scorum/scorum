#include <scorum/chain/evaluators/cancel_game_evaluator.hpp>
#include <scorum/chain/data_service_factory.hpp>

namespace scorum {
namespace chain {
cancel_game_evaluator::cancel_game_evaluator(data_service_factory_i& services)
    : evaluator_impl<data_service_factory_i, cancel_game_evaluator>(services)
{
}

void cancel_game_evaluator::do_apply(const operation_type& op)
{
}
}
}