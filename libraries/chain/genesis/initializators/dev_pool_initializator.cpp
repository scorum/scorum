#include <scorum/chain/genesis/initializators/dev_pool_initializator.hpp>
#include <scorum/chain/data_service_factory.hpp>

#include <scorum/chain/services/dynamic_global_property.hpp>
#include <scorum/chain/services/dev_pool.hpp>

#include <scorum/chain/schema/dev_committee_object.hpp>

#include <scorum/chain/genesis/genesis_state.hpp>

namespace scorum {
namespace chain {
namespace genesis {

void dev_pool_initializator_impl::on_apply(initializator_context& ctx)
{
    FC_ASSERT(ctx.genesis_state().development_sp_supply.symbol() == SP_SYMBOL);
    FC_ASSERT(ctx.genesis_state().development_scr_supply.symbol() == SCORUM_SYMBOL);

    setup_dev_pool(ctx);
}

void dev_pool_initializator_impl::setup_dev_pool(initializator_context& ctx)
{
    dev_pool_service_i& dev_pool_service = ctx.services().dev_pool_service();

    dev_pool_service.create([&](dev_committee_object& pool) {
        pool.sp_balance = ctx.genesis_state().development_sp_supply;
        pool.scr_balance = ctx.genesis_state().development_scr_supply;
        pool.top_budgets_amounts.insert(std::make_pair(budget_type::post, (uint16_t)SCORUM_DEFAULT_TOP_BUDGETS_AMOUNT));
        pool.top_budgets_amounts.insert(
            std::make_pair(budget_type::banner, (uint16_t)SCORUM_DEFAULT_TOP_BUDGETS_AMOUNT));
    });
}

} // namespace genesis
} // namespace chain
} // namespace scorum
