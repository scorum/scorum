#pragma once
#include <scorum/protocol/scorum_operations.hpp>
#include <scorum/chain/evaluators/evaluator.hpp>

namespace scorum {
namespace chain {
class cancel_game_evaluator : public evaluator_impl<data_service_factory_i, cancel_game_evaluator>
{
public:
    using operation_type = scorum::protocol::cancel_game_operation;

    cancel_game_evaluator(data_service_factory_i& services);

    void do_apply(const operation_type& op);
};
}
}