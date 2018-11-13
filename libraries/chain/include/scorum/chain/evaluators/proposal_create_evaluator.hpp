#pragma once

#include <scorum/protocol/scorum_operations.hpp>
#include <scorum/protocol/proposal_operations.hpp>

#include <scorum/chain/evaluators/evaluator.hpp>

namespace scorum {
namespace chain {

struct account_service_i;
struct proposal_service_i;
struct registration_committee_service_i;
struct dynamic_global_property_service_i;

struct data_service_factory_i;

class proposal_create_evaluator : public evaluator_impl<data_service_factory_i, proposal_create_evaluator>
{
public:
    using operation_type = scorum::protocol::proposal_create_operation;

    proposal_create_evaluator(data_service_factory_i& services);

    void do_apply(const operation_type& op);

private:
    account_service_i& _account_service;
    proposal_service_i& _proposal_service;
    dynamic_global_property_service_i& _property_service;
};

} // namespace chain
} // namespace scorum
