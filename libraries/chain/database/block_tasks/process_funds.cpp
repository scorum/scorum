#include <scorum/chain/database/block_tasks/process_funds.hpp>
#include <scorum/chain/database/block_tasks/reward_balance_algorithm.hpp>

#include <scorum/chain/services/account.hpp>
#include <scorum/chain/services/budgets.hpp>
#include <scorum/chain/services/dev_pool.hpp>
#include <scorum/chain/services/dynamic_global_property.hpp>
#include <scorum/chain/services/reward_balancer.hpp>
#include <scorum/chain/services/reward_funds.hpp>
#include <scorum/chain/services/witness.hpp>
#include <scorum/chain/services/advertising_property.hpp>
#include <scorum/chain/services/hardfork_property.hpp>

#include <scorum/chain/schema/account_objects.hpp>
#include <scorum/chain/schema/dynamic_global_property_object.hpp>
#include <scorum/chain/schema/scorum_objects.hpp>
#include <scorum/chain/schema/dev_committee_object.hpp>
#include <scorum/chain/schema/budget_objects.hpp>

#include <scorum/chain/database/block_tasks/process_witness_reward_in_sp_migration.hpp>

#include <scorum/chain/advertising/advertising_auction.hpp>
#include <boost/range/adaptor/transformed.hpp>
#include <boost/range/algorithm/transform.hpp>
#include <boost/range/algorithm_ext/copy_n.hpp>
#include <scorum/utils/take_n_range.hpp>
#include <scorum/utils/collect_range_adaptor.hpp>

#include <vector>

