#include <scorum/chain/genesis/initializators/dev_pool_initializator.hpp>
#include <scorum/chain/data_service_factory.hpp>

#include <fc/exception/exception.hpp>

#include <scorum/chain/services/dynamic_global_property.hpp>
#include <scorum/chain/services/dev_pool.hpp>

#include <scorum/chain/schema/dynamic_global_property_object.hpp>
#include <scorum/chain/schema/dev_committee_object.hpp>

#include <scorum/chain/genesis/genesis_state.hpp>

#include <scorum/protocol/scorum_operations.hpp>

#include <limits>

namespace scorum {
namespace chain {
namespace genesis {

void dev_pool_initializator_impl::on_apply(initializator_context& ctx)
{
    if (!is_zero_supply(ctx))
    {
        dlog("Development pool supply is zero.");
    }

    FC_ASSERT(ctx.genesis_state().development_sp_supply.symbol() == VESTS_SYMBOL);
    FC_ASSERT(ctx.genesis_state().development_scr_supply.symbol() == SCORUM_SYMBOL);

    create_dev_committee_and_set_pool_balance(ctx);

    increase_total_supply(ctx);
}

bool dev_pool_initializator_impl::is_zero_supply(initializator_context& ctx)
{
    return ctx.genesis_state().development_sp_supply.amount.value
        || ctx.genesis_state().development_scr_supply.amount.value;
}

void dev_pool_initializator_impl::create_dev_committee_and_set_pool_balance(initializator_context& ctx)
{
    dev_pool_service_i& dev_pool_service = ctx.services().dev_pool_service();

    FC_ASSERT(!dev_pool_service.is_exists());

    dev_pool_service.create([&](dev_committee_object& pool) {
        pool.sp_balance = ctx.genesis_state().development_sp_supply;
        pool.scr_balance = ctx.genesis_state().development_scr_supply;
    });
}

void dev_pool_initializator_impl::increase_total_supply(initializator_context& ctx)
{
    dynamic_global_property_service_i& dgp_service = ctx.services().dynamic_global_property_service();

    dgp_service.update([&](dynamic_global_property_object& props) {
        props.total_supply += asset(ctx.genesis_state().development_sp_supply.amount, SCORUM_SYMBOL);
        props.total_supply += asset(ctx.genesis_state().development_scr_supply.amount, SCORUM_SYMBOL);
    });
}

} // namespace genesis
} // namespace chain
} // namespace scorum
