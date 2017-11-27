#ifdef IS_TEST_NET
#include <boost/test/unit_test.hpp>

#include <scorum/chain/dbs_account.hpp>
#include <scorum/chain/dbs_budget.hpp>

#include <scorum/chain/account_object.hpp>
#include <scorum/chain/budget_objects.hpp>

#include "../common/database_fixture.hpp"

using namespace scorum;
using namespace scorum::chain;
using namespace scorum::protocol;
using fc::string;

private_key_type generate_private_key(const std::string& str)
{
    return private_key_type::regenerate(fc::sha256::hash(std::string(str)));
}

class dbs_budget_fixture : public clean_database_fixture
{
public:
    dbs_budget_fixture()
        : budget_service(db.obtain_service<dbs_budget>())
        , account_service(db.obtain_service<dbs_account>())
        , public_key(generate_private_key("user private key").get_public_key())
        , alice(account_service.create_account_by_faucets(
              "alice", "initdelegate", public_key, "", authority(), authority(), authority(), asset(0, SCORUM_SYMBOL)))
        , bob(account_service.create_account_by_faucets(
              "bob", "initdelegate", public_key, "", authority(), authority(), authority(), asset(0, SCORUM_SYMBOL)))
    {
        account_service.increase_balance(alice, asset(500, SCORUM_SYMBOL));
        account_service.increase_balance(bob, asset(500, SCORUM_SYMBOL));
    }

    dbs_budget& budget_service;
    dbs_account& account_service;
    const public_key_type public_key;
    const account_object& alice;
    const account_object& bob;

    const int BUDGET_PER_BLOCK_DEFAULT = 50;
    const int BUDGET_BALANCE_DEFAULT = 200;
};

BOOST_FIXTURE_TEST_SUITE(budget_service, dbs_budget_fixture)

