#include <scorum/chain/database/block_tasks/process_comments_cashout.hpp>

#include <scorum/chain/database/block_tasks/comments_cashout_impl.hpp>

namespace scorum {
namespace chain {
namespace database_ns {

void process_comments_cashout::on_apply(block_task_context& ctx)
{
    debug_log(ctx.get_block_info(), "process_comments_cashout BEGIN");

    dynamic_global_property_service_i& dgp_service = ctx.services().dynamic_global_property_service();
    content_reward_fund_scr_service_i& content_reward_fund_scr_service
        = ctx.services().content_reward_fund_scr_service();
    content_reward_fund_sp_service_i& content_reward_fund_sp_service = ctx.services().content_reward_fund_sp_service();
    comment_service_i& comment_service = ctx.services().comment_service();
    process_comments_cashout_impl impl(ctx);

    impl.update_decreasing_total_claims(content_reward_fund_scr_service);
    impl.update_decreasing_total_claims(content_reward_fund_sp_service);

    comment_service_i::comment_refs_type comments = comment_service.get_by_cashout_time(dgp_service.head_block_time());
    comment_service_i::comment_refs_type voted_comments;

    std::copy_if(comments.begin(), comments.end(), std::back_inserter(voted_comments),
                 [&](const comment_object& c) { return c.net_rshares > 0; });

    impl.reward(content_reward_fund_scr_service, voted_comments);
    impl.reward(content_reward_fund_sp_service, voted_comments);

    for (const comment_object& comment : comments)
    {
        impl.close_comment_payout(comment);
    }

    debug_log(ctx.get_block_info(), "process_comments_cashout END");
}
}
}
}
