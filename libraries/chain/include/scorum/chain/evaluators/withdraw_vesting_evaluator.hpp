#pragma once

#include <scorum/protocol/scorum_operations.hpp>

#include <scorum/chain/evaluators/evaluator.hpp>

namespace scorum {
namespace chain {

class account_service_i;
class dynamic_global_property_service_i;

class data_service_factory_i;

class withdraw_vesting_evaluator : public evaluator_impl<data_service_factory_i, withdraw_vesting_evaluator>
{
public:
    using operation_type = scorum::protocol::withdraw_vesting_operation;

    withdraw_vesting_evaluator(data_service_factory_i& services);

    void do_apply(const operation_type& op);

private:
    account_service_i& _account_service;
    dynamic_global_property_service_i& _dprops_service;
};

} // namespace chain
} // namespace scorum
