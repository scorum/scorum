#include <scorum/chain/genesis/initializators/global_property_initializator.hpp>
#include <scorum/chain/data_service_factory.hpp>

#include <scorum/chain/services/dynamic_global_property.hpp>

#include <scorum/chain/genesis/genesis_state.hpp>

namespace scorum {
namespace chain {
namespace genesis {

void global_property_initializator_impl::apply(data_service_factory_i& services,
                                               const genesis_state_type& genesis_state)
{
    dynamic_global_property_service_i& dgp_service = services.dynamic_global_property_service();

    FC_ASSERT(genesis_state.founders_supply.symbol() == VESTS_SYMBOL);
    FC_ASSERT(genesis_state.steemit_bounty_accounts_supply.symbol() == VESTS_SYMBOL);
    FC_ASSERT(genesis_state.accounts_supply.symbol() == SCORUM_SYMBOL);
    FC_ASSERT(genesis_state.rewards_supply.symbol() == SCORUM_SYMBOL);
    FC_ASSERT(genesis_state.registration_supply.symbol() == SCORUM_SYMBOL);

    FC_ASSERT(!dgp_service.is_exists());

    dgp_service.create([&](dynamic_global_property_object& gpo) {
        gpo.time = dgp_service.get_genesis_time();
        gpo.recent_slots_filled = fc::uint128::max_value();
        gpo.participation_count = 128;
        auto founders_supply = genesis_state.founders_supply.amount;
        auto steemit_bounty_accounts_supply = genesis_state.steemit_bounty_accounts_supply.amount;
        gpo.circulating_capital = genesis_state.accounts_supply;
        gpo.circulating_capital += asset(founders_supply, SCORUM_SYMBOL);
        gpo.circulating_capital += asset(steemit_bounty_accounts_supply, SCORUM_SYMBOL);
        gpo.total_supply = gpo.circulating_capital;
        gpo.total_supply += genesis_state.rewards_supply;
        gpo.total_supply += genesis_state.registration_supply;
        gpo.median_chain_props.maximum_block_size = SCORUM_MAX_BLOCK_SIZE;
    });
}
}
}
}
