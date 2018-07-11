#pragma once

#include <scorum/chain/database/block_tasks/block_tasks.hpp>

#include <scorum/protocol/types.hpp>

namespace scorum {
namespace protocol {
struct asset;
}
namespace chain {

class account_object;
class dynamic_global_property_service_i;
class account_service_i;

namespace database_ns {

using scorum::protocol::asset;
using scorum::protocol::budget_type;

class process_funds : public block_task
{
public:
    virtual void on_apply(block_task_context&);

private:
    void distribute_reward(block_task_context& ctx, const asset& reward);
    void distribute_active_sp_holders_reward(block_task_context& ctx, const asset& reward);
    void distribute_witness_reward(block_task_context& ctx, const asset& reward);
    void charge_account_reward(block_task_context& ctx, const account_object&, const asset& reward);
    void charge_witness_reward(block_task_context& ctx, const account_object&, const asset& reward);
    void charge_content_reward(block_task_context& ctx, const asset& reward);
    void charge_activity_reward(block_task_context& ctx, const asset& reward);
    const asset get_activity_reward(block_task_context& ctx, const asset& reward);
    bool apply_mainnet_schedule_crutches(block_task_context&);

    template <typename ServiceIterfaceType, typename VCGCoeffListType>
    asset allocate_advertising_cash(ServiceIterfaceType& service,
                                    dynamic_global_property_service_i& dgp_service,
                                    account_service_i& account_service,
                                    const VCGCoeffListType& vcg_coefficients,
                                    const budget_type type,
                                    database_virtual_operations_emmiter_i& ctx);
};
}
}
}
