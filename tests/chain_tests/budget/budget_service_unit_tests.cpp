#include <boost/test/unit_test.hpp>

#include <scorum/chain/services/budgets.hpp>

#include <scorum/chain/schema/budget_objects.hpp>

#include <scorum/common_api/config_api.hpp>

#include "budget_check_common.hpp"

#include <limits>

#include "actor.hpp"

namespace budget_service_unit_tests {

using namespace budget_check_common;

using scorum::chain::adv_total_stats;

struct fixture : budget_check_fixture
{
    Actor alice;

    fixture()
        : alice("alice")
    {
        actor(initdelegate).create_account(alice);
        actor(initdelegate).give_scr(alice, BUDGET_BALANCE_DEFAULT * 100u);

        BOOST_REQUIRE_EQUAL(ASSET_SCR(0), post_budgets_stat().volume);
        BOOST_REQUIRE_EQUAL(ASSET_SCR(0), post_budgets_stat().budget_pending_outgo);
        BOOST_REQUIRE_EQUAL(ASSET_SCR(0), post_budgets_stat().owner_pending_income);
    }

    adv_total_stats::budget_type_stat stat() const
    {
        return this->db.get<dynamic_global_property_object>().advertising.post_budgets;
    }

    adv_total_stats::budget_type_stat post_budgets_stat()
    {
        return db.get<dynamic_global_property_object>().advertising.post_budgets;
    }