BOOST_AUTO_TEST_CASE(genesis_budget_creation)
{
    try
    {
        asset balance(BUDGET_BALANCE_DEFAULT, SCORUM_SYMBOL);
        time_point_sec deadline(time_point_sec::maximum());

        const auto& budget = budget_service.create_fund_budget(balance, BUDGET_PER_BLOCK_DEFAULT, deadline);

        BOOST_REQUIRE(budget.balance.amount == BUDGET_BALANCE_DEFAULT);

        auto budget_ids = budget_service.get_budgets(SCORUM_ROOT_POST_PARENT);
        BOOST_REQUIRE(budget_ids.size() == 1);
    }
    FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE(owned_budget_creation)
{
    try
    {
        asset balance(BUDGET_BALANCE_DEFAULT, SCORUM_SYMBOL);
        time_point_sec deadline(time_point_sec::maximum());

        auto reqired_alice_balance = alice.balance.amount;

        const auto& budget
            = budget_service.create_budget(alice, optional<string>(), balance, BUDGET_PER_BLOCK_DEFAULT, deadline);

        BOOST_REQUIRE(budget.balance.amount == BUDGET_BALANCE_DEFAULT);

        reqired_alice_balance -= BUDGET_BALANCE_DEFAULT;

        const auto& actual_account = account_service.get_account("alice");

        BOOST_REQUIRE(actual_account.balance.amount == reqired_alice_balance);
    }
    FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE(second_owned_budget_creation)
{
    try
    {
        asset balance(BUDGET_BALANCE_DEFAULT, SCORUM_SYMBOL);
        time_point_sec deadline(time_point_sec::maximum());

        auto reqired_alice_balance = alice.balance.amount;

        budget_service.create_budget(alice, optional<string>(), balance, BUDGET_PER_BLOCK_DEFAULT, deadline);

        reqired_alice_balance -= BUDGET_BALANCE_DEFAULT;

        const auto& budget
            = budget_service.create_budget(alice, optional<string>(), balance, BUDGET_PER_BLOCK_DEFAULT, deadline);

        BOOST_REQUIRE(budget.balance.amount == BUDGET_BALANCE_DEFAULT);

        reqired_alice_balance -= BUDGET_BALANCE_DEFAULT;

        const auto& actual_account = account_service.get_account("alice");

        BOOST_REQUIRE(actual_account.balance.amount == reqired_alice_balance);
    }
    FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE(get_budgets)
{
    try
    {
        asset balance(BUDGET_BALANCE_DEFAULT, SCORUM_SYMBOL);
        time_point_sec deadline(time_point_sec::maximum());

        BOOST_CHECK_THROW(budget_service.get_budgets("alice"), fc::assert_exception);

        budget_service.create_budget(alice, optional<string>(), balance, BUDGET_PER_BLOCK_DEFAULT, deadline);

        auto budget_ids = budget_service.get_budgets("alice");
        BOOST_REQUIRE(budget_ids.size() == 1);

        budget_service.create_budget(alice, optional<string>(), balance, BUDGET_PER_BLOCK_DEFAULT, deadline);

        budget_ids = budget_service.get_budgets("alice");
        BOOST_REQUIRE(budget_ids.size() == 2);
    }
    FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE(get_budget)
{
    try
    {
        asset balance(BUDGET_BALANCE_DEFAULT, SCORUM_SYMBOL);
        time_point_sec deadline(time_point_sec::maximum());

        budget_service.create_budget(alice, optional<string>(), balance, BUDGET_PER_BLOCK_DEFAULT, deadline);
        budget_service.create_budget(alice, optional<string>(), balance, BUDGET_PER_BLOCK_DEFAULT, deadline);

        auto budget_ids = budget_service.get_budgets("alice");
        BOOST_REQUIRE(budget_ids.size() == 2);

        budget_service.get_budget(budget_ids[0]);
        budget_service.get_budget(budget_ids[1]);
    }
    FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE(get_any_budget)
{
    try
    {
        asset balance(BUDGET_BALANCE_DEFAULT, SCORUM_SYMBOL);
        time_point_sec deadline(time_point_sec::maximum());

        BOOST_CHECK_THROW(budget_service.get_any_budget("alice"), fc::assert_exception);

        budget_service.create_budget(alice, optional<string>(), balance, BUDGET_PER_BLOCK_DEFAULT, deadline);
        budget_service.create_budget(alice, optional<string>(), balance, BUDGET_PER_BLOCK_DEFAULT, deadline);

        budget_service.get_any_budget("alice");
    }
    FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE(close_budget)
{
    try
    {
        asset balance(BUDGET_BALANCE_DEFAULT, SCORUM_SYMBOL);
        time_point_sec deadline(time_point_sec::maximum());

        BOOST_CHECK_THROW(budget_service.get_any_budget("alice"), fc::assert_exception);

        auto reqired_alice_balance = alice.balance.amount;

        const auto& budget
            = budget_service.create_budget(alice, optional<string>(), balance, BUDGET_PER_BLOCK_DEFAULT, deadline);

        reqired_alice_balance -= BUDGET_BALANCE_DEFAULT;

        auto budget_ids = budget_service.get_budgets("alice");
        BOOST_REQUIRE(!budget_ids.empty());

        budget_service.close_budget(budget);

        reqired_alice_balance += BUDGET_BALANCE_DEFAULT;

        BOOST_CHECK_THROW(budget_service.get_budgets("alice"), fc::assert_exception);

        const auto& actual_account = account_service.get_account("alice");

        BOOST_REQUIRE(reqired_alice_balance == actual_account.balance.amount);
    }
    FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE(allocate_cash_no_block)
{
    try
    {
        asset balance(BUDGET_BALANCE_DEFAULT, SCORUM_SYMBOL);
        time_point_sec deadline(time_point_sec::maximum());

        const auto& budget
            = budget_service.create_budget(alice, optional<string>(), balance, BUDGET_PER_BLOCK_DEFAULT, deadline);

        auto cash = budget_service.allocate_cash(budget);

        BOOST_REQUIRE(cash.amount == 0); // wait next block
    }
    FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE(allocate_cash_next_block)
{
    try
    {
        // TODO
    }
    FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_SUITE_END()

#endif
