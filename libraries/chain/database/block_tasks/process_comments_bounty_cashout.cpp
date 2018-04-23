#include <scorum/chain/database/block_tasks/process_comments_bounty_cashout.hpp>

#include <scorum/chain/database/block_tasks/comments_cashout_impl.hpp>

namespace scorum {
namespace chain {
namespace database_ns {

void process_comments_bounty_cashout::on_apply(block_task_context& ctx)
{
    dynamic_global_property_service_i& dprops_service = ctx.services().dynamic_global_property_service();

    if (dprops_service.head_block_time() < SCORUM_BLOGGING_BOUNTY_CASHOUT_DATE)
    {
        return;
    }

    comments_bounty_fund_service_i& comments_bounty_fund_service = ctx.services().comments_bounty_fund_service();

    if (!comments_bounty_fund_service.is_exists())
    {
        return;
    }

    const comments_bounty_fund_object& bounty_fund = comments_bounty_fund_service.get();

    if (bounty_fund.activity_reward_balance.amount < 1)
    {
        return;
    }

    comment_service_i& comment_service = ctx.services().comment_service();

    const auto fn_until = [&](const comment_object& c) { return c.created <= SCORUM_BLOGGING_BOUNTY_CASHOUT_DATE; };
    const auto fn_filter = [&](const comment_object& c) { return c.net_rshares > 0; };
    auto comments = comment_service.get_by_cashout_time(fn_until, fn_filter);

    process_comments_cashout_impl impl(ctx);

    impl.reward(comments_bounty_fund_service, comments);

    auto balance = bounty_fund.activity_reward_balance;

    if (balance.amount > 0 && !comments.empty())
    {
        // distribute precision remainer
        // that is not distributed by default process comments cashout algorithm

        const auto& oldest_comment = comments[0];

        impl.pay_for_comment(oldest_comment, balance);

        comments_bounty_fund_service.update(
            [&](comments_bounty_fund_object& bfo) { bfo.activity_reward_balance -= balance; });
    }
    else if (balance.amount > 0)
    {
        wlog("Fund ${a} is not distributed", ("a", balance));

        reward_fund_sp_service_i& reward_fund_service = ctx.services().reward_fund_sp_service();

        comments_bounty_fund_service.update(
            [&](comments_bounty_fund_object& bfo) { bfo.activity_reward_balance -= balance; });

        reward_fund_service.update([&](reward_fund_sp_object& rfo) { rfo.activity_reward_balance += balance; });
    }
}
}
}
}
