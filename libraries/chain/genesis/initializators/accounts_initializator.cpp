#include <scorum/chain/genesis/initializators/accounts_initializator.hpp>
#include <scorum/chain/data_service_factory.hpp>

#include <scorum/chain/services/account.hpp>

#include <scorum/protocol/asset.hpp>

#include <scorum/chain/genesis/genesis_state.hpp>

#include <fc/exception/exception.hpp>

namespace scorum {
namespace chain {
namespace genesis {

using scorum::protocol::asset;
using scorum::protocol::share_value_type;

void accounts_initializator_impl::on_apply(initializator_context& ctx)
{
    account_service_i& account_service = ctx.services.account_service();

    check_accounts_supply(ctx);

    for (auto& account : ctx.genesis_state.accounts)
    {
        FC_ASSERT(!account.name.empty(), "Account 'name' should not be empty.");

        account_service.create_initial_account(account.name, account.public_key, account.scr_amount,
                                               account.recovery_account, R"({"created_at": "GENESIS"})");
    }
}

void accounts_initializator_impl::check_accounts_supply(initializator_context& ctx)
{
    asset accounts_supply = ctx.genesis_state.accounts_supply;

    FC_ASSERT(accounts_supply.symbol() == SCORUM_SYMBOL);

    for (auto& account : ctx.genesis_state.accounts)
    {
        FC_ASSERT(!account.name.empty(), "Account 'name' should not be empty.");

        FC_ASSERT(account.scr_amount.symbol() == SCORUM_SYMBOL, "Invalid asset symbol for '${1}'.",
                  ("1", account.name));

        accounts_supply -= account.scr_amount;
    }

    FC_ASSERT(accounts_supply.amount == (share_value_type)0, "'accounts_supply' must be sum of all accounts supply.");
}
}
}
}
