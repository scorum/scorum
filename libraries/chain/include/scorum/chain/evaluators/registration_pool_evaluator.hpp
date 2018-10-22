#pragma once

#include <scorum/protocol/scorum_operations.hpp>

#include <scorum/chain/evaluators/evaluator.hpp>

#include <scorum/chain/tasks_base.hpp>

#include <memory>

namespace scorum {
namespace chain {

struct data_service_factory_i;

struct account_service_i;
struct registration_pool_service_i;
struct registration_committee_service_i;
struct dynamic_global_property_service_i;

class registration_pool_object;
class account_object;

class registration_pool_evaluator_impl;

class registration_pool_evaluator : public evaluator_impl<data_service_factory_i, registration_pool_evaluator>
{
public:
    using operation_type = scorum::protocol::account_create_by_committee_operation;

    registration_pool_evaluator(data_service_factory_i& services);
    ~registration_pool_evaluator();

    void do_apply(const operation_type& op);

private:
    std::unique_ptr<registration_pool_evaluator_impl> _impl;

    account_service_i& _account_service;
    registration_pool_service_i& _registration_pool_service;
    registration_committee_service_i& _registration_committee_service;
    dynamic_global_property_service_i& _dprops_service;
};

using scorum::protocol::account_name_type;

class give_bonus_from_registration_pool_task_context
{
public:
    explicit give_bonus_from_registration_pool_task_context(data_service_factory_i& services,
                                                            const account_object& beneficiary);

    data_service_factory_i& services() const;

    const account_object& beneficiary() const;

private:
    data_service_factory_i& _services;
    const account_object& _beneficiary;
};

class give_bonus_from_registration_pool_task : public task<give_bonus_from_registration_pool_task_context>
{
public:
    void on_apply(give_bonus_from_registration_pool_task_context& ctx);
};

} // namespace chain
} // namespace scorum
