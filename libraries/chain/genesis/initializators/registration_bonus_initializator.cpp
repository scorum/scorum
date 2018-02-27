#include <scorum/chain/genesis/initializators/registration_bonus_initializator.hpp>
#include <scorum/chain/data_service_factory.hpp>

#include <scorum/chain/services/account.hpp>
#include <scorum/chain/services/registration_pool.hpp>

#include <scorum/chain/schema/account_objects.hpp>

#include <scorum/chain/evaluators/registration_pool.hpp>

#include <scorum/chain/genesis/genesis_state.hpp>

namespace scorum {
namespace chain {
namespace genesis {

void registration_bonus_initializator_impl::on_apply(initializator_context& ctx)
{
    registration_pool_service_i& registration_pool_service = ctx.services().registration_pool_service();

    if (registration_pool_service.is_exists())
    {
        account_service_i& account_service = ctx.services().account_service();

        size_t total_unvested = ctx.genesis_state().accounts.size();
        for (auto& account : ctx.genesis_state().accounts)
        {
            const auto& account_obj = account_service.get_account(account.name);
            registration_pool_context ctx(ctx.services(), account_obj);
            registration_pool_task allocate_cash;
            allocate_cash.apply(ctx);
            if (ctx.last_result())
            {
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
}
}
}
