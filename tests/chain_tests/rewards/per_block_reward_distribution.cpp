#ifdef IS_TEST_NET
#include <boost/test/unit_test.hpp>

#include <scorum/chain/services/account.hpp>
#include <scorum/chain/services/reward.hpp>
#include <scorum/chain/services/budget.hpp>
#include <scorum/chain/services/dev_pool.hpp>
#include <scorum/chain/services/reward_fund.hpp>
#include <scorum/protocol/config.hpp>

#include <scorum/chain/schema/account_objects.hpp>
#include <scorum/chain/schema/reward_pool_object.hpp>
#include <scorum/chain/schema/budget_object.hpp>
#include <scorum/chain/schema/dev_committee_object.hpp>
#include <scorum/chain/schema/scorum_objects.hpp>

#include "database_default_integration.hpp"

#include <limits>

using namespace database_fixture;

namespace reward_distribution {

class dbs_reward_fixture : public database_integration_fixture
{
public:
    dbs_reward_fixture()
        : reward_service(db.obtain_service<dbs_reward>())
        , budget_service(db.obtain_service<dbs_budget>())
        , dev_service(db.obtain_service<dbs_dev_pool>())
        , reward_fund_service(db.obtain_service<dbs_reward_fund>())
        , account_service(db.obtain_service<dbs_account>())
    {
        open_database();
    }

    dbs_reward& reward_service;
    dbs_budget& budget_service;
    dbs_dev_pool& dev_service;
    dbs_reward_fund& reward_fund_service;
    dbs_account& account_service;

    const asset NULL_BALANCE = asset(0, SCORUM_SYMBOL);
};
}

BOOST_FIXTURE_TEST_SUITE(reward_distribution, reward_distribution::dbs_reward_fixture)

SCORUM_TEST_CASE(check_per_block_reward_distribution_with_fund_budget_only)
{
    const reward_pool_object& pool = reward_service.get_pool();

    asset initial_per_block_reward = pool.current_per_block_reward;

    generate_block();

    auto witness_reward = initial_per_block_reward * SCORUM_WITNESS_PER_BLOCK_REWARD_PERCENT / SCORUM_100_PERCENT;
    auto content_reward = initial_per_block_reward - witness_reward;

    const auto& account = account_service.get_account(TEST_INIT_DELEGATE_NAME);

    BOOST_REQUIRE_EQUAL(dev_service.get().scr_balance, NULL_BALANCE);
    BOOST_REQUIRE_EQUAL(reward_fund_service.get().reward_balance, content_reward);
    BOOST_REQUIRE_EQUAL(account.scorumpower, asset(witness_reward.amount, SP_SYMBOL));
}

SCORUM_TEST_CASE(check_per_block_reward_distribution_with_fund_and_advertising_budgets)
{
    const reward_pool_object& pool = reward_service.get_pool();

    asset initial_per_block_reward = pool.current_per_block_reward;

    const auto& account = account_service.get_account(TEST_INIT_DELEGATE_NAME);
    auto budget = asset(10, SCORUM_SYMBOL);
    auto deadline = db.get_slot_time(1);

    BOOST_CHECK_NO_THROW(budget_service.create_budget(account, budget, deadline));

    generate_block();

    auto dev_team_reward = budget * SCORUM_DEV_TEAM_PER_BLOCK_REWARD_PERCENT / SCORUM_100_PERCENT;
    auto witness_reward = initial_per_block_reward * SCORUM_WITNESS_PER_BLOCK_REWARD_PERCENT / SCORUM_100_PERCENT;
    auto content_reward = initial_per_block_reward - witness_reward;

    BOOST_REQUIRE_EQUAL(dev_service.get().scr_balance, dev_team_reward);
    BOOST_REQUIRE_EQUAL(reward_fund_service.get().reward_balance, content_reward);
    BOOST_REQUIRE_EQUAL(account.scorumpower, asset(witness_reward.amount, SP_SYMBOL));
}

BOOST_AUTO_TEST_SUITE_END()

#endif
