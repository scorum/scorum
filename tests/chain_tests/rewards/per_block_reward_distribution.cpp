#include <boost/test/unit_test.hpp>

#include <scorum/chain/services/account.hpp>
#include <scorum/chain/services/reward_balancer.hpp>
#include <scorum/chain/services/budgets.hpp>
#include <scorum/chain/services/dev_pool.hpp>
#include <scorum/chain/services/reward_funds.hpp>
#include <scorum/chain/services/dynamic_global_property.hpp>
#include <scorum/protocol/config.hpp>

#include <scorum/chain/schema/account_objects.hpp>
#include <scorum/chain/schema/reward_balancer_objects.hpp>
#include <scorum/chain/schema/budget_objects.hpp>
#include <scorum/chain/schema/dev_committee_object.hpp>
#include <scorum/chain/schema/scorum_objects.hpp>
#include <scorum/chain/schema/dynamic_global_property_object.hpp>

#include <scorum/utils/fraction.hpp>

#include <boost/uuid/uuid_generators.hpp>

#include "database_default_integration.hpp"

#include <limits>

using namespace database_fixture;

namespace reward_distribution {
using namespace scorum;

class dbs_reward_fixture : public database_integration_fixture
{
public:
    dbs_reward_fixture()
        : content_reward_scr_service(db.content_reward_scr_service())
        , fund_budget_service(db.fund_budget_service())
        , adv_budget_svc(db.post_budget_service())
        , dev_service(db.dev_pool_service())
        , content_reward_fund_scr_service(db.content_reward_fund_scr_service())
        , content_reward_fund_sp_service(db.content_reward_fund_sp_service())
        , account_service(db.account_service())
        , dgp_service(db.dynamic_global_property_service())
        , voters_reward_sp_service(db.voters_reward_sp_service())
        , voters_reward_scr_service(db.voters_reward_scr_service())
    {
        open_database();
    }

    content_reward_scr_service_i& content_reward_scr_service;
    fund_budget_service_i& fund_budget_service;
    post_budget_service_i& adv_budget_svc;
    dev_pool_service_i& dev_service;
    content_reward_fund_scr_service_i& content_reward_fund_scr_service;
    content_reward_fund_sp_service_i& content_reward_fund_sp_service;
    account_service_i& account_service;
    dynamic_global_property_service_i& dgp_service;
    voters_reward_sp_service_i& voters_reward_sp_service;
    voters_reward_scr_service_i& voters_reward_scr_service;

    const asset NULL_BALANCE = asset(0, SCORUM_SYMBOL);

    boost::uuids::uuid ns_uuid = boost::uuids::string_generator()("00000000-0000-0000-0000-000000000001");
    boost::uuids::name_generator uuid_gen = boost::uuids::name_generator(ns_uuid);
};
}

BOOST_FIXTURE_TEST_SUITE(reward_distribution, reward_distribution::dbs_reward_fixture)

SCORUM_TEST_CASE(check_per_block_reward_distribution_with_fund_budget_only)
{
    generate_block();

    const auto& fund_budget = fund_budget_service.get();
    asset initial_per_block_reward = fund_budget.per_block;

    auto witness_reward = initial_per_block_reward * SCORUM_WITNESS_PER_BLOCK_REWARD_PERCENT / SCORUM_100_PERCENT;
    auto active_voters_reward
        = initial_per_block_reward * SCORUM_ACTIVE_SP_HOLDERS_PER_BLOCK_REWARD_PERCENT / SCORUM_100_PERCENT;
    auto content_reward = initial_per_block_reward - witness_reward - active_voters_reward;

    const auto& account = account_service.get_account(TEST_INIT_DELEGATE_NAME);

    BOOST_REQUIRE_EQUAL(dev_service.get().scr_balance, NULL_BALANCE);
    BOOST_REQUIRE_EQUAL(content_reward_fund_sp_service.get().activity_reward_balance, content_reward);
    BOOST_REQUIRE_EQUAL(account.scorumpower, witness_reward);
    BOOST_REQUIRE_EQUAL(voters_reward_sp_service.get().balance, active_voters_reward);
}

