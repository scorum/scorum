#ifdef IS_TEST_NET
#include <boost/test/unit_test.hpp>

#include <scorum/chain/services/account.hpp>
#include <scorum/chain/services/budget.hpp>

#include <scorum/chain/schema/account_objects.hpp>
#include <scorum/chain/schema/budget_object.hpp>

#include "budget_check_common.hpp"

#include <limits>

using namespace scorum::chain;
using namespace scorum::protocol;
using namespace budget_fixtures;

//
// usage for all budget tests 'chain_test  -t budget_*'
//

class budget_service_check_fixture : public budget_check_fixture
{
public:
    budget_service_check_fixture()
        : budget_service(db.obtain_service<dbs_budget>())
        , account_service(db.obtain_service<dbs_account>())
        , public_key(database_integration_fixture::generate_private_key("user private key").get_public_key())
        , fake(account_service.create_account(SCORUM_ROOT_POST_PARENT_ACCOUNT,
                                              TEST_INIT_DELEGATE_NAME,
                                              public_key,
                                              "",
                                              authority(),
                                              authority(),
                                              authority(),
                                              asset(0, SCORUM_SYMBOL)))
        , alice(account_service.create_account("alice",
                                               TEST_INIT_DELEGATE_NAME,
                                               public_key,
                                               "",
                                               authority(),
                                               authority(),
                                               authority(),
                                               asset(0, SCORUM_SYMBOL)))
        , bob(account_service.create_account("bob",
                                             TEST_INIT_DELEGATE_NAME,
                                             public_key,
                                             "",
                                             authority(),
                                             authority(),
                                             authority(),
                                             asset(0, SCORUM_SYMBOL)))
    {
        account_service.increase_balance(alice, asset(ALICE_ACCOUNT_BUDGET, SCORUM_SYMBOL));
        account_service.increase_balance(bob, asset(BOB_ACCOUNT_BUDGET, SCORUM_SYMBOL));
    }

    dbs_budget& budget_service;
    dbs_account& account_service;
    const public_key_type public_key;
    const account_object& fake;
    const account_object& alice;
    const account_object& bob;

    const int FAKE_ACCOUNT_BUDGET = 0;
    const int ALICE_ACCOUNT_BUDGET = 500;
    const int BOB_ACCOUNT_BUDGET = 1001;

    const int BUDGET_PER_BLOCK_DEFAULT = 1;
    const int BUDGET_BALANCE_DEFAULT = 5;
};

BOOST_FIXTURE_TEST_SUITE(budget_service_check, budget_service_check_fixture)

SCORUM_TEST_CASE(is_const_ref_to_same_memory)
{
    asset balance(BUDGET_BALANCE_DEFAULT, SCORUM_SYMBOL);
    fc::time_point_sec deadline(default_deadline);

    const auto& budget = budget_service.create_budget(alice, balance, deadline);

    db.modify(budget, [&](budget_object& b) { b.balance.amount -= 1; });

    BOOST_REQUIRE(budget.balance.amount == (BUDGET_BALANCE_DEFAULT - 1));
}

SCORUM_TEST_CASE(owned_budget_creation)
{
    asset balance(BUDGET_BALANCE_DEFAULT, SCORUM_SYMBOL);
    fc::time_point_sec deadline(default_deadline);

    auto reqired_alice_balance = alice.balance.amount;

    const auto& budget = budget_service.create_budget(alice, balance, deadline);

    BOOST_CHECK(budget.balance.amount == BUDGET_BALANCE_DEFAULT);
    BOOST_CHECK(budget.per_block == BUDGET_PER_BLOCK_DEFAULT);

    reqired_alice_balance -= BUDGET_BALANCE_DEFAULT;

    const auto& actual_account = account_service.get_account("alice");

    BOOST_REQUIRE(actual_account.balance.amount == reqired_alice_balance);
}

SCORUM_TEST_CASE(second_owned_budget_creation)
{
    asset balance(BUDGET_BALANCE_DEFAULT, SCORUM_SYMBOL);
    fc::time_point_sec deadline(default_deadline);

    auto reqired_alice_balance = alice.balance.amount;

    BOOST_CHECK_NO_THROW(budget_service.create_budget(alice, balance, deadline));

    reqired_alice_balance -= BUDGET_BALANCE_DEFAULT;

    const auto& budget = budget_service.create_budget(alice, balance, deadline);

    BOOST_CHECK(budget.balance.amount == BUDGET_BALANCE_DEFAULT);
    BOOST_CHECK(budget.per_block == BUDGET_PER_BLOCK_DEFAULT);

    reqired_alice_balance -= BUDGET_BALANCE_DEFAULT;

    const auto& actual_account = account_service.get_account("alice");

    BOOST_REQUIRE(actual_account.balance.amount == reqired_alice_balance);
}

