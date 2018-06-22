#include <boost/test/unit_test.hpp>

#include <scorum/chain/services/budgets.hpp>

#include <scorum/chain/schema/budget_objects.hpp>

#include <scorum/common_api/config.hpp>

#include "budget_check_common.hpp"

#include <limits>

#include "actor.hpp"

namespace budget_service_tests {

using namespace budget_check_common;

struct budget_service_getters_check_fixture : public budget_check_fixture
{
    budget_service_getters_check_fixture()
        : alice("alice")
        , bob("bob")
    {
        actor(initdelegate).create_account(alice);
        actor(initdelegate).give_scr(alice, BUDGET_BALANCE_DEFAULT * 100);
        actor(initdelegate).create_account(bob);
        actor(initdelegate).give_scr(bob, BUDGET_BALANCE_DEFAULT * 200);
    }

    Actor alice;
    Actor bob;
};

BOOST_FIXTURE_TEST_SUITE(budget_service_getters_check, budget_service_getters_check_fixture)

SCORUM_TEST_CASE(get_budget_check)
{
    create_budget(alice, budget_type::post);
    create_budget(bob, budget_type::banner);

    BOOST_REQUIRE_EQUAL(post_budget_service.get(post_budget_object::id_type(0)).owner, alice.name);
    BOOST_REQUIRE_EQUAL(banner_budget_service.get(banner_budget_object::id_type(0)).owner, bob.name);
}

SCORUM_TEST_CASE(get_all_budgets_check)
{
    create_budget(alice, budget_type::post);
    create_budget(alice, budget_type::post);
    create_budget(alice, budget_type::banner);
    create_budget(bob, budget_type::post);
    create_budget(bob, budget_type::banner);

    BOOST_REQUIRE_EQUAL(post_budget_service.get_budgets().size(), 3u);
    BOOST_REQUIRE_EQUAL(banner_budget_service.get_budgets().size(), 2u);
}

SCORUM_TEST_CASE(get_owned_budgets_check)
{
    create_budget(alice, budget_type::post);
    create_budget(bob, budget_type::post);
    create_budget(bob, budget_type::banner);

    BOOST_REQUIRE_EQUAL(post_budget_service.get_budgets(alice.name).size(), 1u);
    BOOST_REQUIRE_EQUAL(post_budget_service.get_budgets(bob.name).size(), 1u);
    BOOST_REQUIRE_EQUAL(banner_budget_service.get_budgets(bob.name).size(), 1u);
}

BOOST_AUTO_TEST_SUITE_END()

struct budget_service_lookup_check_fixture : public budget_service_getters_check_fixture
{
    budget_service_lookup_check_fixture()
    {
        create_budget(alice, budget_type::post);
        create_budget(bob, budget_type::post);
        create_budget(bob, budget_type::banner);
    }
};

BOOST_FIXTURE_TEST_SUITE(budget_service_lookup_check, budget_service_lookup_check_fixture)

SCORUM_TEST_CASE(lookup_post_budget_owners_check)
{
    {
        auto owners = post_budget_service.lookup_budget_owners(SCORUM_ROOT_POST_PARENT_ACCOUNT, 1u);
        BOOST_REQUIRE_EQUAL(owners.size(), 1u);
        BOOST_CHECK_EQUAL(*owners.begin(), alice.name);
    }

    {
        auto owners = post_budget_service.lookup_budget_owners(SCORUM_ROOT_POST_PARENT_ACCOUNT, 2u);
        BOOST_REQUIRE_EQUAL(owners.size(), 2u);
        BOOST_CHECK_EQUAL(*owners.begin(), alice.name);
    }

    {
        auto owners = post_budget_service.lookup_budget_owners(SCORUM_ROOT_POST_PARENT_ACCOUNT, 1u);
        BOOST_REQUIRE_EQUAL(owners.size(), 1u);
        BOOST_REQUIRE_EQUAL(*owners.begin(), alice.name);
        owners = post_budget_service.lookup_budget_owners(bob.name, 1u);
        BOOST_REQUIRE_EQUAL(owners.size(), 1u);
        BOOST_CHECK_EQUAL(*owners.begin(), bob.name);
    }
}

SCORUM_TEST_CASE(lookup_banner_budget_owners_check)
{
    auto owners = banner_budget_service.lookup_budget_owners(SCORUM_ROOT_POST_PARENT_ACCOUNT, 100u);
    BOOST_REQUIRE_EQUAL(owners.size(), 1u);
    BOOST_CHECK_EQUAL(*owners.begin(), bob.name);
}

BOOST_AUTO_TEST_SUITE_END()
}
