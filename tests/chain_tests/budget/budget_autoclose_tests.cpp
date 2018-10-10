#include <boost/test/unit_test.hpp>

#include "budget_check_common.hpp"
#include <boost/uuid/uuid_generators.hpp>

namespace {

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

    boost::uuids::uuid ns_uuid = boost::uuids::string_generator()("00000000-0000-0000-0000-000000000001");
    boost::uuids::name_generator uuid_gen = boost::uuids::name_generator(ns_uuid);
};

BOOST_FIXTURE_TEST_SUITE(budget_autoclose_check, budget_autoclose_tests_fixture)

SCORUM_TEST_CASE(auto_close_budget_by_balance)
{
    int balance = 10;
    int start_offset = 1;
    int deadline_offset = 15;
    auto uuid = uuid_gen("alice");

    create_budget(uuid, alice, budget_type::post, balance, start_offset, deadline_offset);

    auto budgets = post_budget_service.get_budgets(alice.name);
    BOOST_REQUIRE(!budgets.empty());
    BOOST_CHECK_EQUAL(budgets[0].get().per_block, ASSET_SCR(1));

    for (int ci = 0; ci < balance; ++ci)
    {
        BOOST_REQUIRE(!post_budget_service.get_budgets(alice.name).empty());
        generate_block();
    }

    BOOST_REQUIRE(post_budget_service.get_budgets(alice.name).empty());
}

SCORUM_TEST_CASE(check_acc_balance_after_deadline)
{
    int balance = 13;

    /**
     * cash withdraw  | x    .    .    .    .
     * block offset   | 0    1    2    3    4
     *                |    start         deadline
     */
    int start_blocks_offset = 1; // start in following block
    int deadline_block_offset = 4;
    int per_block_amount = 13 / 4;
    auto uuid = uuid_gen("alice");

    auto initial = account_service.get_account(alice.name).balance;

    create_budget(uuid, alice, budget_type::post, balance, start_blocks_offset, deadline_block_offset);

    initial -= ASSET_SCR(balance);

    BOOST_REQUIRE(!post_budget_service.get_budgets(alice.name).empty());

    for (int ci = 0; ci < deadline_block_offset; ++ci)
    {
        BOOST_REQUIRE(!post_budget_service.get_budgets(alice.name).empty());

        generate_block();
    }

    BOOST_REQUIRE(post_budget_service.get_budgets(alice.name).empty());

    auto rest = balance - per_block_amount * deadline_block_offset;
    BOOST_CHECK_EQUAL(account_service.get_account(alice.name).balance, initial + ASSET_SCR(rest));
}

SCORUM_TEST_CASE(start_and_deadline_are_not_aligned_test)
{
    create_budget_operation op;
    op.start = db.head_block_time() + 2;
    op.deadline = db.head_block_time() + 2 * SCORUM_BLOCK_INTERVAL + 2;
    op.balance = ASSET_SCR(12);
    op.owner = initdelegate.name;
    op.type = budget_type::post;

    BOOST_CHECK(post_budget_service.get_budgets(initdelegate.name).empty());
    push_operation(op);
    {
        auto budgets = post_budget_service.get_budgets(initdelegate.name);
        BOOST_CHECK(!budgets.empty());
        BOOST_CHECK_EQUAL(budgets[0].get().per_block.amount, 4);
        BOOST_CHECK_EQUAL(budgets[0].get().balance.amount, 8);
    }
    generate_block();
    {
        auto budgets = post_budget_service.get_budgets(initdelegate.name);
        BOOST_CHECK(!budgets.empty());
        BOOST_CHECK_EQUAL(budgets[0].get().per_block.amount, 4);
        BOOST_CHECK_EQUAL(budgets[0].get().balance.amount, 4);
    }
    generate_block();
    {
        auto budgets = post_budget_service.get_budgets(initdelegate.name);
        BOOST_CHECK(budgets.empty());
    }
}

SCORUM_TEST_CASE(start_is_aligned_deadline_is_not_aligned_test)
{
    create_budget_operation op;
    op.start = db.head_block_time() + SCORUM_BLOCK_INTERVAL;
    op.deadline = db.head_block_time() + 2 * SCORUM_BLOCK_INTERVAL + 2;
    op.balance = ASSET_SCR(12);
    op.owner = initdelegate.name;
    op.type = budget_type::post;

    BOOST_CHECK(post_budget_service.get_budgets(initdelegate.name).empty());
    push_operation(op);
    {
        auto budgets = post_budget_service.get_budgets(initdelegate.name);
        BOOST_CHECK(!budgets.empty());
        BOOST_CHECK_EQUAL(budgets[0].get().per_block.amount, 4);
        BOOST_CHECK_EQUAL(budgets[0].get().balance.amount, 8);
    }
    generate_block();
    {
        auto budgets = post_budget_service.get_budgets(initdelegate.name);
        BOOST_CHECK(!budgets.empty());
        BOOST_CHECK_EQUAL(budgets[0].get().per_block.amount, 4);
        BOOST_CHECK_EQUAL(budgets[0].get().balance.amount, 4);
    }
    generate_block();
    {
        auto budgets = post_budget_service.get_budgets(initdelegate.name);
        BOOST_CHECK(budgets.empty());
    }
}

