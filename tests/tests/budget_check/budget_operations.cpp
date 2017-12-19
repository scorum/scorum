#ifdef IS_TEST_NET
#include <boost/test/unit_test.hpp>

#include <scorum/chain/scorum_objects.hpp>

#include <scorum/chain/dbs_account.hpp>
#include <scorum/chain/dbs_budget.hpp>

#include "database_fixture.hpp"

#include <limits>

using namespace scorum;
using namespace scorum::chain;
using namespace scorum::protocol;
using fc::string;

//
// usage for all budget tests 'chain_test  -t budget_*'
//

class budget_operation_check_fixture : public timed_blocks_database_fixture
{
public:
    budget_operation_check_fixture()
    {
        create_budget_op.owner = "alice";
        create_budget_op.content_permlink = BUDGET_CONTENT_PERMLINK;
        create_budget_op.deadline = default_deadline;
        create_budget_op.balance = asset(BUDGET_BALANCE_DEFAULT, SCORUM_SYMBOL);
        close_budget_op.budget_id = 1;
        close_budget_op.owner = "alice";
    }

    create_budget_operation create_budget_op;
    close_budget_operation close_budget_op;

    const int BUDGET_BALANCE_DEFAULT = 200;
    const char* BUDGET_CONTENT_PERMLINK = "https://github.com/scorum/scorum/blob/master/README.md";
};

BOOST_FIXTURE_TEST_SUITE(budget_operation_check, budget_operation_check_fixture)

SCORUM_TEST_CASE(create_budget_operation_check)
{
    BOOST_REQUIRE_NO_THROW(create_budget_op.validate());
}

SCORUM_TEST_CASE(create_budget_operation_check_invalid_balance_amount)
{
    create_budget_op.balance = asset(0, SCORUM_SYMBOL);

    BOOST_REQUIRE_THROW(create_budget_op.validate(), fc::assert_exception);

    create_budget_op.balance = asset(-BUDGET_BALANCE_DEFAULT, SCORUM_SYMBOL);

    BOOST_REQUIRE_THROW(create_budget_op.validate(), fc::assert_exception);
}

SCORUM_TEST_CASE(create_budget_operation_check_invalid_balance_currency)
{
    create_budget_op.balance = asset(BUDGET_BALANCE_DEFAULT, VESTS_SYMBOL);

    BOOST_REQUIRE_THROW(create_budget_op.validate(), fc::assert_exception);
}

SCORUM_TEST_CASE(create_budget_operation_check_invalid_owner_name)
{
    create_budget_op.owner = "";

    BOOST_REQUIRE_THROW(create_budget_op.validate(), fc::assert_exception);

    create_budget_op.owner = "wrong;\n'j'";

    BOOST_REQUIRE_THROW(create_budget_op.validate(), fc::assert_exception);
}

SCORUM_TEST_CASE(create_budget_operation_check_invalid_content_permlink)
{
    create_budget_op.content_permlink = "\xff\x20\xbf";

    BOOST_REQUIRE_THROW(create_budget_op.validate(), fc::assert_exception);
}

SCORUM_TEST_CASE(close_budget_operation_check)
{
    BOOST_REQUIRE_NO_THROW(close_budget_op.validate());
}

SCORUM_TEST_CASE(close_budget_operation_check_invalid_owner_name)
{
    close_budget_op.owner = "";

    BOOST_REQUIRE_THROW(close_budget_op.validate(), fc::assert_exception);

    close_budget_op.owner = "dskf;k30kfl;kvfakg'04i\'j'[IGRIREW40KIA'AKG'K'RK-]]4-4AP[GIRAIORPIGOPRG";

    BOOST_REQUIRE_THROW(close_budget_op.validate(), fc::assert_exception);
}

BOOST_AUTO_TEST_SUITE_END()

class budget_transaction_check_fixture : public budget_operation_check_fixture
{
public:
    budget_transaction_check_fixture()
        : budget_service(db.obtain_service<dbs_budget>())
        , account_service(db.obtain_service<dbs_account>())
    {
    }

    dbs_budget& budget_service;
    dbs_account& account_service;

    private_key_type alice_create_budget(const asset& balance, const fc::time_point_sec& deadline);
};

