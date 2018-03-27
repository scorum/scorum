#ifdef IS_TEST_NET
#include <boost/test/unit_test.hpp>

#include <scorum/chain/schema/reward_balancer_object.hpp>
#include <scorum/chain/services/reward_balancer.hpp>
#include <scorum/chain/services/budget.hpp>
#include <scorum/protocol/config.hpp>

#include <scorum/chain/schema/budget_object.hpp>

#include "database_default_integration.hpp"

#include <limits>

using namespace database_fixture;

class dbs_reward_fixture : public database_integration_fixture
{
public:
    dbs_reward_fixture()
        : reward_service(db.obtain_service<dbs_reward>())
        , budget_service(db.obtain_service<dbs_budget>())
    {
        open_database();
    }

    dbs_reward& reward_service;
    dbs_budget& budget_service;

    const asset NULL_BALANCE = asset(0, SCORUM_SYMBOL);
};

BOOST_FIXTURE_TEST_SUITE(reward_service, dbs_reward_fixture)

BOOST_AUTO_TEST_CASE(check_reward_pool_creation)
{
    try
    {
        BOOST_REQUIRE_NO_THROW(reward_service.get_pool());
    }
    FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE(check_reward_pool_double_creation)
{
    try
    {
        BOOST_REQUIRE_THROW(reward_service.create_pool(asset(10000, SCORUM_SYMBOL)), fc::assert_exception);
    }
    FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE(check_reward_pool_initial_supply_distribution)
{
    try
    {
        const reward_balancer_object& pool = reward_service.get_pool();

        asset total_reward_supply = pool.balance;
        total_reward_supply += budget_service.get_fund_budget().balance;

        BOOST_REQUIRE_EQUAL(total_reward_supply, TEST_REWARD_INITIAL_SUPPLY);
    }
    FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE(check_reward_pool_initial_balance_is_null)
{
    try
    {
        const reward_balancer_object& pool = reward_service.get_pool();

        BOOST_REQUIRE_EQUAL(pool.balance, NULL_BALANCE);
    }
    FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE(check_reward_pool_initial_per_block_reward_is_not_null)
{
    try
    {
        const reward_balancer_object& pool = reward_service.get_pool();

        BOOST_REQUIRE_GT(pool.current_per_block_reward, NULL_BALANCE);
    }
    FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE(check_reward_pool_balance_increasing)
{
    try
    {
        const reward_balancer_object& pool = reward_service.get_pool();

        asset current_balance = pool.balance;
        asset INCREASE_BALANCE(100, SCORUM_SYMBOL);

        BOOST_REQUIRE_EQUAL(reward_service.increase_pool_ballance(INCREASE_BALANCE),
                            current_balance + INCREASE_BALANCE);
    }
    FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE(check_reward_pool_per_block_reward_decreases_balance)
{
    try
    {
        const reward_balancer_object& pool = reward_service.get_pool();

        asset initial_balance = pool.balance;
        asset block_reward = reward_service.take_block_reward();

        BOOST_REQUIRE_EQUAL(pool.balance + block_reward, initial_balance);
    }
    FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE(check_automatic_reward_increasing)
{
    try
    {
        const reward_balancer_object& pool = reward_service.get_pool();

        asset current_per_block_reward = pool.current_per_block_reward;

        asset threshold_balance = asset(current_per_block_reward * SCORUM_REWARD_INCREASE_THRESHOLD_IN_DAYS
                                        * SCORUM_BLOCKS_PER_DAY * (100 / SCORUM_ADJUST_REWARD_PERCENT + 1));

        BOOST_REQUIRE_EQUAL(reward_service.increase_pool_ballance(threshold_balance), threshold_balance);

        for (int i = 1; i < 100 / SCORUM_ADJUST_REWARD_PERCENT; ++i)
        {
            asset balanced_per_block_reward = current_per_block_reward * (100 + SCORUM_ADJUST_REWARD_PERCENT) / 100;

            current_per_block_reward = reward_service.take_block_reward();

            // before current_per_block_reward becomes 20 we increase reward by 1
            BOOST_REQUIRE_EQUAL(current_per_block_reward, balanced_per_block_reward + 1);
        }

        // after current_per_block_reward becomes 20 we increase reward by by 5%
        asset balanced_per_block_reward = current_per_block_reward * (100 + SCORUM_ADJUST_REWARD_PERCENT) / 100;

        current_per_block_reward = reward_service.take_block_reward();

        BOOST_REQUIRE_EQUAL(current_per_block_reward, balanced_per_block_reward);
    }
    FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE(check_automatic_reward_decreasing)
{
    try
    {
        const reward_balancer_object& pool = reward_service.get_pool();

        int k = (100 / SCORUM_ADJUST_REWARD_PERCENT + 1);

        asset threshold_balance = asset(pool.current_per_block_reward * SCORUM_GUARANTED_REWARD_SUPPLY_PERIOD_IN_DAYS
                                        * SCORUM_BLOCKS_PER_DAY * k);

        reward_service.update([&](reward_balancer_object& rp) { rp.current_per_block_reward *= k; });

        BOOST_REQUIRE_EQUAL(reward_service.increase_pool_ballance(threshold_balance), threshold_balance);

        asset initial_per_block_reward = reward_service.take_block_reward();
        asset current_per_block_reward = reward_service.take_block_reward();

        BOOST_REQUIRE_EQUAL(current_per_block_reward,
                            initial_per_block_reward - (initial_per_block_reward * SCORUM_ADJUST_REWARD_PERCENT) / 100);
    }
    FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE(check_automatic_reward_decreasing_with_int_arithmetic_correction)
{
    try
    {
        const reward_balancer_object& pool = reward_service.get_pool();

        int k = 2;

        asset threshold_balance = asset(pool.current_per_block_reward * SCORUM_GUARANTED_REWARD_SUPPLY_PERIOD_IN_DAYS
                                        * SCORUM_BLOCKS_PER_DAY * k);

        reward_service.update([&](reward_balancer_object& rp) { rp.current_per_block_reward *= k; });

        BOOST_REQUIRE_EQUAL(reward_service.increase_pool_ballance(threshold_balance), threshold_balance);

        asset initial_per_block_reward = reward_service.take_block_reward();
        asset current_per_block_reward = reward_service.take_block_reward();

        BOOST_REQUIRE_EQUAL(current_per_block_reward, initial_per_block_reward - 1);
    }
    FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_SUITE_END()

#endif
