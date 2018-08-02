#include <scorum/chain/database/block_tasks/process_fifa_world_cup_2018_bounty_cashout.hpp>

#include <scorum/chain/database/block_tasks/comments_cashout_impl.hpp>

namespace scorum {
namespace chain {
namespace database_ns {

void process_fifa_world_cup_2018_bounty_cashout::on_apply(block_task_context& ctx)
{
    debug_log(ctx.get_block_info(), "process_fifa_world_cup_2018_bounty_cashout BEGIN");

    dynamic_global_property_service_i& dprops_service = ctx.services().dynamic_global_property_service();

    if (dprops_service.head_block_time() < SCORUM_FIFA_WORLD_CUP_2018_BOUNTY_CASHOUT_DATE)
    {
        return;
    }

    auto& fifa_world_cup_2018_bounty_reward_fund_service
        = ctx.services().content_fifa_world_cup_2018_bounty_reward_fund_service();

    if (!fifa_world_cup_2018_bounty_reward_fund_service.is_exists())
    {
        return;
    }

    const content_fifa_world_cup_2018_bounty_reward_fund_object& bounty_fund
        = fifa_world_cup_2018_bounty_reward_fund_service.get();

    if (bounty_fund.activity_reward_balance.amount < 1)
    {
        return;
    }

    comment_service_i& comment_service = ctx.services().comment_service();

    const auto fn_filter
        = [&](const comment_object& c) { return c.net_rshares > 0 && c.cashout_time == fc::time_point_sec::maximum(); };
    auto comments = comment_service.get_by_create_time(SCORUM_FIFA_WORLD_CUP_2018_BOUNTY_CASHOUT_DATE, fn_filter);

    process_comments_cashout_impl impl(ctx);

    impl.reward(fifa_world_cup_2018_bounty_reward_fund_service, comments);

    auto balance = bounty_fund.activity_reward_balance;

    if (balance.amount > 0 && !comments.empty())
    {
        wlog("There is precision remainder = ${r}", ("r", balance));

        // distribute precision remainder
        // that is not distributed by default process comments cashout algorithm

        const auto& oldest_comment = comments[0];

        impl.pay_for_comment(oldest_comment, balance, asset(0, balance.symbol()));

        fifa_world_cup_2018_bounty_reward_fund_service.update([&](
            content_fifa_world_cup_2018_bounty_reward_fund_object& bfo) { bfo.activity_reward_balance -= balance; });
    }
    else if (balance.amount > 0)
    {
        wlog("Fund ${a} is not distributed", ("a", balance));

        content_reward_fund_sp_service_i& reward_fund_service = ctx.services().content_reward_fund_sp_service();

        fifa_world_cup_2018_bounty_reward_fund_service.update([&](
            content_fifa_world_cup_2018_bounty_reward_fund_object& bfo) { bfo.activity_reward_balance -= balance; });

        reward_fund_service.update([&](content_reward_fund_sp_object& rfo) { rfo.activity_reward_balance += balance; });
    }

    debug_log(ctx.get_block_info(), "process_fifa_world_cup_2018_bounty_cashout END");
}
}
}
}
