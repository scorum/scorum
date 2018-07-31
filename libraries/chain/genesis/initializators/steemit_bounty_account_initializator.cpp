#include <scorum/chain/genesis/initializators/steemit_bounty_account_initializator.hpp>
#include <scorum/chain/data_service_factory.hpp>

#include <scorum/chain/services/dynamic_global_property.hpp>
#include <scorum/chain/services/account.hpp>

#include <scorum/chain/schema/account_objects.hpp>
#include <scorum/chain/schema/dynamic_global_property_object.hpp>

#include <scorum/chain/genesis/genesis_state.hpp>

#include <fc/exception/exception.hpp>

namespace scorum {
namespace chain {
namespace genesis {

void steemit_bounty_account_initializator_impl::on_apply(initializator_context& ctx)
{
    if (!is_steemit_pool_exists(ctx))
    {
        dlog("There is no steemit bounty account supply.");
        return;
    }

    account_service_i& account_service = ctx.services().account_service();

    FC_ASSERT(ctx.genesis_state().steemit_bounty_accounts_supply.symbol() == SP_SYMBOL);

    check_accounts(ctx);

    for (auto& account : ctx.genesis_state().steemit_bounty_accounts)
    {
        const auto& account_obj = account_service.get_account(account.name);

        account_service.increase_scorumpower(account_obj, account.sp_amount);
    }
}

bool steemit_bounty_account_initializator_impl::is_steemit_pool_exists(initializator_context& ctx)
{
    return ctx.genesis_state().steemit_bounty_accounts_supply.amount.value
        || !ctx.genesis_state().steemit_bounty_accounts.empty();
}

void steemit_bounty_account_initializator_impl::check_accounts(initializator_context& ctx)
{
    account_service_i& account_service = ctx.services().account_service();

    asset sp_accounts_supply = ctx.genesis_state().steemit_bounty_accounts_supply;

    for (auto& account : ctx.genesis_state().steemit_bounty_accounts)
    {
        FC_ASSERT(!account.name.empty(), "Account 'name' should not be empty.");

        account_service.check_account_existence(account.name);

        FC_ASSERT(account.sp_amount.symbol() == SP_SYMBOL, "Invalid asset symbol for '${1}'.", ("1", account.name));

        FC_ASSERT(account.sp_amount.amount > (share_value_type)0, "Invalid asset amount for '${1}'.",
                  ("1", account.name));

        sp_accounts_supply -= account.sp_amount;
    }

    FC_ASSERT(sp_accounts_supply.amount == (share_value_type)0,
              "'steemit_bounty_accounts_supply' must be sum of all steemit_bounty_accounts supply.");
}
}
}
}
