#include "withdraw_vesting_check_common.hpp"

#include <scorum/chain/services/account.hpp>
#include <scorum/chain/services/dev_pool.hpp>
#include <scorum/chain/services/withdraw_vesting.hpp>
#include <scorum/chain/services/withdraw_vesting_route.hpp>
#include <scorum/chain/services/dynamic_global_property.hpp>

#include <scorum/chain/schema/dev_committee_object.hpp>
#include <scorum/chain/schema/dynamic_global_property_object.hpp>

namespace scorum {
namespace chain {

withdraw_vesting_check_fixture::withdraw_vesting_check_fixture()
    : account_service(db.account_service())
    , pool_service(db.dev_pool_service())
    , withdraw_vesting_service(db.withdraw_vesting_service())
    , withdraw_vesting_route_service(db.withdraw_vesting_route_service())
    , dynamic_global_property_service(db.dynamic_global_property_service())
{
    open_database();
}

void withdraw_vesting_check_fixture::create_dev_pool(const asset& balance_in, const asset& balance_out)
{
    FC_ASSERT(!pool_service.is_exists());

    generate_blocks(2);

    db_plugin->debug_update(
        [&](database&) {
            pool_service.create([&](dev_committee_object& pool) {
                pool.balance_in = balance_in;
                pool.balance_out = balance_out;
            });

            dynamic_global_property_service.update([&](dynamic_global_property_object& props) {
                props.total_supply += asset(balance_in.amount, SCORUM_SYMBOL);
                props.total_supply += asset(balance_out.amount, SCORUM_SYMBOL);
            });
        },
        default_skip);

    validate_database();
    generate_blocks(2);
}
}
}
