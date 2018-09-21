#pragma once

#include <scorum/chain/database/block_tasks/block_tasks.hpp>

#include <scorum/protocol/types.hpp>

namespace scorum {
namespace protocol {
struct asset;
}
namespace chain {

using scorum::protocol::budget_type;

class account_object;
class dynamic_global_property_service_i;
class account_service_i;
template <budget_type> class adv_budget_service_i;

namespace database_ns {

using scorum::protocol::asset;

class process_funds : public block_task
{
public:
    virtual void on_apply(block_task_context&);

private:
    void distribute_reward(block_task_context& ctx, const asset& reward);
    void distribute_active_sp_holders_reward(block_task_context& ctx, const asset& reward);
    void distribute_witness_reward(block_task_context& ctx, const asset& reward);
    void pay_account_reward(block_task_context& ctx, const account_object&, const asset& reward);
    void pay_account_pending_reward(block_task_context& ctx, const account_object&, const asset& reward);
    void pay_witness_reward(block_task_context& ctx, const account_object&, const asset& reward);
    void pay_content_reward(block_task_context& ctx, const asset& reward);
    void pay_activity_reward(block_task_context& ctx, const asset& reward);
    const asset get_activity_reward(block_task_context& ctx, const asset& reward);
    bool apply_mainnet_schedule_crutches(block_task_context&);

    template <budget_type budget_type_v>
    asset process_adv_pending_payouts(adv_budget_service_i<budget_type_v>& budget_svc,
                                      account_service_i& account_svc,
                                      database_virtual_operations_emmiter_i& virt_op_emitter);
};
}
}
}
