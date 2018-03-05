#include "withdraw_vesting_check_common.hpp"

#include <scorum/chain/services/account.hpp>
#include <scorum/chain/services/dev_pool.hpp>
#include <scorum/chain/services/withdraw_vesting.hpp>
#include <scorum/chain/services/withdraw_vesting_route.hpp>
#include <scorum/chain/services/dynamic_global_property.hpp>

#include <scorum/chain/schema/dev_committee_object.hpp>
#include <scorum/chain/schema/dynamic_global_property_object.hpp>

namespace database_fixture {

withdraw_vesting_check_fixture::withdraw_vesting_check_fixture()
    : account_service(db.account_service())
    , pool_service(db.dev_pool_service())
    , withdraw_vesting_service(db.withdraw_vesting_service())
    , withdraw_vesting_route_service(db.withdraw_vesting_route_service())
    , dynamic_global_property_service(db.dynamic_global_property_service())
{
    open_database();
}

void withdraw_vesting_check_fixture::set_dev_pool_balance(const asset& sp_balance, const asset& scr_balance)
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
