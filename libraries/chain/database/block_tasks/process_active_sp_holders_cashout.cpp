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
        if (reward_scr.amount > 0)
        {
            account_service.decrease_pending_balance(account, reward_scr);
            account_service.increase_balance(account, reward_scr);
        }
        auto reward_sp = account.active_sp_holders_pending_sp_reward;
        if (reward_sp.amount > 0)
        {
            account_service.decrease_pending_scorumpower(account, reward_sp);
            account_service.create_scorumpower(account, reward_sp);
        }
        account_service.update(
            account, [&](account_object& obj) { obj.active_sp_holders_cashout_time = fc::time_point_sec::maximum(); });
        if (reward_scr.amount > 0)
        {
            ctx.push_virtual_operation(active_sp_holders_reward_operation(account.name, reward_scr));
        }
        if (reward_sp.amount > 0)
        {
            ctx.push_virtual_operation(active_sp_holders_reward_operation(account.name, reward_sp));
        }
        if (account.voting_power < SCORUM_100_PERCENT && account.last_vote_cashout_time > dgp_service.head_block_time())
        {
            account_service.update_active_sp_holders_cashout_time(account);
        }
    }

    debug_log(ctx.get_block_info(), "process_active_sp_holders_cashout END");
}
}
}
}
