#pragma once

#include <scorum/protocol/scorum_operations.hpp>

#include <scorum/chain/evaluators/evaluator.hpp>

#include <memory>

namespace scorum {
namespace chain {

class account_service_i;
class dev_pool_service_i;

class set_withdraw_vesting_route_evaluator_impl;

class data_service_factory_i;

class set_withdraw_vesting_route_to_account_evaluator
    : public evaluator_impl<data_service_factory_i, set_withdraw_vesting_route_to_account_evaluator>
{
public:
    using operation_type = scorum::protocol::set_withdraw_vesting_route_operation;

    set_withdraw_vesting_route_to_account_evaluator(data_service_factory_i& services);
    ~set_withdraw_vesting_route_to_account_evaluator();

    void do_apply(const operation_type& op);

private:
    std::unique_ptr<set_withdraw_vesting_route_evaluator_impl> _impl;
    account_service_i& _account_service;
};

class set_withdraw_vesting_route_to_dev_pool_evaluator
    : public evaluator_impl<data_service_factory_i, set_withdraw_vesting_route_to_dev_pool_evaluator>
{
public:
    using operation_type = scorum::protocol::set_withdraw_vesting_route_to_dev_pool_operation;

    set_withdraw_vesting_route_to_dev_pool_evaluator(data_service_factory_i& services);
    ~set_withdraw_vesting_route_to_dev_pool_evaluator();

    void do_apply(const operation_type& op);

private:
    std::unique_ptr<set_withdraw_vesting_route_evaluator_impl> _impl;
    dev_pool_service_i& _dev_pool_service;
};

} // namespace chain
} // namespace scorum