SCORUM_TEST_CASE(start_and_deadline_are_aligned_test)
{
    create_budget_operation op;
    op.start = db.head_block_time() + SCORUM_BLOCK_INTERVAL;
    op.deadline = db.head_block_time() + 3 * SCORUM_BLOCK_INTERVAL;
    op.balance = ASSET_SCR(12);
    op.owner = initdelegate.name;
    op.type = budget_type::post;

    BOOST_CHECK(post_budget_service.get_budgets(initdelegate.name).empty());
    push_operation(op);
    {
        auto budgets = post_budget_service.get_budgets(initdelegate.name);
        BOOST_CHECK(!budgets.empty());
        BOOST_CHECK_EQUAL(budgets[0].get().per_block.amount, 4);
        BOOST_CHECK_EQUAL(budgets[0].get().balance.amount, 8);
    }
    generate_block();
    {
        auto budgets = post_budget_service.get_budgets(initdelegate.name);
        BOOST_CHECK(!budgets.empty());
        BOOST_CHECK_EQUAL(budgets[0].get().per_block.amount, 4);
        BOOST_CHECK_EQUAL(budgets[0].get().balance.amount, 4);
    }
    generate_block();
    {
        auto budgets = post_budget_service.get_budgets(initdelegate.name);
        BOOST_CHECK(budgets.empty());
    }
}

SCORUM_TEST_CASE(start_is_not_aligned_deadline_is_aligned_test)
{
    create_budget_operation op;
    op.start = db.head_block_time() + 2;
    op.deadline = db.head_block_time() + 3 * SCORUM_BLOCK_INTERVAL;
    op.balance = ASSET_SCR(12);
    op.owner = initdelegate.name;
    op.type = budget_type::post;

    BOOST_CHECK(post_budget_service.get_budgets(initdelegate.name).empty());
    push_operation(op);
    {
        auto budgets = post_budget_service.get_budgets(initdelegate.name);
        BOOST_CHECK(!budgets.empty());
        BOOST_CHECK_EQUAL(budgets[0].get().per_block.amount, 4);
        BOOST_CHECK_EQUAL(budgets[0].get().balance.amount, 8);
    }
    generate_block();
    {
        auto budgets = post_budget_service.get_budgets(initdelegate.name);
        BOOST_CHECK(!budgets.empty());
        BOOST_CHECK_EQUAL(budgets[0].get().per_block.amount, 4);
        BOOST_CHECK_EQUAL(budgets[0].get().balance.amount, 4);
    }
    generate_block();
    {
        auto budgets = post_budget_service.get_budgets(initdelegate.name);
        BOOST_CHECK(budgets.empty());
    }
}

SCORUM_TEST_CASE(start_and_deadline_same_block)
{
    create_budget_operation op;
    op.start = db.head_block_time() + SCORUM_BLOCK_INTERVAL;
    op.deadline = db.head_block_time() + SCORUM_BLOCK_INTERVAL;
    op.balance = ASSET_SCR(12);
    op.owner = initdelegate.name;
    op.type = budget_type::post;

    BOOST_CHECK(post_budget_service.get_budgets(initdelegate.name).empty());
    push_operation(op, initdelegate.private_key, false);
    BOOST_CHECK(!post_budget_service.get_budgets(initdelegate.name).empty());
    generate_block();
    BOOST_CHECK(post_budget_service.get_budgets(initdelegate.name).empty());
}

