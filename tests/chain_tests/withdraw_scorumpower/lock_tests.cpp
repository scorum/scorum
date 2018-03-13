#include <boost/test/unit_test.hpp>

#include <scorum/chain/services/account.hpp>
#include <scorum/chain/services/withdraw_scorumpower.hpp>

#include <scorum/chain/schema/account_objects.hpp>
#include <scorum/chain/schema/withdraw_scorumpower_objects.hpp>

#include "withdraw_scorumpower_check_common.hpp"

BOOST_AUTO_TEST_SUITE(withdraw_scorumpower_lock_tests)

struct withdraw_scorumpower_lock_fixture
{
};

BOOST_FIXTURE_TEST_CASE(withdraw_sp_from_account_is_locked_check, withdraw_scorumpower_lock_fixture)
{
    // TODO
}

BOOST_FIXTURE_TEST_CASE(withdraw_sp_from_account__will_be_unlocked_check, withdraw_scorumpower_lock_fixture)
{
    // TODO
}

BOOST_FIXTURE_TEST_CASE(withdraw_sp_from_dev_pool_is_locked_check, withdraw_scorumpower_lock_fixture)
{
    // TODO
}

BOOST_FIXTURE_TEST_CASE(withdraw_sp_from_dev_pool_will_be_unlocked_check, withdraw_scorumpower_lock_fixture)
{
    // TODO
}

BOOST_AUTO_TEST_SUITE_END()
