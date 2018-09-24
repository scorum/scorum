#pragma once

#include <scorum/protocol/scorum_operations.hpp>
#include <scorum/protocol/types.hpp>
#include <scorum/chain/evaluators/evaluator.hpp>

namespace scorum {
namespace chain {

struct account_service_i;
struct post_budget_service_i;
struct banner_budget_service_i;
struct database_virtual_operations_emmiter_i;
template <protocol::budget_type> struct adv_budget_service_i;

class close_budget_evaluator : public evaluator_impl<data_service_factory_i, close_budget_evaluator>
{
public:
    using operation_type = scorum::protocol::close_budget_operation;

    close_budget_evaluator(data_service_factory_i& services, database_virtual_operations_emmiter_i& virt_op_emmiter);

    void do_apply(const operation_type& op);

private:
    template <protocol::budget_type budget_type_v>
    void close_budget(adv_budget_service_i<budget_type_v>& budget_svc, const operation_type& op);

private:
    account_service_i& _account_service;
    post_budget_service_i& _post_budget_service;
    banner_budget_service_i& _banner_budget_service;
    database_virtual_operations_emmiter_i& _virt_op_emmiter;
};
}
}
