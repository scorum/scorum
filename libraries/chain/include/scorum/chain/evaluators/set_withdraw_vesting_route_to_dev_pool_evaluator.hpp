#pragma once

#include <scorum/protocol/scorum_operations.hpp>

#include <scorum/chain/evaluators/evaluator.hpp>

namespace scorum {
namespace chain {

class account_service_i;
class withdraw_vesting_route_service_i;

class data_service_factory_i;

class set_withdraw_vesting_route_to_dev_pool_evaluator
    : public evaluator_impl<data_service_factory_i, set_withdraw_vesting_route_to_dev_pool_evaluator>
{
public:
    using operation_type = scorum::protocol::set_withdraw_vesting_route_to_dev_pool_operation;

    set_withdraw_vesting_route_to_dev_pool_evaluator(data_service_factory_i& services);

    void do_apply(const operation_type& op);

private:
    account_service_i& _account_service;
    withdraw_vesting_route_service_i& _withdraw_service;
};

} // namespace chain
} // namespace scorum
