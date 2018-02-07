#include <scorum/chain/genesis/initializators/founders_initializator.hpp>
#include <scorum/chain/data_service_factory.hpp>

#include <fc/exception/exception.hpp>

#include <scorum/chain/services/dynamic_global_property.hpp>
#include <scorum/chain/services/account.hpp>

#include <scorum/chain/schema/account_objects.hpp>

#include <scorum/chain/genesis/genesis_state.hpp>

namespace scorum {
namespace chain {
namespace genesis {

void founders_initializator::apply(data_service_factory_i& services, const genesis_state_type& genesis_state)
{
    dynamic_global_property_service_i& dgp_service = services.dynamic_global_property_service();
    account_service_i& account_service = services.account_service();

    asset founders_supply = genesis_state.founders_supply;
    uint16_t total_sp_percent = (uint16_t)0;
    for (auto& founder : genesis_state.founders)
    {
        FC_ASSERT(!founder.name.empty(), "Founder 'name' should not be empty.");
        account_service.check_account_existence(founder.name);
        FC_ASSERT(founder.sp_percent >= (uint16_t)0 && founder.sp_percent <= (uint16_t)100,
                  "Founder 'sp_percent' should be in range [0, 100]. ${1} received.", ("1", founder.sp_percent));
        FC_ASSERT(founder.sp_percent == (uint16_t)0 || founders_supply.amount > (share_value_type)0,
                  "Empty 'founders_supply'.");

        total_sp_percent += founder.sp_percent;
        if (total_sp_percent >= (uint16_t)100)
            break;
    }

    FC_ASSERT(genesis_state.founders.empty() || total_sp_percent == (uint16_t)100, "Total 'sp_percent' must be 100%.");

    asset founders_supply_rest = founders_supply;
    account_name_type pitiful;
    for (auto& founder : genesis_state.founders)
    {
        if (founder.sp_percent == (uint16_t)0)
            continue;

        const auto& founder_obj = account_service.get_account(founder.name);
        asset sp_bonus(0, VESTS_SYMBOL);
        sp_bonus.amount *= founder.sp_percent * SCORUM_1_PERCENT;
        sp_bonus.amount /= SCORUM_100_PERCENT;
        if (sp_bonus.amount > (share_value_type)0)
        {
            account_service.increase_vesting_shares(founder_obj, sp_bonus);
            dgp_service.update([&](dynamic_global_property_object& props) { props.total_vesting_shares += sp_bonus; });
            founders_supply_rest -= sp_bonus;
        }
        else if (founder.sp_percent > (uint16_t)0)
        {
            pitiful = founder_obj.name;
        }
    }

    if (founders_supply_rest.amount > (share_value_type)0)
    {
        if (pitiful != account_name_type())
        {
            const auto& founder_obj = account_service.get_account(pitiful);
            account_service.increase_vesting_shares(founder_obj, founders_supply_rest);
            dgp_service.update(
                [&](dynamic_global_property_object& props) { props.total_vesting_shares += founders_supply_rest; });
        }
    }

    FC_ASSERT(!founders_supply_rest.amount.value);
}
}
}
}
