#include <scorum/chain/database/block_tasks/process_funds.hpp>

#include <scorum/chain/services/account.hpp>
#include <scorum/chain/services/reward.hpp>
#include <scorum/chain/services/budget.hpp>
#include <scorum/chain/services/reward_fund.hpp>
#include <scorum/chain/services/dynamic_global_property.hpp>
#include <scorum/chain/services/witness.hpp>

#include <scorum/chain/schema/account_objects.hpp>
#include <scorum/chain/schema/dynamic_global_property_object.hpp>
#include <scorum/chain/schema/scorum_objects.hpp>

namespace scorum {
namespace chain {
namespace database_ns {

using scorum::protocol::producer_reward_operation;

void process_funds::on_apply(block_task_context& ctx)
{
    data_service_factory_i& services = ctx.services();
    account_service_i& account_service = services.account_service();
    reward_service_i& reward_service = services.reward_service();
    budget_service_i& budget_service = services.budget_service();
    reward_fund_service_i& reward_fund_service = services.reward_fund_service();
    dynamic_global_property_service_i& dgp_service = services.dynamic_global_property_service();
    witness_service_i& witness_service = services.witness_service();

    // We don't have inflation.
    // We just get per block reward from reward pool and expect that after initial supply is handed out(fund budget is
    // over) reward budgets will be created by our users.

    asset budgets_reward = asset(0, SCORUM_SYMBOL);
    for (const budget_object& budget : budget_service.get_budgets())
    {
        budgets_reward += budget_service.allocate_cash(budget);
    }
    reward_service.increase_pool_ballance(budgets_reward);

    auto total_block_reward = reward_service.take_block_reward();

    auto content_reward = asset(total_block_reward.amount * SCORUM_CONTENT_REWARD_PERCENT / SCORUM_100_PERCENT,
                                total_block_reward.symbol());
    auto witness_reward = total_block_reward - content_reward; /// Remaining 5% to witness pay

    reward_fund_service.update([&](reward_fund_object& rfo) { rfo.reward_balance += content_reward; });

    dgp_service.update([&](dynamic_global_property_object& p) { p.circulating_capital += total_block_reward; });

    const auto& props = dgp_service.get();

    const auto& cwit = witness_service.get(props.current_witness);

    if (cwit.schedule != witness_object::top20 && cwit.schedule != witness_object::timeshare)
    {
        wlog("Encountered unknown witness type for witness: ${w}", ("w", cwit.owner));
    }

    const auto producer_reward
        = account_service.create_scorumpower(account_service.get_account(cwit.owner), witness_reward);
    ctx.push_virtual_operation(producer_reward_operation(cwit.owner, producer_reward));
}
}
}
}
