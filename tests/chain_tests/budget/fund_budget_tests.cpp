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

    const asset FUND_BUDGET_INITIAL_SUPPLY
        = asset(TEST_REWARD_INITIAL_SUPPLY.amount
                    * (SCORUM_REWARDS_INITIAL_SUPPLY_PERIOD_IN_DAYS - SCORUM_GUARANTED_REWARD_SUPPLY_PERIOD_IN_DAYS)
                    / SCORUM_REWARDS_INITIAL_SUPPLY_PERIOD_IN_DAYS,
                SCORUM_SYMBOL);
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

    BOOST_REQUIRE_EQUAL(budget.balance, FUND_BUDGET_INITIAL_SUPPLY);
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

BOOST_AUTO_TEST_SUITE_END()

#endif
