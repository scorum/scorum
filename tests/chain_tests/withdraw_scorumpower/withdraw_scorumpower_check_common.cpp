#include "withdraw_scorumpower_check_common.hpp"

#include <scorum/chain/services/account.hpp>
#include <scorum/chain/services/dev_pool.hpp>
#include <scorum/chain/services/withdraw_scorumpower.hpp>
#include <scorum/chain/services/withdraw_scorumpower_route.hpp>
#include <scorum/chain/services/dynamic_global_property.hpp>

#include <scorum/chain/schema/dev_committee_object.hpp>
#include <scorum/chain/schema/dynamic_global_property_object.hpp>

namespace scorum {
namespace chain {

withdraw_scorumpower_check_fixture::withdraw_scorumpower_check_fixture()
    : account_service(db.account_service())
    , pool_service(db.dev_pool_service())
    , withdraw_scorumpower_service(db.withdraw_scorumpower_service())
    , withdraw_scorumpower_route_service(db.withdraw_scorumpower_route_service())
    , dynamic_global_property_service(db.dynamic_global_property_service())
{
    open_database();
}

void withdraw_scorumpower_check_fixture::set_dev_pool_balance(const asset& sp_balance, const asset& scr_balance)
{
    generate_blocks(2);

    db_plugin->debug_update(
        [&](database&) {
            pool_service.update([&](dev_committee_object& pool) {
                pool.sp_balance = sp_balance;
                pool.scr_balance = scr_balance;
            });

            dynamic_global_property_service.update([&](dynamic_global_property_object& props) {
                props.total_supply += asset(sp_balance.amount, SCORUM_SYMBOL);
                props.total_supply += asset(scr_balance.amount, SCORUM_SYMBOL);
            });
        },
        default_skip);

    validate_database();
    generate_blocks(2);
}
}
}
