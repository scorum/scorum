#include <scorum/chain/genesis/initializators/rewards_initializator.hpp>
#include <scorum/chain/data_service_factory.hpp>

#include <scorum/chain/services/dynamic_global_property.hpp>
#include <scorum/chain/services/reward_funds.hpp>
#include <scorum/chain/services/reward_balancer.hpp>
#include <scorum/chain/services/budget.hpp>

#include <scorum/chain/schema/scorum_objects.hpp>
#include <scorum/chain/schema/budget_object.hpp>

#include <scorum/chain/genesis/genesis_state.hpp>

namespace scorum {
namespace chain {
namespace genesis {

void rewards_initializator_impl::on_apply(initializator_context& ctx)
{
    FC_ASSERT(ctx.genesis_state().rewards_supply.symbol() == SCORUM_SYMBOL);

    create_scr_reward_fund(ctx);
    create_sp_reward_fund(ctx);
    create_balancer(ctx);
    create_fund_budget(ctx);
}

void rewards_initializator_impl::create_scr_reward_fund(initializator_context& ctx)
{
    reward_fund_scr_service_i& reward_fund_service = ctx.services().reward_fund_scr_service();
    dynamic_global_property_service_i& dgp_service = ctx.services().dynamic_global_property_service();

    FC_ASSERT(!reward_fund_service.is_exists());

    reward_fund_service.create([&](reward_fund_scr_object& rfo) {
        rfo.last_update = dgp_service.head_block_time();
        rfo.author_reward_curve = curve_id::linear;
        rfo.curation_reward_curve = curve_id::square_root;
    });
}

void rewards_initializator_impl::create_sp_reward_fund(initializator_context& ctx)
{
    reward_fund_sp_service_i& reward_fund_service = ctx.services().reward_fund_sp_service();
    dynamic_global_property_service_i& dgp_service = ctx.services().dynamic_global_property_service();

    FC_ASSERT(!reward_fund_service.is_exists());

    reward_fund_service.create([&](reward_fund_sp_object& rfo) {
        rfo.last_update = dgp_service.head_block_time();
        rfo.author_reward_curve = curve_id::linear;
        rfo.curation_reward_curve = curve_id::square_root;
    });
}

void rewards_initializator_impl::create_balancer(initializator_context& ctx)
{
    reward_service_i& reward_service = ctx.services().reward_service();

    FC_ASSERT(!reward_service.is_exists());

    reward_service.create_balancer(asset(0, SCORUM_SYMBOL));
}

void rewards_initializator_impl::create_fund_budget(initializator_context& ctx)
{
    dynamic_global_property_service_i& dgp_service = ctx.services().dynamic_global_property_service();
    budget_service_i& budget_service = ctx.services().budget_service();

    FC_ASSERT(!budget_service.is_fund_budget_exists());

    fc::time_point deadline = dgp_service.get_genesis_time() + fc::days(SCORUM_REWARDS_INITIAL_SUPPLY_PERIOD_IN_DAYS);

    budget_service.create_fund_budget(ctx.genesis_state().rewards_supply, deadline);
}
}
}
}