namespace scorum {
namespace chain {
namespace database_ns {

using scorum::protocol::producer_reward_operation;

process_funds::process_funds(block_task_context& ctx)
    : _content_reward_service(ctx.services().content_reward_scr_service())
    , _dev_pool_service(ctx.services().dev_pool_service())
    , _dprops_service(ctx.services().dynamic_global_property_service())
    , _fund_budget_service(ctx.services().fund_budget_service())
    , _post_budget_service(ctx.services().post_budget_service())
    , _banner_budget_service(ctx.services().banner_budget_service())
    , _account_service(ctx.services().account_service())
    , _adv_property_svc(ctx.services().advertising_property_service())
    , _virt_op_emitter(ctx)
    , _voter_reward_scr_svc(ctx.services().voters_reward_scr_service())
    , _voter_reward_sp_svc(ctx.services().voters_reward_sp_service())
    , _content_reward_fund_scr_svc(ctx.services().content_reward_fund_scr_service())
    , _content_reward_fund_sp_svc(ctx.services().content_reward_fund_sp_service())
    , _hardfork_svc(ctx.services().hardfork_property_service())
    , _witness_svc(ctx.services().witness_service())
    , _ctx(ctx)
    , _block_num(ctx.block_num())
{
}

void process_funds::on_apply(block_task_context& ctx)
{
    if (apply_mainnet_schedule_crutches())
        return;

    debug_log(ctx.get_block_info(), "process_funds BEGIN");

    // We don't have inflation.
    // We just get per block reward from original reward fund(4.8M SP)
    // and expect that after initial supply is handed out(fund budget is over) reward budgets will be created by our
    // users(through the purchase of advertising). Advertising budgets are in SCR.

    asset original_fund_reward = asset(0, SP_SYMBOL);
    if (_fund_budget_service.is_exists())
    {
        original_fund_reward += _fund_budget_service.allocate_cash(_fund_budget_service.get());
    }
    distribute_reward(original_fund_reward); // distribute SP

    auto adv_budgets_reward = run_auction_round();

    // 50% of the revenue goes to support and develop the product, namely,
    // towards the company's R&D center.
    auto dev_team_reward_factor = utils::make_fraction(SCORUM_DEV_TEAM_PER_BLOCK_REWARD_PERCENT, SCORUM_100_PERCENT);
    auto dev_team_reward = adv_budgets_reward * dev_team_reward_factor;

    _dev_pool_service.update([&](dev_committee_object& dco) { dco.scr_balance += dev_team_reward; });

    // 50% of revenue is distributed in SCR among users.
    // pass it through reward balancer
    reward_balance_algorithm<content_reward_scr_service_i> balancer(_content_reward_service);
    balancer.increase_ballance(adv_budgets_reward - dev_team_reward);
    asset users_reward = balancer.take_block_reward();

    distribute_reward(users_reward); // distribute SCR

    debug_log(ctx.get_block_info(), "process_funds END");
}

asset process_funds::run_auction_round()
{
    advertising_auction auction(_dprops_service, _adv_property_svc, _post_budget_service, _banner_budget_service);
    auction.run_round();

    auto post_budgets_reward = process_adv_pending_payouts(_post_budget_service);
    auto banner_budgets_reward = process_adv_pending_payouts(_banner_budget_service);
    auto adv_budgets_reward = post_budgets_reward + banner_budgets_reward;

    close_empty_budgets(_post_budget_service);
    close_empty_budgets(_banner_budget_service);

    return adv_budgets_reward;
}

template <budget_type budget_type_v>
asset process_funds::process_adv_pending_payouts(adv_budget_service_i<budget_type_v>& budget_svc)
{
    auto pending_budgets = budget_svc.get_pending_budgets();

    std::vector<budget_outgo_operation> allocated_cash_ops;
    std::vector<budget_owner_income_operation> returned_cash_ops;
    for (const adv_budget_object<budget_type_v>& budget : pending_budgets)
    {
        if (budget.budget_pending_outgo.amount > 0)
            allocated_cash_ops.emplace_back(budget_type_v, budget.owner, budget.id._id, budget.budget_pending_outgo);

        if (budget.owner_pending_income.amount > 0)
            returned_cash_ops.emplace_back(budget_type_v, budget.owner, budget.id._id, budget.owner_pending_income);
    }

    auto adv_budgets_reward = budget_svc.perform_pending_payouts(pending_budgets);

    for (const auto& op : allocated_cash_ops)
        _virt_op_emitter.push_virtual_operation(op);

    for (const auto& op : returned_cash_ops)
        _virt_op_emitter.push_virtual_operation(op);

    return adv_budgets_reward;
}

template <budget_type budget_type_v>
void process_funds::close_empty_budgets(adv_budget_service_i<budget_type_v>& budget_svc)
{
    auto empty_budgets = budget_svc.get_empty_budgets();

    for (const auto& b : empty_budgets)
    {
        _virt_op_emitter.push_virtual_operation(budget_closing_operation(budget_type_v, b.get().owner, b.get().id._id));

        budget_svc.remove(b);
    }
}

void process_funds::distribute_reward(const asset& users_reward)
{
    // clang-format off
    /// 5% of total per block reward(equal to 10% of users only reward) to witness and active sp holder pay
    asset witness_reward = users_reward * utils::make_fraction(SCORUM_WITNESS_PER_BLOCK_REWARD_PERCENT, SCORUM_100_PERCENT);
    asset active_sp_holder_reward = users_reward  * utils::make_fraction(SCORUM_ACTIVE_SP_HOLDERS_PER_BLOCK_REWARD_PERCENT ,SCORUM_100_PERCENT);
    asset content_reward = users_reward - witness_reward - active_sp_holder_reward;
    // clang-format on

    process_witness_reward_in_sp_migration().adjust_witness_reward(_ctx, witness_reward);

    FC_ASSERT(content_reward.amount >= 0, "content_reward(${r}) must not be less zero", ("r", content_reward));

    distribute_witness_reward(witness_reward);

    distribute_active_sp_holders_reward(active_sp_holder_reward);

    pay_content_reward(content_reward);
}

void process_funds::distribute_witness_reward(const asset& witness_reward)
{
    const auto& cwit = _witness_svc.get(_dprops_service.get().current_witness);

    if (cwit.schedule != witness_object::top20 && cwit.schedule != witness_object::timeshare)
    {
        wlog("Encountered unknown witness type for witness: ${w}", ("w", cwit.owner));
    }

    const auto& witness = _account_service.get_account(cwit.owner);

    pay_witness_reward(witness, witness_reward);

    if (witness_reward.amount != 0)
        _virt_op_emitter.push_virtual_operation(producer_reward_operation(witness.name, witness_reward));
}

void process_funds::distribute_active_sp_holders_reward(const asset& reward)
{
    asset total_reward = get_activity_reward(reward);

    asset distributed_reward = asset(0, reward.symbol());

    auto active_sp_holders_array = _account_service.get_active_sp_holders();
    if (!active_sp_holders_array.empty())
    {
        // distribute
        asset total_sp = std::accumulate(active_sp_holders_array.begin(), active_sp_holders_array.end(),
                                         asset(0, SP_SYMBOL), [&](asset& accumulator, const account_object& account) {
                                             return accumulator += account.vote_reward_competitive_sp;
                                         });

        if (total_sp.amount > 0)
        {
            active_sp_holders_reward_legacy_operation::rewarded_type rewarded;
            for (const account_object& account : active_sp_holders_array)
            {
                // It is used SP balance amount of account to calculate reward either in SP or SCR tokens
                asset account_reward
                    = total_reward * utils::make_fraction(account.vote_reward_competitive_sp.amount, total_sp.amount);

                if (_hardfork_svc.has_hardfork(SCORUM_HARDFORK_0_2))
                {
                    pay_account_pending_reward(account, account_reward);
                }
                else
                {
                    pay_account_reward(account, account_reward);
                }

                distributed_reward += account_reward;

                if (!_hardfork_svc.has_hardfork(SCORUM_HARDFORK_0_2) && account_reward.amount > 0)
                {
                    rewarded[account.name] = account_reward;
                }
            }

            if (!_hardfork_svc.has_hardfork(SCORUM_HARDFORK_0_2) && !rewarded.empty())
            {
                _virt_op_emitter.push_virtual_operation(
                    active_sp_holders_reward_legacy_operation{ std::move(rewarded) });
            }
        }
    }

    // put undistributed money in special fund
    pay_activity_reward(total_reward - distributed_reward);
}

void process_funds::pay_account_reward(const account_object& account, const asset& reward)
{
    if (reward.amount <= 0)
        return;

    if (reward.symbol() == SCORUM_SYMBOL)
    {
        _account_service.increase_balance(account, reward);
    }
    else
    {
        _account_service.create_scorumpower(account, reward);
    }
}

void process_funds::pay_account_pending_reward(const account_object& account, const asset& reward)
{
    if (reward.amount <= 0)
        return;

    if (reward.symbol() == SCORUM_SYMBOL)
    {
        _account_service.increase_pending_balance(account, reward);
    }
    else
    {
        _account_service.increase_pending_scorumpower(account, reward);
    }
}

void process_funds::pay_witness_reward(const account_object& witness, const asset& reward)
{
    if (reward.symbol() == SCORUM_SYMBOL)
    {
        _dprops_service.update([&](dynamic_global_property_object& p) { p.total_witness_reward_scr += reward; });
    }
    else
    {
        _dprops_service.update([&](dynamic_global_property_object& p) { p.total_witness_reward_sp += reward; });
    }

    pay_account_reward(witness, reward);
}

void process_funds::pay_content_reward(const asset& reward)
{
    if (reward.amount <= 0)
        return;

    if (reward.symbol() == SCORUM_SYMBOL)
    {
        _content_reward_fund_scr_svc.update(
            [&](content_reward_fund_scr_object& rfo) { rfo.activity_reward_balance += reward; });
    }
    else
    {
        _content_reward_fund_sp_svc.update(
            [&](content_reward_fund_sp_object& rfo) { rfo.activity_reward_balance += reward; });
    }
}

void process_funds::pay_activity_reward(const asset& reward)
{
    if (reward.amount <= 0)
        return;

    if (reward.symbol() == SCORUM_SYMBOL)
    {
        reward_balance_algorithm<voters_reward_scr_service_i> balancer(_voter_reward_scr_svc);
        balancer.increase_ballance(reward);
    }
    else
    {
        reward_balance_algorithm<voters_reward_sp_service_i> balancer(_voter_reward_sp_svc);
        balancer.increase_ballance(reward);
    }
}

const asset process_funds::get_activity_reward(const asset& reward)
{
    if (reward.symbol() == SCORUM_SYMBOL)
    {
        reward_balance_algorithm<voters_reward_scr_service_i> balancer(_voter_reward_scr_svc);
        return reward + balancer.take_block_reward();
    }
    else
    {
        reward_balance_algorithm<voters_reward_sp_service_i> balancer(_voter_reward_sp_svc);
        return reward + balancer.take_block_reward();
    }
}

bool process_funds::apply_mainnet_schedule_crutches()
{
    // We have bug on mainnet: signed_block was applied, but undo_session wasn't pushed, therefore DB state roll backed
    // and witnesses were not rewarded. It leads us to mismatching of schedule for working and newly synced nodes. Next
    // code fixes it.
    if (_block_num == 1650380 || // fix reward for headshot witness
        _block_num == 1808664) // fix reward for addit-yury witness
    {
        return true;
    }

    return false;
}
}
}
}