    void create_budget(uint32_t balance = BUDGET_BALANCE_DEFAULT)
    {
        auto start = db.head_block_time() + SCORUM_BLOCK_INTERVAL;
        auto deadline = db.head_block_time() + BUDGET_DEADLINE_IN_BLOCKS_DEFAULT * SCORUM_BLOCK_INTERVAL;

        post_budget_service.create_budget(alice.name, ASSET_SCR(balance), start, deadline, "");
    }
};

BOOST_FIXTURE_TEST_SUITE(advertising_stat_tests, fixture)

SCORUM_TEST_CASE(budget_creation_increase_volume)
{
    create_budget();

    BOOST_CHECK_EQUAL(ASSET_SCR(BUDGET_BALANCE_DEFAULT), post_budgets_stat().volume);
}

SCORUM_TEST_CASE(budget_creation_dont_change_other_stat)
{
    create_budget();

    BOOST_CHECK_EQUAL(ASSET_SCR(0), post_budgets_stat().budget_pending_outgo);
    BOOST_CHECK_EQUAL(ASSET_SCR(0), post_budgets_stat().owner_pending_income);
}

SCORUM_TEST_CASE(allocate_cash_decrease_volume)
{
    create_budget();

    const auto& budget = *(db.get_index<post_budget_index, by_id>().begin());

    post_budget_service.allocate_cash(budget);

    BOOST_CHECK_EQUAL(ASSET_SCR(BUDGET_BALANCE_DEFAULT - BUDGET_PERBLOCK_DEFAULT), post_budgets_stat().volume);
}

SCORUM_TEST_CASE(allocate_cash_dont_change_other_stat)
{
    create_budget();

    const auto& budget = *(db.get_index<post_budget_index, by_id>().begin());

    post_budget_service.allocate_cash(budget);

    BOOST_CHECK_EQUAL(ASSET_SCR(0), post_budgets_stat().budget_pending_outgo);
    BOOST_CHECK_EQUAL(ASSET_SCR(0), post_budgets_stat().owner_pending_income);
}

SCORUM_TEST_CASE(update_pending_payouts_updates_pending_outgo_and_pending_income)
{
    create_budget();

    const auto& budget = *(db.get_index<post_budget_index, by_id>().begin());

    post_budget_service.update_pending_payouts(budget, ASSET_SCR(10), ASSET_SCR(20));

    BOOST_CHECK_EQUAL(ASSET_SCR(10), post_budgets_stat().owner_pending_income);
    BOOST_CHECK_EQUAL(ASSET_SCR(20), post_budgets_stat().budget_pending_outgo);
}

SCORUM_TEST_CASE(update_pending_payouts_dont_change_volume)
{
    create_budget();

    const auto& budget = *(db.get_index<post_budget_index, by_id>().begin());

    BOOST_CHECK_EQUAL(ASSET_SCR(BUDGET_BALANCE_DEFAULT), post_budgets_stat().volume);

    post_budget_service.update_pending_payouts(budget, ASSET_SCR(10), ASSET_SCR(0));

    BOOST_CHECK_EQUAL(ASSET_SCR(BUDGET_BALANCE_DEFAULT), post_budgets_stat().volume);
}

SCORUM_TEST_CASE(perform_pending_payouts_dont_change_anything_because_pending_outgo_and_pending_income_zero)
{
    create_budget();

    const auto& budget = *(db.get_index<post_budget_index, by_id>().begin());

    BOOST_CHECK_EQUAL(ASSET_SCR(BUDGET_BALANCE_DEFAULT), post_budgets_stat().volume);
    BOOST_CHECK_EQUAL(ASSET_NULL_SCR, post_budgets_stat().budget_pending_outgo);
    BOOST_CHECK_EQUAL(ASSET_NULL_SCR, post_budgets_stat().owner_pending_income);

    post_budget_service.perform_pending_payouts({ budget });

    BOOST_CHECK_EQUAL(ASSET_SCR(BUDGET_BALANCE_DEFAULT), post_budgets_stat().volume);
    BOOST_CHECK_EQUAL(ASSET_NULL_SCR, post_budgets_stat().budget_pending_outgo);
    BOOST_CHECK_EQUAL(ASSET_NULL_SCR, post_budgets_stat().owner_pending_income);
}

SCORUM_TEST_CASE(perform_pending_payouts_decrease_budget_pending_outgo)
{
    const auto budget_pending_outgo = ASSET_SCR(10);

    create_budget();

    const auto& budget = *(db.get_index<post_budget_index, by_id>().begin());

    db.modify(db.get<dynamic_global_property_object>(), [&](dynamic_global_property_object& obj) {
        obj.advertising.post_budgets.budget_pending_outgo = budget_pending_outgo;
    });

    db.modify(budget, [&](post_budget_object& obj) { obj.budget_pending_outgo = budget_pending_outgo; });

    BOOST_REQUIRE_EQUAL(budget_pending_outgo, post_budgets_stat().budget_pending_outgo);
    BOOST_REQUIRE_EQUAL(budget_pending_outgo, budget.budget_pending_outgo);

    post_budget_service.perform_pending_payouts({ budget });

    BOOST_CHECK_EQUAL(ASSET_NULL_SCR, post_budgets_stat().budget_pending_outgo);
    BOOST_REQUIRE_EQUAL(ASSET_NULL_SCR, budget.budget_pending_outgo);
}

SCORUM_TEST_CASE(perform_pending_payouts_dont_change_volume)
{
    const auto volume = ASSET_SCR(10);

    create_budget();

    const auto& budget = *(db.get_index<post_budget_index, by_id>().begin());

    db.modify(db.get<dynamic_global_property_object>(),
              [&](dynamic_global_property_object& obj) { obj.advertising.post_budgets.volume = volume; });

    BOOST_REQUIRE_EQUAL(volume, post_budgets_stat().volume);

    post_budget_service.perform_pending_payouts({ budget });

    BOOST_REQUIRE_EQUAL(volume, post_budgets_stat().volume);
}

SCORUM_TEST_CASE(perform_pending_payouts_decrease_owner_pending_income)
{
    const auto owner_pending_income = ASSET_SCR(10);

    create_budget();

    const auto& budget = *(db.get_index<post_budget_index, by_id>().begin());

    db.modify(db.get<dynamic_global_property_object>(), [&](dynamic_global_property_object& obj) {
        obj.advertising.post_budgets.owner_pending_income = owner_pending_income;
    });

    db.modify(budget, [&](post_budget_object& obj) { obj.owner_pending_income = owner_pending_income; });

    BOOST_REQUIRE_EQUAL(owner_pending_income, post_budgets_stat().owner_pending_income);
    BOOST_REQUIRE_EQUAL(owner_pending_income, budget.owner_pending_income);

    post_budget_service.perform_pending_payouts({ budget });

    BOOST_CHECK_EQUAL(ASSET_NULL_SCR, post_budgets_stat().owner_pending_income);
    BOOST_REQUIRE_EQUAL(ASSET_NULL_SCR, budget.owner_pending_income);
}

SCORUM_TEST_CASE(finish_budget_increase_owner_pending_income_on_budget_balance)
{
    create_budget();

    const auto& budget = *(db.get_index<post_budget_index, by_id>().begin());

    const auto expected_owner_pending_income = budget.balance;

    post_budget_service.finish_budget(budget.id);

    BOOST_CHECK_EQUAL(expected_owner_pending_income, post_budgets_stat().owner_pending_income);
}

SCORUM_TEST_CASE(finish_budget_decrease_volume_by_budget_balance)
{
    create_budget();

    const auto& budget = *(db.get_index<post_budget_index, by_id>().begin());

    db.modify(db.get<dynamic_global_property_object>(),
              [&](dynamic_global_property_object& obj) { obj.advertising.post_budgets.volume = budget.balance; });

    post_budget_service.finish_budget(budget.id);

    BOOST_CHECK_EQUAL(ASSET_NULL_SCR, post_budgets_stat().volume);
}

BOOST_AUTO_TEST_SUITE_END()
}
