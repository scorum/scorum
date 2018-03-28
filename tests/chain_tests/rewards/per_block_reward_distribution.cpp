#ifdef IS_TEST_NET
#include <boost/test/unit_test.hpp>

#include <scorum/chain/services/account.hpp>
#include <scorum/chain/services/reward_balancer.hpp>
#include <scorum/chain/services/budget.hpp>
#include <scorum/chain/services/dev_pool.hpp>
#include <scorum/chain/services/reward_fund.hpp>
#include <scorum/chain/services/dynamic_global_property.hpp>
#include <scorum/protocol/config.hpp>

#include <scorum/chain/schema/account_objects.hpp>
#include <scorum/chain/schema/reward_balancer_object.hpp>
#include <scorum/chain/schema/budget_object.hpp>
#include <scorum/chain/schema/dev_committee_object.hpp>
#include <scorum/chain/schema/scorum_objects.hpp>
#include <scorum/chain/schema/dynamic_global_property_object.hpp>

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
        , dgp_service(db.obtain_service<dbs_dynamic_global_property>())
    {
        open_database();
    }

    dbs_reward& reward_service;
    dbs_budget& budget_service;
    dbs_dev_pool& dev_service;
    dbs_reward_fund& reward_fund_service;
    dbs_account& account_service;
    dynamic_global_property_service_i& dgp_service;

    const asset NULL_BALANCE = asset(0, SCORUM_SYMBOL);
};
}

BOOST_FIXTURE_TEST_SUITE(reward_distribution, reward_distribution::dbs_reward_fixture)

SCORUM_TEST_CASE(check_per_block_reward_distribution_with_fund_budget_only)
{
    generate_block();

    const auto& fund_budget = budget_service.get_fund_budget();
    asset initial_per_block_reward = asset(fund_budget.per_block, SCORUM_SYMBOL);

    auto witness_reward = initial_per_block_reward * SCORUM_WITNESS_PER_BLOCK_REWARD_PERCENT / SCORUM_100_PERCENT;
    auto content_reward = initial_per_block_reward - witness_reward;

    const auto& account = account_service.get_account(TEST_INIT_DELEGATE_NAME);

    BOOST_REQUIRE_EQUAL(dev_service.get().scr_balance, NULL_BALANCE);
    BOOST_REQUIRE_EQUAL(reward_fund_service.get().activity_reward_balance_scr, content_reward);
    BOOST_REQUIRE_EQUAL(account.scorumpower, asset(witness_reward.amount, SP_SYMBOL));
}

SCORUM_TEST_CASE(check_per_block_reward_distribution_with_fund_and_advertising_budgets)
{
    const auto& account = account_service.get_account(TEST_INIT_DELEGATE_NAME);
    auto advertising_budget = asset(2, SCORUM_SYMBOL);
    auto deadline = db.get_slot_time(1);

    BOOST_CHECK_NO_THROW(budget_service.create_budget(account, advertising_budget, deadline));

    generate_block();

    const auto& fund_budget = budget_service.get_fund_budget();
    asset initial_per_block_reward = asset(fund_budget.per_block, SCORUM_SYMBOL);

    auto dev_team_reward = advertising_budget * SCORUM_DEV_TEAM_PER_BLOCK_REWARD_PERCENT / SCORUM_100_PERCENT;
    auto user_reward = initial_per_block_reward + advertising_budget - dev_team_reward;

    auto witness_reward = user_reward * SCORUM_WITNESS_PER_BLOCK_REWARD_PERCENT / SCORUM_100_PERCENT;
    auto content_reward = user_reward - witness_reward;

    BOOST_REQUIRE_EQUAL(dev_service.get().scr_balance, dev_team_reward);
    BOOST_REQUIRE_EQUAL(reward_fund_service.get().activity_reward_balance_scr, content_reward);
    BOOST_REQUIRE_EQUAL(account.scorumpower, asset(witness_reward.amount, SP_SYMBOL));
}

BOOST_AUTO_TEST_SUITE_END()

#endif
