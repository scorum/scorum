#ifdef IS_TEST_NET
#include <boost/test/unit_test.hpp>

#include <scorum/chain/dbs_account.hpp>
#include <scorum/chain/dbs_budget.hpp>

#include <scorum/chain/account_object.hpp>
#include <scorum/chain/budget_objects.hpp>

#include "../common/database_fixture.hpp"

#include <limits>

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
        , fake(account_service.create_account_by_faucets(SCORUM_ROOT_POST_PARENT,
                                                         "initdelegate",
                                                         public_key,
                                                         "",
                                                         authority(),
                                                         authority(),
                                                         authority(),
                                                         asset(0, SCORUM_SYMBOL)))
        , alice(account_service.create_account_by_faucets(
              "alice", "initdelegate", public_key, "", authority(), authority(), authority(), asset(0, SCORUM_SYMBOL)))
        , bob(account_service.create_account_by_faucets(
              "bob", "initdelegate", public_key, "", authority(), authority(), authority(), asset(0, SCORUM_SYMBOL)))
    {
        account_service.increase_balance(alice, asset(ALICE_ACCOUNT_BUDGET, SCORUM_SYMBOL));
        account_service.increase_balance(bob, asset(BOB_ACCOUNT_BUDGET, SCORUM_SYMBOL));

        default_deadline = db.get_slot_time(BLOCK_LIMIT_DEFAULT);
        //        less_then_block_limit_deadline = db.get_slot_time(BLOCK_LIMIT_DEFAULT - 1);
        //        less_then_block_limit_deadline -= SCORUM_BLOCK_INTERVAL / 2;
        //        greater_then_block_limit_deadline = db.get_slot_time(BLOCK_LIMIT_DEFAULT + 1);
        //        greater_then_block_limit_deadline += SCORUM_BLOCK_INTERVAL / 2;

        if (!m_time_printed)
        {
            const size_t w = 30;
            std::cout << std::setw(w) << "head_block_time = " << db.head_block_time().to_iso_string() << std::endl;
            for (int slot = 1; slot <= BLOCK_LIMIT_DEFAULT; ++slot)
            {
                std::stringstream title;
                title << "slot_time(" << slot << ") = ";
                std::cout << std::setw(w) << title.str() << db.get_slot_time(slot).to_iso_string() << std::endl;
            }
            std::cout << std::setw(w) << "default_deadline = " << default_deadline.to_iso_string() << std::endl;
            //            std::cout << std::setw(w) << "less_then_block_limit_deadline = "
            //                      << less_then_block_limit_deadline.to_iso_string()
            //                      << std::endl;
            //            std::cout << std::setw(w) << "greater_then_block_limit_deadline = "
            //                      << greater_then_block_limit_deadline.to_iso_string()
            //                      << std::endl;
            m_time_printed = true;
        }
    }

    dbs_budget& budget_service;
    dbs_account& account_service;
    const public_key_type public_key;
    const account_object& fake;
    const account_object& alice;
    const account_object& bob;
    fc::time_point_sec default_deadline;
    //    fc::time_point_sec less_then_block_limit_deadline;
    //    fc::time_point_sec greater_then_block_limit_deadline;
    static bool m_time_printed;

    const int FAKE_ACCOUNT_BUDGET = 0;
    const int ALICE_ACCOUNT_BUDGET = 500;
    const int BOB_ACCOUNT_BUDGET = 1001;

    const int BLOCK_LIMIT_DEFAULT = 5;
    const int BUDGET_PER_BLOCK_DEFAULT = 40;
    const int BUDGET_BALANCE_DEFAULT = 200;

    const int ITEMS_PAGE_SIZE = 100;
};

bool dbs_budget_fixture::m_time_printed = false;

BOOST_FIXTURE_TEST_SUITE(budget_service, dbs_budget_fixture)

