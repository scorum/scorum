#include <boost/test/unit_test.hpp>

#include <scorum/chain/schema/account_objects.hpp>
#include <scorum/chain/schema/withdraw_vesting_objects.hpp>

#include "withdraw_vesting_check_common.hpp"

using namespace scorum::protocol;
using namespace scorum::chain;

BOOST_AUTO_TEST_SUITE(withdraw_vesting_route_from_account_to_dev_pool_tests)

struct withdraw_vesting_route_from_account_to_dev_pool_tests_fixture : public withdraw_vesting_check_fixture
{
    withdraw_vesting_route_from_account_to_dev_pool_tests_fixture()
    {
        ACTOR(alice);
        alice_key = alice_private_key;

        generate_blocks(5);
    }

    private_key_type alice_key;
};

BOOST_FIXTURE_TEST_CASE(withdrawal_tree_check, withdraw_vesting_route_from_account_to_dev_pool_tests_fixture)
{
    // TODO
}

BOOST_AUTO_TEST_SUITE_END()
