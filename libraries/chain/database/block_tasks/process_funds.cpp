#include <scorum/chain/database/block_tasks/process_funds.hpp>

#include <scorum/chain/services/account.hpp>
#include <scorum/chain/services/budget.hpp>
#include <scorum/chain/services/dev_pool.hpp>
#include <scorum/chain/services/dynamic_global_property.hpp>
#include <scorum/chain/services/reward_balancer.hpp>
#include <scorum/chain/services/reward_funds.hpp>
#include <scorum/chain/services/witness.hpp>

#include <scorum/chain/schema/account_objects.hpp>
#include <scorum/chain/schema/dynamic_global_property_object.hpp>
#include <scorum/chain/schema/scorum_objects.hpp>
#include <scorum/chain/schema/dev_committee_object.hpp>

namespace scorum {
namespace chain {
namespace database_ns {

using scorum::protocol::producer_reward_operation;

void process_funds::on_apply(block_task_context& ctx)
{
    data_service_factory_i& services = ctx.services();
    reward_service_i& reward_service = services.reward_service();
    budget_service_i& budget_service = services.budget_service();
    dev_pool_service_i& dev_service = services.dev_pool_service();

    // We don't have inflation.
    // We just get per block reward from original reward fund(0.48M SP)
    // and expect that after initial supply is handed out(fund budget is over) reward budgets will be created by our
    // users(through the purchase of advertising). Advertising budgets are in SCR.

    if (budget_service.is_fund_budget_exists())
    {
        const budget_object& budget = budget_service.get_fund_budget();
        asset original_fund_reward = budget_service.allocate_cash(budget);
        distribute_reward(ctx, asset(original_fund_reward.amount, SP_SYMBOL));
    }

    asset advertising_budgets_reward = asset(0, SCORUM_SYMBOL);
    for (const budget_object& budget : budget_service.get_budgets())
    {
        advertising_budgets_reward += budget_service.allocate_cash(budget);
    }

    // 50% of the revenue goes to support and develop the product, namely,
    // towards the company’s R&D center.
    asset dev_team_reward = advertising_budgets_reward * SCORUM_DEV_TEAM_PER_BLOCK_REWARD_PERCENT / SCORUM_100_PERCENT;
    dev_service.update([&](dev_committee_object& dco) { dco.scr_balance += dev_team_reward; });

    // 50% of revenue is distributed in SCR among users.
    // pass it through reward balancer
    reward_service.increase_ballance(advertising_budgets_reward - dev_team_reward);
    asset users_reward = reward_service.take_block_reward();

    distribute_reward(ctx, users_reward);
}

void process_funds::distribute_reward(block_task_context& ctx, const asset& users_reward)
{
    if (users_reward.amount <= 0)
        return;

    data_service_factory_i& services = ctx.services();
    account_service_i& account_service = services.account_service();
    dynamic_global_property_service_i& dgp_service = services.dynamic_global_property_service();
    witness_service_i& witness_service = services.witness_service();

    auto reward_symbol = users_reward.symbol();

    // 5% of total per block reward(equal to 10% of users only reward) to witness pay
    asset witness_reward = users_reward * SCORUM_WITNESS_PER_BLOCK_REWARD_PERCENT / SCORUM_100_PERCENT;
    asset content_reward = users_reward - witness_reward;

    const auto& cwit = witness_service.get(dgp_service.get().current_witness);

    if (cwit.schedule != witness_object::top20 && cwit.schedule != witness_object::timeshare)
    {
        wlog("Encountered unknown witness type for witness: ${w}", ("w", cwit.owner));
    }

    const auto& witness = account_service.get_account(cwit.owner);
    if (SCORUM_SYMBOL == reward_symbol)
    {
        account_service.increase_balance(witness, witness_reward);

        reward_fund_scr_service_i& reward_fund_service = services.reward_fund_scr_service();
        reward_fund_service.update([&](reward_fund_scr_object& rfo) { rfo.activity_reward_balance += content_reward; });
    }
    else if (SP_SYMBOL == reward_symbol)
    {
        account_service.create_scorumpower(witness, witness_reward);

        reward_fund_sp_service_i& reward_fund_service = services.reward_fund_sp_service();
        reward_fund_service.update([&](reward_fund_sp_object& rfo) { rfo.activity_reward_balance += content_reward; });
    }

    ctx.push_virtual_operation(producer_reward_operation(witness.name, witness_reward));

    dgp_service.update(
        [&](dynamic_global_property_object& p) { p.circulating_capital += asset(users_reward.amount, SCORUM_SYMBOL); });
}
}
}
}