BOOST_AUTO_TEST_CASE(is_const_ref_to_same_memory)
{
    try
    {
        asset balance(BUDGET_BALANCE_DEFAULT, SCORUM_SYMBOL);
        time_point_sec deadline(default_deadline);

        const auto& budget = budget_service.create_budget(alice, balance, deadline);

        db.modify(budget, [&](budget_object& b) { b.balance.amount -= 1; });

        BOOST_REQUIRE(budget.balance.amount == (BUDGET_BALANCE_DEFAULT - 1));
    }
    FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE(fund_budget_creation)
{
    try
    {
        asset balance(BUDGET_BALANCE_DEFAULT, SCORUM_SYMBOL);
        time_point_sec deadline(default_deadline);

        const auto& budget = budget_service.create_fund_budget(balance, deadline);

        BOOST_CHECK(budget.balance.amount == BUDGET_BALANCE_DEFAULT);
        BOOST_CHECK(budget.per_block == BUDGET_PER_BLOCK_DEFAULT);

        auto budgets = budget_service.get_fund_budgets();
        BOOST_REQUIRE(budgets.size() == 1);
    }
    FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE(owned_budget_creation)
{
    try
    {
        asset balance(BUDGET_BALANCE_DEFAULT, SCORUM_SYMBOL);
        time_point_sec deadline(default_deadline);

        auto reqired_alice_balance = alice.balance.amount;

        const auto& budget = budget_service.create_budget(alice, balance, deadline);

        BOOST_CHECK(budget.balance.amount == BUDGET_BALANCE_DEFAULT);
        BOOST_CHECK(budget.per_block == BUDGET_PER_BLOCK_DEFAULT);

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
        time_point_sec deadline(default_deadline);

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
    FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE(second_fund_budget_creation)
{
    try
    {
        asset balance(BUDGET_BALANCE_DEFAULT, SCORUM_SYMBOL);
        time_point_sec deadline(default_deadline);

        {
            const auto& budget = budget_service.create_fund_budget(balance, deadline);

            BOOST_CHECK(budget.balance.amount == BUDGET_BALANCE_DEFAULT);
            BOOST_CHECK(budget.per_block == BUDGET_PER_BLOCK_DEFAULT);

            auto budgets = budget_service.get_fund_budgets();
            BOOST_REQUIRE(budgets.size() == 1);
        }

        {
            const auto& budget = budget_service.create_fund_budget(balance, deadline);

            BOOST_CHECK(budget.balance.amount == BUDGET_BALANCE_DEFAULT);
            BOOST_CHECK(budget.per_block == BUDGET_PER_BLOCK_DEFAULT);

            auto budgets = budget_service.get_fund_budgets();
            BOOST_REQUIRE(budgets.size() == 2);
        }
    }
    FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE(fund_budget_creation_asserts)
{
    try
    {
        asset balance(BUDGET_BALANCE_DEFAULT, SCORUM_SYMBOL);
        time_point_sec deadline(default_deadline);

        asset wrong_currency_balance(BUDGET_BALANCE_DEFAULT, VESTS_SYMBOL);

        BOOST_CHECK_THROW(budget_service.create_fund_budget(wrong_currency_balance, deadline), fc::assert_exception);

        asset wrong_amount_balance(0, SCORUM_SYMBOL);

        BOOST_CHECK_THROW(budget_service.create_fund_budget(wrong_amount_balance, deadline), fc::assert_exception);

        wrong_amount_balance = asset(-100, SCORUM_SYMBOL);

        BOOST_CHECK_THROW(budget_service.create_fund_budget(wrong_amount_balance, deadline), fc::assert_exception);

        time_point_sec invalid_deadline = db.head_block_time();

        BOOST_CHECK_THROW(budget_service.create_fund_budget(balance, invalid_deadline), fc::assert_exception);

        invalid_deadline -= 1000;

        BOOST_CHECK_THROW(budget_service.create_fund_budget(balance, invalid_deadline), fc::assert_exception);
    }
    FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE(owned_budget_creation_asserts)
{
    try
    {
        asset balance(BUDGET_BALANCE_DEFAULT, SCORUM_SYMBOL);
        time_point_sec deadline(default_deadline);

        BOOST_CHECK_THROW(budget_service.create_budget(fake, balance, deadline), fc::assert_exception);

        asset wrong_currency_balance(BUDGET_BALANCE_DEFAULT, VESTS_SYMBOL);

        BOOST_CHECK_THROW(budget_service.create_budget(alice, wrong_currency_balance, deadline), fc::assert_exception);

        asset wrong_amount_balance(0, SCORUM_SYMBOL);

        BOOST_CHECK_THROW(budget_service.create_budget(alice, wrong_amount_balance, deadline), fc::assert_exception);

        wrong_amount_balance = asset(-100, SCORUM_SYMBOL);

        BOOST_CHECK_THROW(budget_service.create_budget(alice, wrong_amount_balance, deadline), fc::assert_exception);

        time_point_sec invalid_deadline = db.head_block_time();

        BOOST_CHECK_THROW(budget_service.create_budget(alice, balance, invalid_deadline), fc::assert_exception);

        invalid_deadline -= 1000;

        BOOST_CHECK_THROW(budget_service.create_budget(alice, balance, invalid_deadline), fc::assert_exception);

        asset too_large_balance(ALICE_ACCOUNT_BUDGET * 2, VESTS_SYMBOL);

        BOOST_CHECK_THROW(budget_service.create_budget(alice, too_large_balance, deadline), fc::assert_exception);
    }
    FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE(fund_budget_creation_limit)
{
    try
    {
        asset balance(BUDGET_BALANCE_DEFAULT, SCORUM_SYMBOL);
        time_point_sec deadline(default_deadline);

        for (int ci = 0; ci < SCORUM_LIMIT_FUND_BUDGETS; ++ci)
        {
            BOOST_REQUIRE_NO_THROW(budget_service.create_fund_budget(balance, deadline));
        }

        BOOST_REQUIRE_THROW(budget_service.create_fund_budget(balance, deadline), fc::assert_exception);
    }
    FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE(budget_creation_limit)
{
    try
    {
        share_type bp = SCORUM_LIMIT_BUDGETS_PER_OWNER + 1;
        BOOST_REQUIRE(BOB_ACCOUNT_BUDGET >= bp);

        asset balance(BOB_ACCOUNT_BUDGET / bp, SCORUM_SYMBOL);
        time_point_sec deadline(default_deadline);

        for (int ci = 0; ci < SCORUM_LIMIT_BUDGETS_PER_OWNER; ++ci)
        {
            BOOST_REQUIRE_NO_THROW(budget_service.create_budget(bob, balance, deadline));
        }

        BOOST_CHECK(bob.balance.amount == (BOB_ACCOUNT_BUDGET - SCORUM_LIMIT_BUDGETS_PER_OWNER * balance.amount));

        BOOST_REQUIRE_THROW(budget_service.create_budget(bob, balance, deadline), fc::assert_exception);
    }
    FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE(get_all_budgets)
{
    try
    {
        asset balance(BUDGET_BALANCE_DEFAULT, SCORUM_SYMBOL);
        time_point_sec deadline(default_deadline);

        BOOST_CHECK_NO_THROW(budget_service.create_fund_budget(balance, deadline));
        BOOST_CHECK_NO_THROW(budget_service.create_budget(alice, balance, deadline));
        BOOST_CHECK_NO_THROW(budget_service.create_budget(bob, balance, deadline));

        auto budgets = budget_service.get_budgets();
        BOOST_REQUIRE(budgets.size() == 3);
    }
    FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE(get_all_budget_count)
{
    try
    {
        asset balance(BUDGET_BALANCE_DEFAULT, SCORUM_SYMBOL);
        time_point_sec deadline(default_deadline);

        BOOST_CHECK_NO_THROW(budget_service.create_fund_budget(balance, deadline));
        BOOST_CHECK_NO_THROW(budget_service.create_budget(alice, balance, deadline));
        BOOST_CHECK_NO_THROW(budget_service.create_budget(bob, balance, deadline));

        BOOST_REQUIRE(budget_service.get_budget_count() == 3);
    }
    FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE(lookup_budget_owners)
{
    try
    {
        asset balance(BUDGET_BALANCE_DEFAULT, SCORUM_SYMBOL);
        time_point_sec deadline(default_deadline);

        BOOST_CHECK_NO_THROW(budget_service.create_fund_budget(balance, deadline));
        BOOST_CHECK_NO_THROW(budget_service.create_budget(alice, balance, deadline));
        BOOST_CHECK_NO_THROW(budget_service.create_budget(bob, balance, deadline));
        BOOST_CHECK_NO_THROW(budget_service.create_budget(bob, balance, deadline));

        BOOST_REQUIRE(budget_service.get_budget_count() == 4);

        BOOST_CHECK_THROW(budget_service.lookup_budget_owners("alice", std::numeric_limits<uint32_t>::max()),
                          fc::assert_exception);

        {
            auto owners = budget_service.lookup_budget_owners(SCORUM_ROOT_POST_PARENT, ITEMS_PAGE_SIZE);
            BOOST_REQUIRE(owners.size() == 3);
        }

        {
            auto owners = budget_service.lookup_budget_owners("alice", ITEMS_PAGE_SIZE);
            BOOST_REQUIRE(owners.size() == 2);
        }

        {
            auto owners = budget_service.lookup_budget_owners("bob", ITEMS_PAGE_SIZE);
            BOOST_REQUIRE(owners.size() == 1);
        }

        {
            auto owners = budget_service.lookup_budget_owners(SCORUM_ROOT_POST_PARENT, 2);
            BOOST_REQUIRE(owners.size() == 2);
        }
    }
    FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE(get_budgets)
{
    try
    {
        asset balance(BUDGET_BALANCE_DEFAULT, SCORUM_SYMBOL);
        time_point_sec deadline(default_deadline);

        BOOST_CHECK_THROW(budget_service.get_budgets("alice"), fc::assert_exception);

        BOOST_CHECK_NO_THROW(budget_service.create_budget(alice, balance, deadline));

        auto budgets = budget_service.get_budgets("alice");
        BOOST_REQUIRE(budgets.size() == 1);

        budget_service.create_budget(alice, balance, deadline);

        budgets = budget_service.get_budgets("alice");
        BOOST_REQUIRE(budgets.size() == 2);

        for (const budget_object& budget : budgets)
        {
            BOOST_REQUIRE(budget.owner == account_name_type("alice"));
        }
    }
    FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE(get_budget_count)
{
    try
    {
        asset balance(BUDGET_BALANCE_DEFAULT, SCORUM_SYMBOL);
        time_point_sec deadline(default_deadline);

        BOOST_CHECK_THROW(budget_service.get_budgets("alice"), fc::assert_exception);

        BOOST_CHECK_NO_THROW(budget_service.create_budget(alice, balance, deadline));
        BOOST_CHECK_NO_THROW(budget_service.create_budget(alice, balance, deadline));
        BOOST_CHECK_NO_THROW(budget_service.create_budget(bob, balance, deadline));

        BOOST_REQUIRE(budget_service.get_budget_count("alice") == 2);
        BOOST_REQUIRE(budget_service.get_budget_count("bob") == 1);
    }
    FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE(get_fund_budget_count)
{
    try
    {
        asset balance(BUDGET_BALANCE_DEFAULT, SCORUM_SYMBOL);
        time_point_sec deadline(default_deadline);

        BOOST_CHECK_THROW(budget_service.get_budgets("alice"), fc::assert_exception);

        BOOST_CHECK_NO_THROW(budget_service.create_fund_budget(balance, deadline));
        BOOST_CHECK_NO_THROW(budget_service.create_fund_budget(balance, deadline));

        BOOST_REQUIRE(budget_service.get_fund_budget_count() == 2);
    }
    FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE(close_budget)
{
    try
    {
        asset balance(BUDGET_BALANCE_DEFAULT, SCORUM_SYMBOL);
        time_point_sec deadline(default_deadline);

        BOOST_CHECK_THROW(budget_service.get_budgets("alice"), fc::assert_exception);

        auto reqired_alice_balance = alice.balance.amount;

        const auto& budget = budget_service.create_budget(alice, balance, deadline);

        reqired_alice_balance -= BUDGET_BALANCE_DEFAULT;

        auto budgets = budget_service.get_budgets("alice");
        BOOST_REQUIRE(!budgets.empty());

        BOOST_CHECK_NO_THROW(budget_service.close_budget(budget));

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
        time_point_sec deadline(default_deadline);

        const auto& budget = budget_service.create_budget(alice, balance, deadline);

        auto cash = budget_service.allocate_cash(budget);

        BOOST_REQUIRE(cash.amount == 0); // wait next block
    }
    FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE(allocate_cash_next_block)
{
    try
    {
        db_plugin->debug_update(
            [=](database&) {
                asset balance(BUDGET_BALANCE_DEFAULT, SCORUM_SYMBOL);
                time_point_sec deadline(default_deadline);

                budget_service.create_fund_budget(balance, deadline);

            },
            default_skip);

        generate_block();

        {
            auto budgets = budget_service.get_fund_budgets();

            BOOST_REQUIRE(!budgets.empty());

            const budget_object& budget = budgets[0];

            auto cash = budget_service.allocate_cash(budget);

            BOOST_REQUIRE(cash.amount == BUDGET_PER_BLOCK_DEFAULT);
        }

        {
            auto budgets = budget_service.get_fund_budgets();

            BOOST_REQUIRE(!budgets.empty());

            const budget_object& budget = budgets[0];

            BOOST_REQUIRE(budget.balance.amount == (BUDGET_BALANCE_DEFAULT - BUDGET_PER_BLOCK_DEFAULT));
        }
    }
    FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE(auto_close_fund_budget_by_deadline)
{
    try
    {
        db_plugin->debug_update(
            [=](database&) {
                asset balance(BUDGET_BALANCE_DEFAULT, SCORUM_SYMBOL);
                time_point_sec deadline(default_deadline);

                budget_service.create_fund_budget(balance, deadline);

            },
            default_skip);

        BOOST_REQUIRE_NO_THROW(budget_service.get_fund_budgets());

        asset total_cash(0, SCORUM_SYMBOL);

        for (int ci = 0; ci < BLOCK_LIMIT_DEFAULT; ++ci)
        {
            generate_block();
            generate_block();

            db_plugin->debug_update(
                [&](database&) {
                    if (budget_service.get_fund_budget_count() > 0)
                    {
                        const budget_object& budget = budget_service.get_fund_budgets()[0];
                        asset cash = budget_service.allocate_cash(budget);
                        total_cash += cash;
                    }
                },
                default_skip);
        }

        BOOST_REQUIRE_THROW(budget_service.get_fund_budgets(), fc::assert_exception);

        BOOST_REQUIRE(total_cash.amount == BUDGET_BALANCE_DEFAULT);
    }
    FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE(auto_close_fund_budget_by_balance)
{
    try
    {
        db_plugin->debug_update(
            [=](database&) {
                asset balance(BUDGET_BALANCE_DEFAULT, SCORUM_SYMBOL);
                time_point_sec deadline(time_point_sec::maximum());

                const budget_object& budget = budget_service.create_fund_budget(balance, deadline);

                BOOST_REQUIRE(budget.per_block == 1);
            },
            default_skip);

        BOOST_REQUIRE_NO_THROW(budget_service.get_fund_budgets());

        asset total_cash(0, SCORUM_SYMBOL);

        for (int ci = 0; true; ++ci)
        {
            BOOST_REQUIRE(ci <= BUDGET_BALANCE_DEFAULT);

            generate_block();
            generate_block();

            db_plugin->debug_update(
                [&](database&) {
                    if (budget_service.get_fund_budget_count() > 0)
                    {
                        const budget_object& budget = budget_service.get_fund_budgets()[0];
                        asset cash = budget_service.allocate_cash(budget);
                        total_cash += cash;
                    }
                },
                default_skip);

            if (!budget_service.get_fund_budget_count())
            {
                break;
            }
        }

        BOOST_REQUIRE_THROW(budget_service.get_fund_budgets(), fc::assert_exception);

        BOOST_REQUIRE(total_cash.amount == BUDGET_BALANCE_DEFAULT);
    }
    FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE(try_close_fund_budget)
{
    try
    {
        asset balance(BUDGET_BALANCE_DEFAULT, SCORUM_SYMBOL);
        time_point_sec deadline(default_deadline);

        const budget_object& budget = budget_service.create_fund_budget(balance, deadline);

        BOOST_REQUIRE_NO_THROW(budget_service.get_fund_budgets());

        BOOST_REQUIRE_THROW(budget_service.close_budget(budget);, fc::assert_exception);
    }
    FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_SUITE_END()

#endif
