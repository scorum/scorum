#ifdef IS_TEST_NET
#include <boost/test/unit_test.hpp>

#include <scorum/chain/pool/reward_pool.hpp>
#include <scorum/chain/dbs_reward.hpp>
#include <scorum/chain/dbs_budget.hpp>
#include <scorum/protocol/config.hpp>

#include "../common/database_fixture.hpp"

#include <limits>

using namespace scorum;
using namespace scorum::chain;
using namespace scorum::protocol;
using fc::string;

class dbs_reward_fixture : public clean_database_fixture
{
public:
    dbs_reward_fixture()
        : reward_service(db.obtain_service<dbs_reward>())
        , budget_service(db.obtain_service<dbs_budget>())
    {
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

        BOOST_CHECK(reward_service.get_pool().name == reward_pool_name_type("reward_pool"));
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
        const reward_pool_object& pool = reward_service.get_pool();

        asset total_reward_supply = pool.balance;
        for (const auto& bugget : budget_service.get_fund_budgets())
        {
            total_reward_supply += bugget.get().balance;
        }

        BOOST_REQUIRE(total_reward_supply == TEST_REWARD_INITIAL_SUPPLY);
    }
    FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE(check_reward_pool_initial_balance_is_not_null)
{
    try
    {
        const reward_pool_object& pool = reward_service.get_pool();

        BOOST_REQUIRE(pool.balance > NULL_BALANCE);
    }
    FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE(check_reward_pool_initial_per_block_reward_is_not_null)
{
    try
    {
        const reward_pool_object& pool = reward_service.get_pool();

        BOOST_REQUIRE(pool.current_per_block_reward > NULL_BALANCE);
    }
    FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE(check_reward_pool_initial_balancing)
{
    try
    {
        const reward_pool_object& pool = reward_service.get_pool();

        BOOST_REQUIRE(pool.balance.amount > pool.current_per_block_reward.amount
                          * SCORUM_GUARANTED_REWARD_SUPPLY_PERIOD_IN_DAYS * SCORUM_BLOCKS_PER_DAY);
    }
    FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE(check_reward_pool_balance_increasing)
{
    try
    {
        const reward_pool_object& pool = reward_service.get_pool();

        asset current_balance = pool.balance;
        asset INCREASE_BALANCE(100, SCORUM_SYMBOL);

        BOOST_REQUIRE(reward_service.increase_pool_ballance(INCREASE_BALANCE) == current_balance + INCREASE_BALANCE);
    }
    FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE(check_reward_pool_per_block_reward_decreases_balance)
{
    try
    {
        const reward_pool_object& pool = reward_service.get_pool();

        asset initial_balance = pool.balance;
        asset block_reward = reward_service.take_block_reward();

        BOOST_REQUIRE(pool.balance + block_reward == initial_balance);
    }
    FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE(check_reward_pool_automatic_reward_increasing)
{
    try
    {
        const reward_pool_object& pool = reward_service.get_pool();

        asset initial_per_block_reward = pool.current_per_block_reward;
        asset threshold_balance(pool.current_per_block_reward.amount * SCORUM_REWARD_INCREASE_THRESHOLD_IN_DAYS
                                * SCORUM_BLOCKS_PER_DAY);

        BOOST_REQUIRE(reward_service.increase_pool_ballance(threshold_balance + asset(1) - pool.balance)
                      == threshold_balance + asset(1));

        asset current_per_block_reward = reward_service.take_block_reward();

        BOOST_REQUIRE(current_per_block_reward.amount
                      == initial_per_block_reward.amount * (100 + SCORUM_ADJUST_REWARD_PERCENT) / 100);
    }
    FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE(check_reward_pool_automatic_reward_decreasing)
{
    try
    {
        const reward_pool_object& pool = reward_service.get_pool();

        asset initial_per_block_reward = pool.current_per_block_reward;
        asset threshold_balance(pool.current_per_block_reward.amount * SCORUM_GUARANTED_REWARD_SUPPLY_PERIOD_IN_DAYS
                                * SCORUM_BLOCKS_PER_DAY);

        asset current_per_block_reward = NULL_BALANCE;
        while (pool.balance > threshold_balance)
        {
            current_per_block_reward = reward_service.take_block_reward();
        }

        current_per_block_reward = reward_service.take_block_reward();

        BOOST_REQUIRE(current_per_block_reward.amount
                      == initial_per_block_reward.amount * (100 - SCORUM_ADJUST_REWARD_PERCENT) / 100);
    }
    FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_SUITE_END()

#endif
