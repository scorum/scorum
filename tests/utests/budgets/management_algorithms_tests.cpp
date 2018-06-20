#include <boost/test/unit_test.hpp>

#include <scorum/chain/services/dynamic_global_property.hpp>
#include <scorum/chain/services/budgets.hpp>
#include <scorum/chain/services/account.hpp>

#include <scorum/chain/database/budget_management_algorithms.hpp>

#include "defines.hpp"

#include "actor.hpp"

#include "service_wrappers.hpp"

namespace management_algorithms_tests {

struct fixture : public shared_memory_fixture
{
    MockRepository mocks;

    fc::time_point_sec head_block_time = fc::time_point_sec::from_iso_string("2018-07-01T00:00:00");

    dynamic_global_property_service_wrapper dgp_service_fixture;

    fixture()
        : dgp_service_fixture(*this, mocks, [&](dynamic_global_property_object& p) {
            p.time = head_block_time;
            p.head_block_number = 1;
        })
    {
    }
};

struct budget_fixture : public fixture
{
    fc::time_point_sec start_time = head_block_time + fc::seconds(SCORUM_BLOCK_INTERVAL * 100);
    fc::time_point_sec deadline = start_time + fc::seconds(SCORUM_BLOCK_INTERVAL * 100);
    share_type balance = 1000200;
};

struct fund_budget_fixture : public budget_fixture
{
    service_base_wrapper<fund_budget_service_i> fund_budget_service_fixture;

    fund_budget_fixture()
        : fund_budget_service_fixture(*this, mocks)
    {
    }
};

BOOST_FIXTURE_TEST_CASE(fund_budget_creation, fund_budget_fixture)
{
    fund_budget_management_algorithm creator(fund_budget_service_fixture.service(), dgp_service_fixture.service());

    SCORUM_REQUIRE_THROW(
        creator.create_budget(SCORUM_ROOT_POST_PARENT_ACCOUNT, asset(balance, SCORUM_SYMBOL), start_time, deadline, ""),
        fc::assert_exception);

    SCORUM_REQUIRE_THROW(
        creator.create_budget(SCORUM_ROOT_POST_PARENT_ACCOUNT, asset(balance, SP_SYMBOL), deadline, start_time, ""),
        fc::assert_exception);

    BOOST_REQUIRE_NO_THROW(
        creator.create_budget(SCORUM_ROOT_POST_PARENT_ACCOUNT, asset(balance, SP_SYMBOL), start_time, deadline, ""));

    BOOST_CHECK_EQUAL(fund_budget_service_fixture.object.owner, SCORUM_ROOT_POST_PARENT_ACCOUNT);
    BOOST_CHECK_EQUAL(fund_budget_service_fixture.object.permlink, "");
    BOOST_CHECK_EQUAL(fund_budget_service_fixture.object.created.sec_since_epoch(), head_block_time.sec_since_epoch());
    BOOST_CHECK_EQUAL(fund_budget_service_fixture.object.start.sec_since_epoch(), start_time.sec_since_epoch());
    BOOST_CHECK_EQUAL(fund_budget_service_fixture.object.deadline.sec_since_epoch(), deadline.sec_since_epoch());
    BOOST_CHECK_EQUAL(fund_budget_service_fixture.object.balance, asset(balance, SP_SYMBOL));
    BOOST_CHECK_GT(fund_budget_service_fixture.object.per_block, asset(0, SP_SYMBOL));
    BOOST_CHECK_LT(fund_budget_service_fixture.object.per_block, asset(balance, SP_SYMBOL));
}

struct account_budget_fixture : public budget_fixture
{
    Actor alice;

    service_base_wrapper<post_budget_service_i> post_budget_service_fixture;
    service_base_wrapper<banner_budget_service_i> banner_budget_service_fixture;
    account_service_wrapper account_service_fixture;

