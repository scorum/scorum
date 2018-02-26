#pragma once

#include <scorum/protocol/scorum_operations.hpp>
#include <scorum/chain/evaluators/evaluator.hpp>

#include <scorum/protocol/types.hpp>

namespace scorum {
namespace chain {

class data_service_factory_i;
class account_service_i;
class proposal_service_i;
class dynamic_global_property_service_i;
class proposal_executor_service_i;

class proposal_vote_evaluator : public evaluator_impl<data_service_factory_i, proposal_vote_evaluator>
{
public:
    using operation_type = scorum::protocol::proposal_vote_operation;

    proposal_vote_evaluator(data_service_factory_i& services);

    void do_apply(const operation_type& op);

private:
    account_service_i& _account_service;
    proposal_service_i& _proposal_service;
    dynamic_global_property_service_i& _properties_service;
    proposal_executor_service_i& _proposal_executor;
};

} // namespace chain
} // namespace scorum
