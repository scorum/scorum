#pragma once

#include <scorum/protocol/scorum_operations.hpp>
#include <scorum/chain/evaluators/evaluator.hpp>

namespace scorum {
namespace chain {

class data_service_factory_i;

class vote_evaluator : public evaluator_impl<data_service_factory_i, vote_evaluator>
{
public:
    using operation_type = scorum::protocol::vote_operation;

    vote_evaluator(data_service_factory_i& services);

    void do_apply(const operation_type& op);
};

} // namespace chain
} // namespace scorum
