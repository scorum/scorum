#include <boost/test/unit_test.hpp>

#include "budget_check_common.hpp"

namespace budget_autoclose_tests {

using namespace budget_check_common;

class budget_autoclose_tests_fixture : public budget_check_fixture
{
public:
    budget_autoclose_tests_fixture()
        : account_service(db.account_service())
        , alice("alice")
    {
        actor(initdelegate).create_account(alice);
        actor(initdelegate).give_scr(alice, BUDGET_BALANCE_DEFAULT * 100);
    }

    account_service_i& account_service;

    Actor alice;
};

BOOST_FIXTURE_TEST_SUITE(budget_autoclose_check, budget_autoclose_tests_fixture)

SCORUM_TEST_CASE(auto_close_budget_by_balance)
{
    int balance = 10;
    int deadline_in_blocks = 15;

    create_budget(alice, budget_type::post, balance, deadline_in_blocks);

    BOOST_REQUIRE(!post_budget_service.get_budgets(alice.name).empty());

    BOOST_CHECK_EQUAL(post_budget_service.get(post_budget_object::id_type(0)).per_block, ASSET_SCR(1));

    for (int ci = 0; ci < balance; ++ci)
    {
        BOOST_REQUIRE(!post_budget_service.get_budgets(alice.name).empty());

        generate_block();
    }

    BOOST_REQUIRE(post_budget_service.get_budgets(alice.name).empty());
}

SCORUM_TEST_CASE(auto_close_budget_by_deadline_start_and_deadline_arent_aligned_by_block_interval_shouldnt_throw)
{
    create_budget_operation op;
    op.start = db.head_block_time() + 1;
    op.deadline = db.head_block_time() + SCORUM_BLOCK_INTERVAL * 2 + 2;
    op.balance = ASSET_SCR(15);
    op.owner = initdelegate.name;
    op.type = budget_type::banner;

    push_operation(op);

    BOOST_REQUIRE_NO_THROW(generate_block());
    BOOST_REQUIRE(post_budget_service.get_budgets(alice.name).empty());
    BOOST_REQUIRE_NO_THROW(generate_block());
}

SCORUM_TEST_CASE(auto_close_budget_by_deadline)
{
    int balance = 13;
    int deadline_in_blocks = 4;

    auto initial = account_service.get_account(alice.name).balance;

    create_budget(alice, budget_type::post, balance, deadline_in_blocks);

    initial -= ASSET_SCR(balance);

    BOOST_REQUIRE(!post_budget_service.get_budgets(alice.name).empty());

    for (int ci = 0; ci < deadline_in_blocks; ++ci)
    {
        BOOST_REQUIRE(!post_budget_service.get_budgets(alice.name).empty());

        generate_block();
    }

    BOOST_REQUIRE(post_budget_service.get_budgets(alice.name).empty());

    auto rest = balance - (balance / deadline_in_blocks * deadline_in_blocks);
    BOOST_CHECK_EQUAL(account_service.get_account(alice.name).balance, initial + ASSET_SCR(rest));
}

SCORUM_TEST_CASE(return_money_to_account_after_deadline_is_over_and_we_have_missing_blocks)
{
    int balance = 10;
    int deadline_in_blocks = 10;

    create_budget(alice, budget_type::post, balance, deadline_in_blocks);

    BOOST_REQUIRE(!post_budget_service.get_budgets(alice.name).empty());

    const post_budget_object& budget = (*post_budget_service.get_budgets(alice.name).cbegin());

    auto original_balance = account_service.get_account(alice.name).balance;

    auto deadline = budget.deadline;

    auto miss_intermediate_blocks = true;
    auto actually_generated_blocks
        = db_plugin->debug_generate_blocks_until(debug_key, deadline, miss_intermediate_blocks, get_skip_flags());

    BOOST_REQUIRE_LT((db.head_block_time() - deadline).to_seconds(), SCORUM_BLOCK_INTERVAL);

    BOOST_REQUIRE(post_budget_service.get_budgets(alice.name).empty());

    BOOST_REQUIRE_EQUAL(account_service.get_account(alice.name).balance,
                        original_balance + ASSET_SCR(deadline_in_blocks - actually_generated_blocks));
}

BOOST_AUTO_TEST_SUITE_END()
}