SCORUM_TEST_CASE(owned_budget_creation_asserts)
{
    asset balance(BUDGET_BALANCE_DEFAULT, SCORUM_SYMBOL);
    fc::time_point_sec deadline(default_deadline);

    BOOST_CHECK_THROW(budget_service.create_budget(fake, balance, deadline), fc::assert_exception);

    asset wrong_currency_balance(BUDGET_BALANCE_DEFAULT, VESTS_SYMBOL);

    BOOST_CHECK_THROW(budget_service.create_budget(alice, wrong_currency_balance, deadline), fc::assert_exception);

    asset wrong_amount_balance(0, SCORUM_SYMBOL);

    BOOST_CHECK_THROW(budget_service.create_budget(alice, wrong_amount_balance, deadline), fc::assert_exception);

    wrong_amount_balance = asset(-100, SCORUM_SYMBOL);

    BOOST_CHECK_THROW(budget_service.create_budget(alice, wrong_amount_balance, deadline), fc::assert_exception);

    fc::time_point_sec invalid_deadline = db.head_block_time();

    BOOST_CHECK_THROW(budget_service.create_budget(alice, balance, invalid_deadline), fc::assert_exception);

    invalid_deadline -= 1000;

    BOOST_CHECK_THROW(budget_service.create_budget(alice, balance, invalid_deadline), fc::assert_exception);

    asset too_large_balance(ALICE_ACCOUNT_BUDGET * 2, VESTS_SYMBOL);

    BOOST_CHECK_THROW(budget_service.create_budget(alice, too_large_balance, deadline), fc::assert_exception);
}

SCORUM_TEST_CASE(budget_creation_limit)
{
    share_type bp = SCORUM_BUDGET_LIMIT_COUNT_PER_OWNER + 1;
    BOOST_REQUIRE(BOB_ACCOUNT_BUDGET >= bp);

    asset balance(BOB_ACCOUNT_BUDGET / bp, SCORUM_SYMBOL);
    fc::time_point_sec deadline(default_deadline);

    for (int ci = 0; ci < SCORUM_BUDGET_LIMIT_COUNT_PER_OWNER; ++ci)
    {
        BOOST_REQUIRE_NO_THROW(budget_service.create_budget(bob, balance, deadline));
    }

    BOOST_CHECK(bob.balance.amount == (BOB_ACCOUNT_BUDGET - SCORUM_BUDGET_LIMIT_COUNT_PER_OWNER * balance.amount));

    BOOST_REQUIRE_THROW(budget_service.create_budget(bob, balance, deadline), fc::assert_exception);
}

SCORUM_TEST_CASE(get_all_budgets)
{
    asset balance(BUDGET_BALANCE_DEFAULT, SCORUM_SYMBOL);
    fc::time_point_sec deadline(default_deadline);

    BOOST_CHECK_NO_THROW(budget_service.get_fund_budget());
    BOOST_CHECK_NO_THROW(budget_service.create_budget(alice, balance, deadline));
    BOOST_CHECK_NO_THROW(budget_service.create_budget(bob, balance, deadline));

    auto budgets = budget_service.get_budgets();
    BOOST_REQUIRE(budgets.size() == 3);
}

SCORUM_TEST_CASE(get_all_budget_count)
{
    asset balance(BUDGET_BALANCE_DEFAULT, SCORUM_SYMBOL);
    fc::time_point_sec deadline(default_deadline);

    BOOST_CHECK_NO_THROW(budget_service.get_fund_budget());
    BOOST_CHECK_NO_THROW(budget_service.create_budget(alice, balance, deadline));
    BOOST_CHECK_NO_THROW(budget_service.create_budget(bob, balance, deadline));

    BOOST_REQUIRE(budget_service.get_budgets().size() == 3);
}