private_key_type budget_transaction_check_fixture::alice_create_budget(const asset& balance,
                                                                       const fc::time_point_sec& deadline)
{
    BOOST_REQUIRE(BLOCK_LIMIT_DEFAULT > 0);

    ACTORS((alice))

    fund("alice", BUDGET_BALANCE_DEFAULT);

    create_budget_operation op;
    op.owner = "alice";
    op.content_permlink = BUDGET_CONTENT_PERMLINK;
    op.balance = balance;
    op.deadline = deadline;

    BOOST_REQUIRE_NO_THROW(op.validate());

    signed_transaction tx;

    tx.set_expiration(db.head_block_time() + SCORUM_MAX_TIME_UNTIL_EXPIRATION);
    tx.operations.push_back(op);

    BOOST_REQUIRE_NO_THROW(tx.sign(alice_private_key, db.get_chain_id()));
    BOOST_REQUIRE_NO_THROW(tx.validate());

    BOOST_REQUIRE_NO_THROW(db.push_transaction(tx, 0));

    BOOST_REQUIRE(budget_service.get_budgets("alice").size() == 1);

    return alice_private_key;
}

BOOST_FIXTURE_TEST_SUITE(budget_transaction_check, budget_transaction_check_fixture)

SCORUM_TEST_CASE(create_budget_check)
{
    asset balance(BUDGET_BALANCE_DEFAULT, SCORUM_SYMBOL);

    BOOST_REQUIRE_NO_THROW(alice_create_budget(balance, default_deadline));

    const budget_object& budget = (*budget_service.get_budgets("alice").cbegin());

    BOOST_REQUIRE(budget.owner == "alice");
    BOOST_REQUIRE(!budget.content_permlink.compare(BUDGET_CONTENT_PERMLINK));
    BOOST_REQUIRE(budget.balance == balance);
    BOOST_REQUIRE(budget.deadline == default_deadline);

    BOOST_REQUIRE_NO_THROW(validate_database());
}

SCORUM_TEST_CASE(close_budget_check)
{
    private_key_type alice_private_key
        = alice_create_budget(asset(BUDGET_BALANCE_DEFAULT, SCORUM_SYMBOL), default_deadline);

    const budget_object& budget = (*budget_service.get_budgets("alice").cbegin());

    close_budget_operation close_op;
    close_op.owner = "alice";
    close_op.budget_id = budget.id._id;

    BOOST_REQUIRE_NO_THROW(close_op.validate());

    signed_transaction tx;

    tx.set_expiration(db.head_block_time() + SCORUM_MAX_TIME_UNTIL_EXPIRATION);
    tx.operations.push_back(close_op);

    BOOST_REQUIRE_NO_THROW(tx.sign(alice_private_key, db.get_chain_id()));
    BOOST_REQUIRE_NO_THROW(tx.validate());

    BOOST_REQUIRE_NO_THROW(db.push_transaction(tx, 0));

    BOOST_REQUIRE(budget_service.get_budgets("alice").size() == 0);

    BOOST_REQUIRE_NO_THROW(validate_database());
}

SCORUM_TEST_CASE(auto_close_budget_by_balance)
{
    BOOST_REQUIRE_NO_THROW(
        alice_create_budget(asset(BUDGET_BALANCE_DEFAULT, SCORUM_SYMBOL), time_point_sec::maximum()));

    BOOST_REQUIRE(!budget_service.get_budgets("alice").empty());

    asset total_cash(0, SCORUM_SYMBOL);

    for (int ci = 0; true; ++ci)
    {
        BOOST_REQUIRE(ci <= BUDGET_BALANCE_DEFAULT);

        if (budget_service.get_budgets("alice").empty())
        {
            break; // budget has closed, break and check result
        }

        generate_block();

        db_plugin->debug_update(
            [&](database&) {
                if (!budget_service.get_budgets("alice").empty())
                {
                    const budget_object& budget = (*budget_service.get_budgets("alice").cbegin());
                    total_cash += budget_service.allocate_cash(budget);
                }
            },
            default_skip);
    }

    BOOST_REQUIRE(budget_service.get_budgets("alice").empty());

    BOOST_REQUIRE(total_cash.amount == BUDGET_BALANCE_DEFAULT);
}

BOOST_AUTO_TEST_SUITE_END()

#endif
