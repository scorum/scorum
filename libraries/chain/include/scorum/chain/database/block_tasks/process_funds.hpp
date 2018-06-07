#pragma once

#include <scorum/chain/database/block_tasks/block_tasks.hpp>

namespace scorum {
namespace protocol {
struct asset;
}
namespace chain {
class account_object;
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
    void charge_account_reward(block_task_context& ctx, const account_object&, const asset& reward);
    void charge_content_reward(block_task_context& ctx, const asset& reward);
    void charge_activity_reward(block_task_context& ctx, const asset& reward);
    const asset get_activity_reward(block_task_context& ctx, const asset& reward);
    bool apply_mainnet_schedule_crutches(block_task_context&);
};
}
}
}
