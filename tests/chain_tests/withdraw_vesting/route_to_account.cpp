#include <boost/test/unit_test.hpp>

#include <scorum/chain/services/account.hpp>
#include <scorum/chain/services/dev_pool.hpp>
#include <scorum/chain/services/withdraw_vesting_route.hpp>
#include <scorum/chain/services/withdraw_vesting.hpp>

#include <scorum/chain/schema/account_objects.hpp>
#include <scorum/chain/schema/withdraw_vesting_objects.hpp>

#include "database_trx_integration.hpp"

using namespace scorum::protocol;
using namespace scorum::chain;

BOOST_AUTO_TEST_SUITE(withdraw_vesting_dev_pool_tests)

struct route_to_account_fixture : public database_trx_integration_fixture
{
    route_to_account_fixture()
        : account_service(db.account_service())
        , withdraw_vesting_route_service(db.withdraw_vesting_route_service())
        , withdraw_vesting_service(db.withdraw_vesting_service())
    {
        open_database();

        ACTORS((alice)(bob));
        alice_key = alice_private_key;
        bob_key = bob_private_key;
    }

    account_service_i& account_service;
    withdraw_vesting_route_service_i& withdraw_vesting_route_service;
    withdraw_vesting_service_i& withdraw_vesting_service;
    private_key_type alice_key;
    private_key_type bob_key;
};

BOOST_FIXTURE_TEST_CASE(set_withdraw_route_to_account_check, route_to_account_fixture)
{
    // TODO
}

// TODO

BOOST_AUTO_TEST_SUITE_END()
