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

    FC_ASSERT(!dgp_service.is_exists());

    dgp_service.create([&](dynamic_global_property_object& gpo) {
        gpo.time = dgp_service.get_genesis_time();
        gpo.recent_slots_filled = fc::uint128::max_value();
        gpo.participation_count = 128;
        asset founders_supply = genesis_state.founders_supply;
        gpo.circulating_capital = genesis_state.accounts_supply + asset(founders_supply.amount, SCORUM_SYMBOL);
        gpo.total_supply = gpo.circulating_capital + genesis_state.rewards_supply + genesis_state.registration_supply;
        gpo.median_chain_props.maximum_block_size = SCORUM_MAX_BLOCK_SIZE;
    });
}
}
}
}
