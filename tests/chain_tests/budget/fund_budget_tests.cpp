#ifdef IS_TEST_NET
#include <boost/test/unit_test.hpp>

#include <scorum/chain/services/account.hpp>
#include <scorum/chain/services/budget.hpp>

#include <scorum/chain/schema/budget_object.hpp>

#include "database_default_integration.hpp"

using namespace database_fixture;

class fund_budget_fixture : public database_integration_fixture
{
public:
    fund_budget_fixture()
        : budget_service(db.obtain_service<dbs_budget>())
    {
        open_database();
    }

    dbs_budget& budget_service;
};

//
// usage for all budget tests 'chain_test  -t budget_*'
//
BOOST_FIXTURE_TEST_SUITE(budget_fund_check, fund_budget_fixture)

SCORUM_TEST_CASE(fund_budget_creation)
{
    // fund budget is created within database(in database::init_genesis_rewards())
    // TODO: make MOC for database and test budget_service separately
    BOOST_REQUIRE_NO_THROW(budget_service.get_fund_budget());
}

SCORUM_TEST_CASE(second_fund_budget_creation)
{
    asset balance(1, SCORUM_SYMBOL);
    fc::time_point_sec deadline = db.get_slot_time(1);

    BOOST_REQUIRE_THROW(budget_service.create_fund_budget(balance, deadline), fc::assert_exception);
}

SCORUM_TEST_CASE(fund_budget_initial_supply)
{
    auto budget = budget_service.get_fund_budget();

    BOOST_REQUIRE_EQUAL(budget.balance, TEST_REWARD_INITIAL_SUPPLY);
}

SCORUM_TEST_CASE(fund_budget_initial_deadline)
{
    auto budget = budget_service.get_fund_budget();

    BOOST_REQUIRE(budget.deadline == db.get_genesis_time() + fc::days(SCORUM_REWARDS_INITIAL_SUPPLY_PERIOD_IN_DAYS));
}

SCORUM_TEST_CASE(try_close_fund_budget)
{
    auto budget = budget_service.get_fund_budget();

    BOOST_REQUIRE_THROW(budget_service.close_budget(budget), fc::assert_exception);
}

SCORUM_TEST_CASE(fund_budget_payments_after_deadline_is_over_and_we_have_missing_blocks)
{
    auto& budget = budget_service.get_fund_budget();

    auto original_balance = budget.balance;

    auto miss_intermediate_blocks = true;

    auto actually_generated_blocks
        = db_plugin->debug_generate_blocks_until(debug_key, budget.deadline, miss_intermediate_blocks, default_skip);
    BOOST_REQUIRE((db.head_block_time() - budget.deadline).to_seconds() < SCORUM_BLOCK_INTERVAL);

    BOOST_REQUIRE(budget_service.is_fund_budget_exists());

    BOOST_REQUIRE_EQUAL(budget.balance,
                        original_balance - asset(budget.per_block, SCORUM_SYMBOL) * actually_generated_blocks);

    // generate block after deadline
    generate_block();
    ++actually_generated_blocks;

    BOOST_REQUIRE(budget_service.is_fund_budget_exists());

    BOOST_REQUIRE_EQUAL(budget.balance,
                        original_balance - asset(budget.per_block, SCORUM_SYMBOL) * actually_generated_blocks);
}

BOOST_AUTO_TEST_SUITE_END()

#endif
