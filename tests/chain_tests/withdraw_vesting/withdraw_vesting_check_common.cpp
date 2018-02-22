#include "withdraw_vesting_check_common.hpp"

#include <scorum/chain/services/account.hpp>
#include <scorum/chain/services/dev_pool.hpp>
#include <scorum/chain/services/withdraw_vesting.hpp>
#include <scorum/chain/services/withdraw_vesting_route.hpp>

#include <scorum/chain/schema/dev_committee_object.hpp>

namespace scorum {
namespace chain {

withdraw_vesting_check_fixture::withdraw_vesting_check_fixture()
    : account_service(db.account_service())
    , pool_service(db.dev_pool_service())
    , withdraw_vesting_service(db.withdraw_vesting_service())
    , withdraw_vesting_route_service(db.withdraw_vesting_route_service())
{
    open_database();

    generate_blocks(5);

    FC_ASSERT(!pool_service.is_exists());

    create_dev_pool();

    generate_blocks(5);
}

void withdraw_vesting_check_fixture::create_dev_pool()
{
    db_plugin->debug_update(
        [&](database&) {
            pool_service.create([&](dev_committee_object& pool) {
                pool.balance_in = ASSET_NULL_SP;
                pool.balance_out = ASSET_NULL_SCR;
            });
        },
        default_skip);
}
}
}
