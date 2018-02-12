#include <scorum/chain/genesis/initializators/founders_initializator.hpp>
#include <scorum/chain/data_service_factory.hpp>

#include <fc/exception/exception.hpp>

#include <scorum/chain/services/dynamic_global_property.hpp>
#include <scorum/chain/services/account.hpp>

#include <scorum/chain/schema/account_objects.hpp>

#include <scorum/chain/genesis/genesis_state.hpp>

#include <limits>

namespace scorum {
namespace chain {
namespace genesis {

namespace {
uint16_t get_percent(float sp_percent)
{
    float ret = sp_percent * SCORUM_1_PERCENT;
    return (uint16_t)ret; // remove mantissa
}
}

void founders_initializator_impl::apply(data_service_factory_i& services, const genesis_state_type& genesis_state)
{
    FC_ASSERT(SCORUM_100_PERCENT <= std::numeric_limits<uint16_t>::max()); // constant value check

    account_service_i& account_service = services.account_service();
    dynamic_global_property_service_i& dgp_service = services.dynamic_global_property_service();

    asset founders_supply = genesis_state.founders_supply;

    FC_ASSERT(founders_supply.symbol() == VESTS_SYMBOL);

    uint16_t total_sp_percent = (uint16_t)0;
    for (auto& founder : genesis_state.founders)
    {
        FC_ASSERT(!founder.name.empty(), "Founder 'name' should not be empty.");

        account_service.check_account_existence(founder.name);

        FC_ASSERT(founder.sp_percent >= 0.f && founder.sp_percent <= 100.f,
                  "Founder 'sp_percent' should be in range [0, 100]. ${1} received.", ("1", founder.sp_percent));

        total_sp_percent += get_percent(founder.sp_percent);

        FC_ASSERT(total_sp_percent <= (uint16_t)SCORUM_100_PERCENT, "Total 'sp_percent' more then 100%.");
    }

    asset founders_supply_rest = founders_supply;
    account_name_type pitiful;
    for (auto& founder : genesis_state.founders)
    {
        uint16_t percent = get_percent(founder.sp_percent);

        const auto& founder_obj = account_service.get_account(founder.name);
        asset sp_bonus = founders_supply;
        sp_bonus.amount *= percent;
        sp_bonus.amount /= SCORUM_100_PERCENT;
        if (sp_bonus.amount > (share_value_type)0)
        {
            account_service.increase_vesting_shares(founder_obj, sp_bonus);
            dgp_service.update([&](dynamic_global_property_object& props) { props.total_vesting_shares += sp_bonus; });
            founders_supply_rest -= sp_bonus;
        }
        else if (founder.sp_percent > 0.f)
        {
            pitiful = founder_obj.name; // only last pitiful may get SP
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
            founders_supply_rest.amount = 0;
        }
    }

    FC_ASSERT(!founders_supply_rest.amount.value);
}
}
}
}
