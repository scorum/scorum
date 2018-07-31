#include <scorum/chain/database/block_tasks/process_account_registration_bonus_expiration.hpp>

#include <scorum/chain/services/account_registration_bonus.hpp>
#include <scorum/chain/services/dynamic_global_property.hpp>
#include <scorum/chain/services/registration_pool.hpp>
#include <scorum/chain/services/account.hpp>

#include <scorum/chain/schema/registration_objects.hpp>

namespace scorum {
namespace chain {
namespace database_ns {

void process_account_registration_bonus_expiration::on_apply(block_task_context& ctx)
{
    debug_log(ctx.get_block_info(), "process_account_registration_bonus_expiration BEGIN");

    account_registration_bonus_service_i& account_registration_bonus_service
        = ctx.services().account_registration_bonus_service();
    dynamic_global_property_service_i& dgp_service = ctx.services().dynamic_global_property_service();

    const auto& accounts = account_registration_bonus_service.get_by_expiration_time(dgp_service.head_block_time());
    for (const account_registration_bonus_object& account : accounts)
    {
        return_funds(ctx, account);
        account_registration_bonus_service.remove(account);
    }

    debug_log(ctx.get_block_info(), "process_account_registration_bonus_expiration END");
}

void process_account_registration_bonus_expiration::return_funds(block_task_context& ctx,
                                                                 const account_registration_bonus_object& account)
{
    registration_pool_service_i& registration_pool_service = ctx.services().registration_pool_service();
    account_service_i& account_service = ctx.services().account_service();

    asset bonus = account.bonus;

    const account_object& account_obj = account_service.get_account(account.account);

    asset actual_returned_bonus
        = std::max(asset(0, SP_SYMBOL), std::min(bonus, account_obj.scorumpower - account_obj.delegated_scorumpower));
    if (actual_returned_bonus < bonus)
    {
        wlog("Account '${a}' has insufficient funds to return scorumpower ${f}. Actually returned is ${r}.",
             ("a", account_obj.name)("f", bonus)("r", actual_returned_bonus));
    }

    account_service.decrease_scorumpower(account_obj, actual_returned_bonus);

    registration_pool_service.update(
        [&](registration_pool_object& r) { r.balance += asset(actual_returned_bonus.amount, SCORUM_SYMBOL); });
}
}
}
}