    account_budget_fixture()
        : alice("alice")
        , post_budget_service_fixture(*this, mocks)
        , banner_budget_service_fixture(*this, mocks)
        , account_service_fixture(*this, mocks)
    {
        alice.scorum(asset(balance * 10, SCORUM_SYMBOL));
        account_service_fixture.add_actor(alice);
    }

    template <typename BudgetServiceFixture> void test_budget_creation(BudgetServiceFixture& budget_service_fixture)
    {
        owned_base_budget_management_algorithm<typename BudgetServiceFixture::interface_type> creator(
            budget_service_fixture.service(), dgp_service_fixture.service(), account_service_fixture.service());

        const char* permlink = "adidas";

        SCORUM_REQUIRE_THROW(
            creator.create_budget(alice.name, asset(balance, SP_SYMBOL), start_time, deadline, permlink),
            fc::assert_exception);

        SCORUM_REQUIRE_THROW(
            creator.create_budget(alice.name, asset(balance, SCORUM_SYMBOL), deadline, start_time, permlink),
            fc::assert_exception);

        SCORUM_REQUIRE_THROW(creator.create_budget(alice.name, alice.scr_amount * 2, deadline, start_time, permlink),
                             fc::assert_exception);

        BOOST_REQUIRE_NO_THROW(
            creator.create_budget(alice.name, asset(balance, SCORUM_SYMBOL), start_time, deadline, permlink));

        BOOST_CHECK_EQUAL(budget_service_fixture.object.owner, alice.name);
        BOOST_CHECK_EQUAL(budget_service_fixture.object.permlink, permlink);
        BOOST_CHECK_EQUAL(budget_service_fixture.object.created.sec_since_epoch(), head_block_time.sec_since_epoch());
        BOOST_CHECK_EQUAL(budget_service_fixture.object.start.sec_since_epoch(), start_time.sec_since_epoch());
        BOOST_CHECK_EQUAL(budget_service_fixture.object.deadline.sec_since_epoch(), deadline.sec_since_epoch());
        BOOST_CHECK_EQUAL(budget_service_fixture.object.balance, asset(balance, SCORUM_SYMBOL));
        BOOST_CHECK_GT(budget_service_fixture.object.per_block, asset(0, SCORUM_SYMBOL));
        BOOST_CHECK_LT(budget_service_fixture.object.per_block, asset(balance, SCORUM_SYMBOL));

        BOOST_CHECK_EQUAL(account_service_fixture.service().get_account(alice.name).balance,
                          alice.scr_amount - asset(balance, SCORUM_SYMBOL));
    }

    template <typename BudgetServiceFixture> void test_close_budget(BudgetServiceFixture& budget_service_fixture)
    {
        owned_base_budget_management_algorithm<typename BudgetServiceFixture::interface_type> manager(
            budget_service_fixture.service(), dgp_service_fixture.service(), account_service_fixture.service());

        BOOST_REQUIRE_NO_THROW(
            manager.create_budget(alice.name, asset(balance, SCORUM_SYMBOL), start_time, deadline, "pepsi"));

        BOOST_CHECK_EQUAL(account_service_fixture.service().get_account(alice.name).balance,
                          alice.scr_amount - asset(balance, SCORUM_SYMBOL));

        BOOST_REQUIRE_NO_THROW(manager.close_budget(budget_service_fixture.object));

        BOOST_CHECK_EQUAL(account_service_fixture.service().get_account(alice.name).balance, alice.scr_amount);
    }
};

BOOST_FIXTURE_TEST_CASE(post_budget_creation, account_budget_fixture)
{
    test_budget_creation(post_budget_service_fixture);
}

BOOST_FIXTURE_TEST_CASE(banner_budget_creation, account_budget_fixture)
{
    test_budget_creation(banner_budget_service_fixture);
}

BOOST_FIXTURE_TEST_CASE(post_budget_close, account_budget_fixture)
{
    test_close_budget(post_budget_service_fixture);
}

BOOST_FIXTURE_TEST_CASE(banner_budget_close, account_budget_fixture)
{
    test_close_budget(banner_budget_service_fixture);
}
}
