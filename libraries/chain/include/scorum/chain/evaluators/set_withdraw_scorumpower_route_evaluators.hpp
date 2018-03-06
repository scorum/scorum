#pragma once

#include <scorum/protocol/scorum_operations.hpp>

#include <scorum/chain/evaluators/evaluator.hpp>

#include <scorum/chain/tasks_base.hpp>

#include <memory>

namespace scorum {
namespace chain {

class account_service_i;
class dev_pool_service_i;

class set_withdraw_scorumpower_route_evaluator_impl;

class data_service_factory_i;

// This evaluator sets withdraw vesting route from account to account
// by operation set_withdraw_scorumpower_route_to_account_operation.
class set_withdraw_scorumpower_route_to_account_evaluator
    : public evaluator_impl<data_service_factory_i, set_withdraw_scorumpower_route_to_account_evaluator>
{
public:
    using operation_type = scorum::protocol::set_withdraw_scorumpower_route_to_account_operation;

    set_withdraw_scorumpower_route_to_account_evaluator(data_service_factory_i& services);
    ~set_withdraw_scorumpower_route_to_account_evaluator();

    void do_apply(const operation_type& op);

private:
    std::unique_ptr<set_withdraw_scorumpower_route_evaluator_impl> _impl;
    account_service_i& _account_service;
};

// This evaluator sets withdraw vesting route from account to development pool
// by operation set_withdraw_scorumpower_route_to_dev_pool_operation.
class set_withdraw_scorumpower_route_to_dev_pool_evaluator
    : public evaluator_impl<data_service_factory_i, set_withdraw_scorumpower_route_to_dev_pool_evaluator>
{
public:
    using operation_type = scorum::protocol::set_withdraw_scorumpower_route_to_dev_pool_operation;

    set_withdraw_scorumpower_route_to_dev_pool_evaluator(data_service_factory_i& services);
    ~set_withdraw_scorumpower_route_to_dev_pool_evaluator();

    void do_apply(const operation_type& op);

private:
    std::unique_ptr<set_withdraw_scorumpower_route_evaluator_impl> _impl;
    account_service_i& _account_service;
    dev_pool_service_i& _dev_pool_service;
};

using scorum::protocol::account_name_type;
using scorum::protocol::asset;

class set_withdraw_scorumpower_route_context
{
public:
    explicit set_withdraw_scorumpower_route_context(data_service_factory_i& services,
                                                const account_name_type& account,
                                                uint16_t percent,
                                                bool auto_vest);

    data_service_factory_i& services() const
    {
        return _services;
    }

    const account_name_type& account() const
    {
        return _account;
    }

    uint16_t percent() const
    {
        return _percent;
    }

    bool auto_vest() const
    {
        return _auto_vest;
    }

private:
    data_service_factory_i& _services;
    account_name_type _account;
    uint16_t _percent;
    bool _auto_vest;
};

// This task sets withdraw vesting route from development pool to account
// withount any operation for development commitee purpose.
class set_withdraw_scorumpower_route_from_dev_pool_task : public task<set_withdraw_scorumpower_route_context>
{
public:
    void on_apply(set_withdraw_scorumpower_route_context& ctx);
};

} // namespace chain
} // namespace scorum
