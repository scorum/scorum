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

void accounts_initializator::apply(data_service_factory_i& services, const genesis_state_type& genesis_state)
{
    account_service_i& account_service = services.account_service();

    asset accounts_supply = genesis_state.accounts_supply;
    for (auto& account : genesis_state.accounts)
    {
        FC_ASSERT(!account.name.empty(), "Account 'name' should not be empty.");

        accounts_supply -= account.scr_amount;
    }

    FC_ASSERT(accounts_supply.amount == (share_value_type)0, "'accounts_supply' must be sum of all accounts supply.");

    for (auto& account : genesis_state.accounts)
    {
        FC_ASSERT(!account.name.empty(), "Account 'name' should not be empty.");

        account_service.create_initial_account(account.name, account.public_key, account.scr_amount,
                                               account.recovery_account, "{\"created_at\": \"GENESIS\"}");
    }
}
}
}
}
