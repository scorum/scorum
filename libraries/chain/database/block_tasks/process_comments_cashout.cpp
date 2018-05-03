#include <scorum/chain/database/block_tasks/process_comments_cashout.hpp>

#include <scorum/chain/database/block_tasks/comments_cashout_impl.hpp>

namespace scorum {
namespace chain {
namespace database_ns {

void process_comments_cashout::on_apply(block_task_context& ctx)
{
    dynamic_global_property_service_i& dgp_service = ctx.services().dynamic_global_property_service();
    reward_fund_scr_service_i& reward_fund_scr_service = ctx.services().reward_fund_scr_service();
    reward_fund_sp_service_i& reward_fund_sp_service = ctx.services().reward_fund_sp_service();
    comment_service_i& comment_service = ctx.services().comment_service();
    process_comments_cashout_impl impl(ctx);

    auto comments = comment_service.get_by_cashout_time(dgp_service.head_block_time());

    impl.update_decreasing_total_claims(reward_fund_scr_service);
    impl.reward(reward_fund_scr_service, comments);
    impl.update_decreasing_total_claims(reward_fund_sp_service);
    impl.reward(reward_fund_sp_service, comments);

    for (const comment_object& comment : comments)
    {
        impl.close_comment_payout(comment);
    }
}
}
}
}
