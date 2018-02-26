#pragma once

#include <scorum/protocol/scorum_operations.hpp>
#include <scorum/protocol/proposal_operations.hpp>

#include <scorum/chain/evaluators/evaluator.hpp>

namespace scorum {
namespace chain {

class account_service_i;
class proposal_service_i;
class registration_committee_service_i;
class dynamic_global_property_service_i;

class data_service_factory_i;

class proposal_create_evaluator : public evaluator_impl<data_service_factory_i, proposal_create_evaluator>
{
public:
    using operation_type = scorum::protocol::proposal_create_operation;

    using change_quorum_operation = scorum::protocol::registration_committee_change_quorum_operation;

    proposal_create_evaluator(data_service_factory_i& services);

    void do_apply(const operation_type& op);

    protocol::percent_type get_quorum(const protocol::proposal_operation& op);

private:
    account_service_i& _account_service;
    proposal_service_i& _proposal_service;
    dynamic_global_property_service_i& _property_service;
};

} // namespace chain
} // namespace scorum
