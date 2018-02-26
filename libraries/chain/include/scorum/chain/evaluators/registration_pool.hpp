#pragma once

#include <scorum/protocol/scorum_operations.hpp>

#include <scorum/chain/evaluators/evaluator.hpp>

#include <scorum/chain/tasks_base.hpp>

#include <memory>

namespace scorum {
namespace chain {

class data_service_factory_i;

class account_service_i;
class registration_pool_service_i;
class registration_committee_service_i;

class registration_pool_object;

class registration_pool_impl;

class registration_pool_evaluator : public evaluator_impl<data_service_factory_i, registration_pool_evaluator>
{
public:
    using operation_type = scorum::protocol::account_create_by_committee_operation;

    registration_pool_evaluator(data_service_factory_i& services);
    ~registration_pool_evaluator();

    void do_apply(const operation_type& op);

private:
    std::unique_ptr<registration_pool_impl> _impl;

    account_service_i& _account_service;
    registration_pool_service_i& _registration_pool_service;
    registration_committee_service_i& _registration_committee_service;
};

using scorum::protocol::asset;

class registration_pool_context
{
public:
    explicit registration_pool_context(data_service_factory_i& services);

    data_service_factory_i& services() const
    {
        return _services;
    }

private:
    data_service_factory_i& _services;
};

class registration_pool_task : public task<registration_pool_context>
{
public:
    void on_apply(registration_pool_context& ctx);
};

} // namespace chain
} // namespace scorum