SCORUM_TEST_CASE(starting_budget_immediately_during_creation_test)
{
    create_budget_operation op;
    op.start = db.head_block_time();
    op.deadline = db.head_block_time() + 4 * SCORUM_BLOCK_INTERVAL;
    op.balance = ASSET_SCR(20);
    op.owner = initdelegate.name;
    op.type = budget_type::post;

    BOOST_CHECK(post_budget_service.get_budgets(initdelegate.name).empty());
    push_operation(op, fc::ecc::private_key(), false);
    {
        auto budget = post_budget_service.get_budgets(initdelegate.name)[0].get();

        // Since budget start time is 'in the past' i.e. during block generation signed_block.time > budget.start_time
        // (budget.start_time is actually equal last_block_head_time), per_blocks_count will be equal 5,
        // but where will be only 4 withdrawals from budget => the very first per_block won't be withdrawn.
        // Which means that 1 per_block amount will alway be
        BOOST_CHECK_EQUAL(budget.per_block.amount, 20 / 5);
        BOOST_CHECK_EQUAL(budget.balance.amount, 20);
    }
    {
        generate_blocks(3);
        BOOST_CHECK_EQUAL(post_budget_service.get_budgets(initdelegate.name)[0].get().balance.amount, 8);

        generate_block(); // deadline reached: 4 satoshi were withdrawn and 4 go to the owner
        BOOST_CHECK(post_budget_service.get_budgets(initdelegate.name).empty());
    }
}

SCORUM_TEST_CASE(check_balance_after_deadline)
{
    auto budget_balance = 15;
    auto acc_balance = account_service.get_account(alice.name).balance;

    {
        create_budget(uuid_gen("alice"), alice, budget_type::post, budget_balance, 1, 2);
        generate_blocks(2);
        auto rest = budget_balance - (budget_balance / 2) * 2; // 1
        BOOST_CHECK_EQUAL(acc_balance.amount - budget_balance + rest,
                          account_service.get_account(alice.name).balance.amount);
        acc_balance = account_service.get_account(alice.name).balance;
    }
    {
        create_budget(uuid_gen("alice"), alice, budget_type::post, budget_balance, 1, 3);
        generate_blocks(3);
        auto rest = budget_balance - (budget_balance / 3) * 3; // 0
        BOOST_CHECK_EQUAL(acc_balance.amount - budget_balance + rest,
                          account_service.get_account(alice.name).balance.amount);
        acc_balance = account_service.get_account(alice.name).balance;
    }
    {
        create_budget(uuid_gen("alice"), alice, budget_type::post, budget_balance, 1, 4);
        generate_blocks(4);
        auto rest = budget_balance - (budget_balance / 4) * 4; // 3
        BOOST_CHECK_EQUAL(acc_balance.amount - budget_balance + rest,
                          account_service.get_account(alice.name).balance.amount);
        acc_balance = account_service.get_account(alice.name).balance;
    }
    {
        create_budget(uuid_gen("alice"), alice, budget_type::post, budget_balance, 0, 3);
        generate_blocks(3);
        auto rest = budget_balance - (budget_balance / 4) * 4; // 3
        auto very_first_per_block_which_wasnt_withdrawn = budget_balance / 4;
        BOOST_CHECK_EQUAL(acc_balance.amount - budget_balance + rest + very_first_per_block_which_wasnt_withdrawn,
                          account_service.get_account(alice.name).balance.amount);
    }
}

SCORUM_TEST_CASE(return_money_to_account_after_deadline_is_over_and_we_have_missing_blocks)
{
    int balance = 10;
    int deadline_blocks_offset = 10;

    create_budget(uuid_gen("alice"), alice, budget_type::post, balance, deadline_blocks_offset);

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
                        original_balance + ASSET_SCR(deadline_blocks_offset - actually_generated_blocks));
}

SCORUM_TEST_CASE(should_raise_closing_virt_operation_after_deadline)
{
    auto was_raised = false;
    db.pre_apply_operation.connect([&](const operation_notification& op_notif) {
        op_notif.op.weak_visit([&](const budget_closing_operation&) { was_raised = true; });
    });

    create_budget(uuid_gen("alice"), alice, budget_type::post, 1000, 1, 2);
    generate_blocks(2);

    BOOST_CHECK(was_raised);
}

SCORUM_TEST_CASE(one_satoshi_budget_should_be_closed_in_first_block)
{
    create_budget(uuid_gen("alice"), alice, budget_type::post, 1, 1, 10);

    BOOST_REQUIRE(!post_budget_service.get_budgets(alice.name).empty());

    generate_blocks(2);

    BOOST_REQUIRE(post_budget_service.get_budgets(alice.name).empty());
}

SCORUM_TEST_CASE(not_enough_money_for_each_per_block_should_be_close_when_balance_is_empty)
{
    create_budget(uuid_gen("alice"), alice, budget_type::post, 3, 1, 10); // per_block = 1

    BOOST_REQUIRE(!post_budget_service.get_budgets(alice.name).empty());

    generate_blocks(3);

    BOOST_REQUIRE(post_budget_service.get_budgets(alice.name).empty());
}

BOOST_AUTO_TEST_SUITE_END()
}
