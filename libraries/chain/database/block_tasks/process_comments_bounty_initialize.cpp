#include <scorum/chain/database/block_tasks/process_comments_bounty_initialize.hpp>

#include <scorum/chain/services/dynamic_global_property.hpp>
#include <scorum/chain/services/reward_fund.hpp>
#include <scorum/chain/services/comments_bounty_fund.hpp>

#include <scorum/chain/schema/dynamic_global_property_object.hpp>
#include <scorum/chain/schema/reward_objects.hpp>
#include <scorum/chain/schema/comment_objects.hpp>

namespace scorum {
namespace chain {
namespace database_ns {

void process_comments_bounty_initialize::on_apply(block_task_context& ctx)
{
    data_service_factory_i& services = ctx.services();

    dynamic_global_property_service_i& dgp_service = services.dynamic_global_property_service();

    if (dgp_service.head_block_time() < SCORUM_BLOGGING_START_DATE)
    {
        return;
    }

    comments_bounty_fund_service_i& comments_bounty_fund_service = services.comments_bounty_fund_service();

    if (comments_bounty_fund_service.is_exists())
    {
        return;
    }

    reward_fund_sp_service_i& reward_fund_service = services.reward_fund_sp_service();

    const reward_fund_sp_object& rf = reward_fund_service.get();

    auto balance = rf.activity_reward_balance;

    comments_bounty_fund_service.create([&](comments_bounty_fund_object& bounty_fund) {
        bounty_fund.activity_reward_balance = balance;
        bounty_fund.author_reward_curve = rf.author_reward_curve;
        bounty_fund.curation_reward_curve = rf.curation_reward_curve;
    });

    reward_fund_service.update([&](reward_fund_sp_object& rfo) { rfo.activity_reward_balance -= balance; });
}
}
}
}
