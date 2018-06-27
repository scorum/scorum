#pragma once

#include <scorum/protocol/scorum_operations.hpp>

#include <scorum/chain/evaluators/evaluator.hpp>

namespace scorum {
namespace chain {

struct account_service_i;
struct post_budget_service_i;
struct banner_budget_service_i;
struct dynamic_global_property_service_i;

class close_budget_evaluator : public evaluator_impl<data_service_factory_i, close_budget_evaluator>
{
public:
    using operation_type = scorum::protocol::close_budget_operation;

    close_budget_evaluator(data_service_factory_i& services);

    void do_apply(const operation_type& op);

private:
    account_service_i& _account_service;
    post_budget_service_i& _post_budget_service;
    banner_budget_service_i& _banner_budget_service;
    dynamic_global_property_service_i& _dprops_service;
};
}
}
