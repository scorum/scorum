#include <scorum/chain/genesis/initializators/global_property_initializator.hpp>
#include <scorum/chain/data_service_factory.hpp>

#include <scorum/chain/services/dynamic_global_property.hpp>

#include <scorum/chain/genesis/genesis_state.hpp>

namespace scorum {
namespace chain {
namespace genesis {

void global_property_initializator_impl::on_apply(initializator_context& ctx)
{
    dynamic_global_property_service_i& dgp_service = ctx.services().dynamic_global_property_service();

    FC_ASSERT(ctx.genesis_state().total_supply.symbol() == SCORUM_SYMBOL);
    FC_ASSERT(ctx.genesis_state().founders_supply.symbol() == SP_SYMBOL);
    FC_ASSERT(ctx.genesis_state().steemit_bounty_accounts_supply.symbol() == SP_SYMBOL);
    FC_ASSERT(ctx.genesis_state().accounts_supply.symbol() == SCORUM_SYMBOL);
    FC_ASSERT(ctx.genesis_state().rewards_supply.symbol() == SCORUM_SYMBOL);
    FC_ASSERT(ctx.genesis_state().registration_supply.symbol() == SCORUM_SYMBOL);
    FC_ASSERT(ctx.genesis_state().development_sp_supply.symbol() == SP_SYMBOL);
    FC_ASSERT(ctx.genesis_state().development_scr_supply.symbol() == SCORUM_SYMBOL);

    FC_ASSERT(!dgp_service.is_exists());

    asset circulating_capital = asset(0, SCORUM_SYMBOL);
    circulating_capital += ctx.genesis_state().accounts_supply;

    auto founders_supply = ctx.genesis_state().founders_supply.amount;
    auto steemit_bounty_accounts_supply = ctx.genesis_state().steemit_bounty_accounts_supply.amount;

    asset total_supply = asset(0, SCORUM_SYMBOL);
    total_supply += circulating_capital;
    total_supply += ctx.genesis_state().rewards_supply;
    total_supply += ctx.genesis_state().registration_supply;
    total_supply += asset(ctx.genesis_state().development_sp_supply.amount, SCORUM_SYMBOL);
    total_supply += asset(ctx.genesis_state().development_scr_supply.amount, SCORUM_SYMBOL);
    total_supply += asset(founders_supply, SCORUM_SYMBOL);
    total_supply += asset(steemit_bounty_accounts_supply, SCORUM_SYMBOL);

    FC_ASSERT(ctx.genesis_state().total_supply == total_supply, "Invalid total supply: ${gt} != ${rt}",
              ("gt", ctx.genesis_state().total_supply)("rt", total_supply));

    dgp_service.create([&](dynamic_global_property_object& gpo) {
        gpo.time = dgp_service.get_genesis_time();
        gpo.recent_slots_filled = fc::uint128::max_value();
        gpo.participation_count = 128;
        gpo.circulating_capital = circulating_capital;
        gpo.total_supply = total_supply;
        gpo.median_chain_props.maximum_block_size = SCORUM_MAX_BLOCK_SIZE;
    });
}
}
}
}
