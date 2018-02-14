#include <scorum/chain/genesis/initializators/registration_bonus_initializator.hpp>
#include <scorum/chain/data_service_factory.hpp>

#include <scorum/chain/services/account.hpp>
#include <scorum/chain/services/registration_pool.hpp>

#include <scorum/chain/schema/account_objects.hpp>

#include <scorum/chain/genesis/genesis_state.hpp>

namespace scorum {
namespace chain {
namespace genesis {

void registration_bonus_initializator_impl::apply(data_service_factory_i& services,
                                                  const genesis_state_type& genesis_state)
{
    registration_pool_service_i& registration_pool_service = services.registration_pool_service();

    if (registration_pool_service.is_exists())
    {
        account_service_i& account_service = services.account_service();

        size_t total_unvested = genesis_state.accounts.size();
        for (auto& account : genesis_state.accounts)
        {
            const auto& account_obj = account_service.get_account(account.name);
            asset bonus = allocate_cash(services);
            if (bonus.amount > (share_value_type)0)
            {
                account_service.create_vesting(account_obj, bonus);
                --total_unvested;
            }
            else
            {
                wlog("No more bonus from registration pool. ${1} accounts have no SP.", ("1", total_unvested));
                break;
            }
        }
    }
}

asset registration_bonus_initializator_impl::allocate_cash(data_service_factory_i& services)
{
    registration_pool_service_i& registration_pool_service = services.registration_pool_service();

    if (!registration_pool_service.is_exists())
        return asset(0, SCORUM_SYMBOL);

    asset per_reg = registration_pool_service.calculate_per_reg();
    FC_ASSERT(per_reg.amount > 0, "Invalid schedule. Zero bonus return.");

    // return value <= per_reg
    per_reg = registration_pool_service.decrease_balance(per_reg);

    if (!registration_pool_service.check_autoclose())
    {
        registration_pool_service.increase_already_allocated_count();
    }

    return per_reg;
}
}
}
}