SCORUM_TEST_CASE(lookup_budget_owners)
{
    asset balance(BUDGET_BALANCE_DEFAULT, SCORUM_SYMBOL);
    fc::time_point_sec deadline(default_deadline);

    BOOST_REQUIRE_GT(SCORUM_BUDGET_LIMIT_DB_LIST_SIZE, 1);

    BOOST_CHECK_NO_THROW(budget_service.get_fund_budget());
    BOOST_CHECK_NO_THROW(budget_service.create_budget(alice, balance, deadline));
    BOOST_CHECK_NO_THROW(budget_service.create_budget(bob, balance, deadline));
    BOOST_CHECK_NO_THROW(budget_service.create_budget(bob, balance, deadline));

    BOOST_REQUIRE(budget_service.get_budgets().size() == 4);

    BOOST_CHECK_THROW(budget_service.lookup_budget_owners("alice", std::numeric_limits<uint32_t>::max()),
                      fc::assert_exception);

    {
        auto owners
            = budget_service.lookup_budget_owners(SCORUM_ROOT_POST_PARENT_ACCOUNT, SCORUM_BUDGET_LIMIT_DB_LIST_SIZE);
        BOOST_REQUIRE(owners.size() == 2);
    }

    {
        auto owners = budget_service.lookup_budget_owners("alice", SCORUM_BUDGET_LIMIT_DB_LIST_SIZE);
        BOOST_REQUIRE(owners.size() == 2);
    }

    {
        auto owners = budget_service.lookup_budget_owners("bob", SCORUM_BUDGET_LIMIT_DB_LIST_SIZE);
        BOOST_REQUIRE(owners.size() == 1);
    }

    {
        auto owners = budget_service.lookup_budget_owners(SCORUM_ROOT_POST_PARENT_ACCOUNT, 1);
        BOOST_REQUIRE(owners.size() == 1);
    }
}

SCORUM_TEST_CASE(check_get_budgets)
{
    asset balance(BUDGET_BALANCE_DEFAULT, SCORUM_SYMBOL);
    fc::time_point_sec deadline(default_deadline);

    BOOST_REQUIRE_EQUAL(budget_service.get_budgets("alice").size(), 0u);

    BOOST_CHECK_NO_THROW(budget_service.create_budget(alice, balance, deadline));

    BOOST_REQUIRE_EQUAL(budget_service.get_budgets("alice").size(), 1u);

    BOOST_CHECK_NO_THROW(budget_service.create_budget(alice, balance, deadline));

    BOOST_REQUIRE_EQUAL(budget_service.get_budgets("alice").size(), 2u);

    for (const budget_object& budget : budget_service.get_budgets("alice"))
    {
        BOOST_REQUIRE(budget.owner == account_name_type("alice"));
    }
}

SCORUM_TEST_CASE(check_get_budget_count)
{
    asset balance(BUDGET_BALANCE_DEFAULT, SCORUM_SYMBOL);
    fc::time_point_sec deadline(default_deadline);

    BOOST_REQUIRE_EQUAL(budget_service.get_budgets("alice").size(), 0u);

    BOOST_CHECK_NO_THROW(budget_service.create_budget(alice, balance, deadline));
    BOOST_CHECK_NO_THROW(budget_service.create_budget(alice, balance, deadline));
    BOOST_CHECK_NO_THROW(budget_service.create_budget(bob, balance, deadline));

    BOOST_REQUIRE_EQUAL(budget_service.get_budgets("alice").size(), 2u);
    BOOST_REQUIRE_EQUAL(budget_service.get_budgets("bob").size(), 1u);
}

SCORUM_TEST_CASE(check_close_budget)
{
    asset balance(BUDGET_BALANCE_DEFAULT, SCORUM_SYMBOL);
    fc::time_point_sec deadline(default_deadline);

    auto reqired_alice_balance = alice.balance.amount;

    const auto& budget = budget_service.create_budget(alice, balance, deadline);

    reqired_alice_balance -= BUDGET_BALANCE_DEFAULT;

    auto budgets = budget_service.get_budgets("alice");
    BOOST_REQUIRE(!budgets.empty());

    BOOST_CHECK_NO_THROW(budget_service.close_budget(budget));

    reqired_alice_balance += BUDGET_BALANCE_DEFAULT;

    BOOST_REQUIRE(budget_service.get_budgets("alice").empty());

    const auto& actual_account = account_service.get_account("alice");

    BOOST_REQUIRE_EQUAL(reqired_alice_balance, actual_account.balance.amount);
}

SCORUM_TEST_CASE(allocate_cash_per_block)
{
    asset balance(BUDGET_BALANCE_DEFAULT, SCORUM_SYMBOL);
    fc::time_point_sec deadline(default_deadline);

    const auto& budget = budget_service.create_budget(alice, balance, deadline);

    auto cash = budget_service.allocate_cash(budget);

    BOOST_REQUIRE_EQUAL(cash.amount, 0); // wait next block
}

BOOST_AUTO_TEST_SUITE_END()

#endif