SCORUM_TEST_CASE(check_advertising_budget_reward_distribution_deadline_before_cashout)
{
    //          | r[i] + max(1, r[i]*5/100) if B > r[i]*D(100),
    // r[i+1] = | max(1, r[i] - r[i]*5/100) if B < r[i]*D(30),                          (1)
    //          | r[i]                      if B in [r[i]*D(30), r[i]*D(100)].
    //
    // TODO: there are not tests for this formula.

    const auto& account = account_service.get_account(TEST_INIT_DELEGATE_NAME);
    auto advertising_budget = ASSET_SCR(2e+9);
    auto deadline_blocks_n = 4;

    auto start = db.head_block_time() + SCORUM_BLOCK_INTERVAL; // start in following block
    auto deadline = start + (deadline_blocks_n - 1) * SCORUM_BLOCK_INTERVAL;

    BOOST_REQUIRE_LT(deadline.sec_since_epoch(), start.sec_since_epoch() + SCORUM_ADVERTISING_CASHOUT_PERIOD_SEC);

    auto budget = adv_budget_svc.create_budget(uuid_gen("x"), account.name, advertising_budget, start, deadline, "");

    BOOST_REQUIRE_EQUAL(content_reward_scr_service.get().balance.amount, 0);

    generate_blocks(deadline, false);

    auto budget_outgo = budget.per_block * deadline_blocks_n;
    auto dev_pool_reward
        = budget_outgo * utils::make_fraction(SCORUM_DEV_TEAM_PER_BLOCK_REWARD_PERCENT, SCORUM_100_PERCENT);
    BOOST_CHECK_EQUAL(dev_pool_reward, dev_service.get().scr_balance);
}

SCORUM_TEST_CASE(check_advertising_budget_reward_distribution_deadline_after_cashout)
{
    const auto& account = account_service.get_account(TEST_INIT_DELEGATE_NAME);
    auto advertising_budget = ASSET_SCR(6e+8);
    auto start = db.head_block_time() + SCORUM_BLOCK_INTERVAL; // start in following block
    auto deadline = db.head_block_time() + fc::seconds(SCORUM_ADVERTISING_CASHOUT_PERIOD_SEC + SCORUM_BLOCK_INTERVAL);

    BOOST_REQUIRE_EQUAL(SCORUM_ADVERTISING_CASHOUT_PERIOD_SEC % SCORUM_BLOCK_INTERVAL, 0);
    BOOST_REQUIRE_GT(deadline.sec_since_epoch(),
                     db.head_block_time().sec_since_epoch() + SCORUM_ADVERTISING_CASHOUT_PERIOD_SEC);

    auto budget = adv_budget_svc.create_budget(uuid_gen("x"), account.name, advertising_budget, start, deadline, "");

    BOOST_REQUIRE_EQUAL(content_reward_scr_service.get().balance.amount, 0);

    {
        // no payments yet
        generate_blocks(SCORUM_ADVERTISING_CASHOUT_PERIOD_SEC / SCORUM_BLOCK_INTERVAL - 1);
        BOOST_CHECK_EQUAL(dev_service.get().scr_balance.amount, 0);
    }
    {
        // advertising budget cashout
        generate_block();
        auto dev_pool_reward = (advertising_budget - budget.per_block)
            * utils::make_fraction(SCORUM_DEV_TEAM_PER_BLOCK_REWARD_PERCENT, SCORUM_100_PERCENT);
        BOOST_CHECK_EQUAL(dev_pool_reward, dev_service.get().scr_balance);
    }
    {
        // advertising budget deadline
        generate_block();
        auto dev_pool_reward
            = advertising_budget * utils::make_fraction(SCORUM_DEV_TEAM_PER_BLOCK_REWARD_PERCENT, SCORUM_100_PERCENT);
        BOOST_CHECK_EQUAL(dev_pool_reward, dev_service.get().scr_balance);
    }
}

BOOST_AUTO_TEST_SUITE_END()
