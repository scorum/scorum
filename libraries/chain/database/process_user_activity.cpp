#include <scorum/chain/database/process_user_activity.hpp>

#include <scorum/account_identity/owned.hpp>

#include <scorum/chain/services/account_registration_bonus.hpp>

namespace scorum {
namespace chain {

namespace database_ns {

using scorum::protocol::account_name_type;

user_activity_context::user_activity_context(data_service_factory_i& services, const signed_transaction& trx)
    : _services(services)
    , _trx(trx)
{
}

void process_user_activity_task::on_apply(user_activity_context& ctx)
{
    fc::flat_set<account_name_type> result;
    account_identity::transaction_get_owned_accounts(ctx.transaction(), result);

    account_registration_bonus_service_i& account_registration_bonus_service
        = ctx.services().account_registration_bonus_service();

    for (const account_name_type& name : result)
    {
        account_registration_bonus_service.remove_if_exist(name);
    }
}
}
}
}
