#include <boost/test/unit_test.hpp>

#include "common_fixtures.hpp"

namespace management_algorithms_tests {

using namespace common_fixtures;

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
        creator.create_budget(SCORUM_ROOT_POST_PARENT_ACCOUNT, asset(balance, SCORUM_SYMBOL), start, deadline, ""),
        fc::assert_exception);

    SCORUM_REQUIRE_THROW(
        creator.create_budget(SCORUM_ROOT_POST_PARENT_ACCOUNT, asset(balance, SP_SYMBOL), deadline, start, ""),
        fc::assert_exception);

    BOOST_REQUIRE_NO_THROW(
        creator.create_budget(SCORUM_ROOT_POST_PARENT_ACCOUNT, asset(balance, SP_SYMBOL), start, deadline, ""));

    BOOST_CHECK_EQUAL(fund_budget_service_fixture.get().owner, SCORUM_ROOT_POST_PARENT_ACCOUNT);
    BOOST_CHECK_EQUAL(fund_budget_service_fixture.get().content_permlink, "");
    BOOST_CHECK_EQUAL(fund_budget_service_fixture.get().created.sec_since_epoch(), head_block_time.sec_since_epoch());
    BOOST_CHECK_EQUAL(fund_budget_service_fixture.get().start.sec_since_epoch(), start.sec_since_epoch());
    BOOST_CHECK_EQUAL(fund_budget_service_fixture.get().deadline.sec_since_epoch(), deadline.sec_since_epoch());
    BOOST_CHECK_EQUAL(fund_budget_service_fixture.get().balance, asset(balance, SP_SYMBOL));
    BOOST_CHECK_GT(fund_budget_service_fixture.get().per_block, asset(0, SP_SYMBOL));
    BOOST_CHECK_LT(fund_budget_service_fixture.get().per_block, asset(balance, SP_SYMBOL));
}

struct test_account_budget_fixture : public account_budget_fixture
{
    template <typename BudgetServiceFixture> void test_budget_creation(BudgetServiceFixture& budget_service_fixture)
    {
        owned_base_budget_management_algorithm<typename BudgetServiceFixture::interface_type> creator(
            budget_service_fixture.service(), dgp_service_fixture.service(), account_service_fixture.service());

        const char* permlink = "adidas";

        SCORUM_REQUIRE_THROW(creator.create_budget(alice.name, asset(balance, SP_SYMBOL), start, deadline, permlink),
                             fc::assert_exception);

        SCORUM_REQUIRE_THROW(
            creator.create_budget(alice.name, asset(balance, SCORUM_SYMBOL), deadline, start, permlink),
            fc::assert_exception);

        SCORUM_REQUIRE_THROW(creator.create_budget(alice.name, alice.scr_amount * 2, deadline, start, permlink),
                             fc::assert_exception);

        BOOST_REQUIRE_NO_THROW(
            creator.create_budget(alice.name, asset(balance, SCORUM_SYMBOL), start, deadline, permlink));

        BOOST_CHECK_EQUAL(budget_service_fixture.get().owner, alice.name);
        BOOST_CHECK_EQUAL(budget_service_fixture.get().content_permlink, permlink);
        BOOST_CHECK_EQUAL(budget_service_fixture.get().created.sec_since_epoch(), head_block_time.sec_since_epoch());
        BOOST_CHECK_EQUAL(budget_service_fixture.get().start.sec_since_epoch(), start.sec_since_epoch());
        BOOST_CHECK_EQUAL(budget_service_fixture.get().deadline.sec_since_epoch(), deadline.sec_since_epoch());
        BOOST_CHECK_EQUAL(budget_service_fixture.get().balance, asset(balance, SCORUM_SYMBOL));
        BOOST_CHECK_GT(budget_service_fixture.get().per_block, asset(0, SCORUM_SYMBOL));
        BOOST_CHECK_LT(budget_service_fixture.get().per_block, asset(balance, SCORUM_SYMBOL));

        BOOST_CHECK_EQUAL(account_service_fixture.service().get_account(alice.name).balance,
                          alice.scr_amount - asset(balance, SCORUM_SYMBOL));
    }

    template <typename BudgetServiceFixture> void test_close_budget(BudgetServiceFixture& budget_service_fixture)
    {
        owned_base_budget_management_algorithm<typename BudgetServiceFixture::interface_type> manager(
            budget_service_fixture.service(), dgp_service_fixture.service(), account_service_fixture.service());

        BOOST_REQUIRE_NO_THROW(
            manager.create_budget(alice.name, asset(balance, SCORUM_SYMBOL), start, deadline, "pepsi"));

        BOOST_CHECK_EQUAL(account_service_fixture.service().get_account(alice.name).balance,
                          alice.scr_amount - asset(balance, SCORUM_SYMBOL));

        BOOST_REQUIRE_NO_THROW(manager.close_budget(budget_service_fixture.get()));

        BOOST_CHECK_EQUAL(account_service_fixture.service().get_account(alice.name).balance, alice.scr_amount);
    }

    template <typename BudgetServiceFixture> void test_allocate_budget(BudgetServiceFixture& budget_service_fixture)
    {
        owned_base_budget_management_algorithm<typename BudgetServiceFixture::interface_type> manager(
            budget_service_fixture.service(), dgp_service_fixture.service(), account_service_fixture.service());

        BOOST_REQUIRE_NO_THROW(
            manager.create_budget(alice.name, asset(balance, SCORUM_SYMBOL), start, deadline, "pepsi"));

        dgp_service_fixture.update([&](dynamic_global_property_object& obj) {
            obj.head_block_number = budget_service_fixture.get().last_cashout_block;
        });

        auto cash = manager.allocate_cash(budget_service_fixture.get());

        BOOST_CHECK_EQUAL(cash, ASSET_SCR(0));

        dgp_service_fixture.update([&](dynamic_global_property_object& obj) { obj.head_block_number++; });

        cash = manager.allocate_cash(budget_service_fixture.get());

        BOOST_CHECK_EQUAL(cash, budget_service_fixture.get().per_block);
    }
};

BOOST_FIXTURE_TEST_CASE(post_budget_creation, test_account_budget_fixture)
{
    test_budget_creation(post_budget_service_fixture);
}

BOOST_FIXTURE_TEST_CASE(banner_budget_creation, test_account_budget_fixture)
{
    test_budget_creation(banner_budget_service_fixture);
}

BOOST_FIXTURE_TEST_CASE(post_budget_close, test_account_budget_fixture)
{
    test_close_budget(post_budget_service_fixture);
}

BOOST_FIXTURE_TEST_CASE(banner_budget_close, test_account_budget_fixture)
{
    test_close_budget(banner_budget_service_fixture);
}

BOOST_FIXTURE_TEST_CASE(allocate_post_cash_per_block, test_account_budget_fixture)
{
    test_allocate_budget(post_budget_service_fixture);
}

BOOST_FIXTURE_TEST_CASE(allocate_banner_cash_per_block, test_account_budget_fixture)
{
    test_allocate_budget(banner_budget_service_fixture);
}
}
