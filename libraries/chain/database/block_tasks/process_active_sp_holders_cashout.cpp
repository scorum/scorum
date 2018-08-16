#include <scorum/chain/database/block_tasks/process_active_sp_holders_cashout.hpp>

#include <scorum/chain/services/dynamic_global_property.hpp>
#include <scorum/chain/services/account.hpp>

namespace scorum {
namespace chain {
namespace database_ns {

void process_active_sp_holders_cashout::on_apply(block_task_context& ctx)
{
    debug_log(ctx.get_block_info(), "process_active_sp_holders_cashout BEGIN");

    dynamic_global_property_service_i& dgp_service = ctx.services().dynamic_global_property_service();
    account_service_i& account_service = ctx.services().account_service();

    account_service_i::account_refs_type accounts = account_service.get_by_cashout_time(dgp_service.head_block_time());
    for (const account_object& account : accounts)
    {
        auto reward_scr = account.active_sp_holders_pending_scr_reward;
        if (reward_scr > 0)
        {
            account_service.increase_balance(account, asset(reward_scr, SCORUM_SYMBOL));
        }
        auto reward_sp = account.active_sp_holders_pending_sp_reward;
        if (reward_sp > 0)
        {
            account_service.create_scorumpower(account, asset(reward_sp, SCORUM_SYMBOL));
        }
        account_service.update(account, [&](account_object& obj) {
            obj.active_sp_holders_cashout_time = fc::time_point_sec::maximum();
            obj.active_sp_holders_pending_scr_reward = 0;
            obj.active_sp_holders_pending_sp_reward = 0;
        });
        if (reward_scr > 0)
        {
            ctx.push_virtual_operation(
                active_sp_holders_reward_operation(account.name, asset(reward_scr, SCORUM_SYMBOL)));
        }
        if (reward_sp > 0)
        {
            ctx.push_virtual_operation(
                active_sp_holders_reward_operation(account.name, asset(reward_sp, SCORUM_SYMBOL)));
        }
    }

    debug_log(ctx.get_block_info(), "process_active_sp_holders_cashout END");
}
}
}
}
