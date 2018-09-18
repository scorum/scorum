#include <boost/test/unit_test.hpp>

#include <scorum/chain/services/budgets.hpp>

#include <scorum/chain/schema/budget_objects.hpp>

#include <scorum/common_api/config_api.hpp>

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
        , sam("sam")
    {
        actor(initdelegate).create_account(alice);
        actor(initdelegate).give_scr(alice, BUDGET_BALANCE_DEFAULT * 100);
        actor(initdelegate).create_account(bob);
        actor(initdelegate).give_scr(bob, BUDGET_BALANCE_DEFAULT * 200);
        actor(initdelegate).create_account(sam);
        actor(initdelegate).give_scr(sam, BUDGET_BALANCE_DEFAULT * 300);
    }

    Actor alice;
    Actor bob;
    Actor sam;
};

BOOST_FIXTURE_TEST_SUITE(budget_service_check, budget_service_getters_check_fixture)

SCORUM_TEST_CASE(get_budget_check)
{
    create_budget(alice, budget_type::post);
    create_budget(bob, budget_type::banner);

    BOOST_REQUIRE_EQUAL(post_budget_service.get(post_budget_object::id_type(0)).owner, alice.name);
    BOOST_REQUIRE_EQUAL(banner_budget_service.get(banner_budget_object::id_type(0)).owner, bob.name);
}

SCORUM_TEST_CASE(get_budgets_json_isnt_empty_check)
{
    create_budget(alice, budget_type::post);
    create_budget(bob, budget_type::banner);

    BOOST_REQUIRE_EQUAL(post_budget_service.get_budgets().size(), 1u);
    BOOST_CHECK(!post_budget_service.get_budgets()[0].get().json_metadata.empty());
    BOOST_REQUIRE_EQUAL(banner_budget_service.get_budgets().size(), 1u);
    BOOST_CHECK(!banner_budget_service.get_budgets()[0].get().json_metadata.empty());
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

struct get_top_budgets_check_fixture : public budget_service_getters_check_fixture
{
    template <typename ServiceType> void test_limited_check(ServiceType& service, budget_type type)
    {
        create_budget(alice, type);
        create_budget(bob, type);

        BOOST_REQUIRE_EQUAL(service.get_budgets().size(), 2u);

        BOOST_REQUIRE_EQUAL(service.get_top_budgets(db.head_block_time(), 1u).size(), 1u);
        BOOST_REQUIRE_EQUAL(service.get_top_budgets(db.head_block_time(), 2u).size(), 2u);

        BOOST_REQUIRE_EQUAL(service.get_top_budgets(db.head_block_time()).size(), 2u);
    }

    template <typename ServiceType> void test_start_time_check(ServiceType& service, budget_type type)
    {
        const int start_in_blocks = 2;

        create_budget(alice, type, BUDGET_BALANCE_DEFAULT, start_in_blocks,
                      BUDGET_DEADLINE_IN_BLOCKS_DEFAULT + start_in_blocks);
        create_budget(alice, type, BUDGET_BALANCE_DEFAULT, start_in_blocks * 2,
                      BUDGET_DEADLINE_IN_BLOCKS_DEFAULT + start_in_blocks * 2);
        create_budget(bob, type, BUDGET_BALANCE_DEFAULT, start_in_blocks,
                      BUDGET_DEADLINE_IN_BLOCKS_DEFAULT + start_in_blocks);

        BOOST_REQUIRE_EQUAL(service.get_budgets().size(), 3u);

        BOOST_REQUIRE_EQUAL(service.get_top_budgets(db.get_slot_time(start_in_blocks), 3u).size(), 2u);
        BOOST_REQUIRE_EQUAL(service.get_top_budgets(db.get_slot_time(start_in_blocks * 2), 3u).size(), 3u);

        BOOST_REQUIRE_EQUAL(service.get_top_budgets(db.get_slot_time(start_in_blocks)).size(), 2u);
        BOOST_REQUIRE_EQUAL(service.get_top_budgets(db.get_slot_time(start_in_blocks * 2)).size(), 3u);
    }
};

BOOST_FIXTURE_TEST_SUITE(get_top_budgets_check, get_top_budgets_check_fixture)

SCORUM_TEST_CASE(limited_check)
{
    test_limited_check(post_budget_service, budget_type::post);
    test_limited_check(banner_budget_service, budget_type::banner);
}

SCORUM_TEST_CASE(start_time_check)
{
    test_start_time_check(post_budget_service, budget_type::post);
    test_start_time_check(banner_budget_service, budget_type::banner);
}

SCORUM_TEST_CASE(ordered_by_per_block_check)
{
    create_budget(alice, budget_type::post, BUDGET_BALANCE_DEFAULT, BUDGET_DEADLINE_IN_BLOCKS_DEFAULT);
    create_budget(bob, budget_type::post, BUDGET_BALANCE_DEFAULT * 2, BUDGET_DEADLINE_IN_BLOCKS_DEFAULT);
    create_budget(alice, budget_type::post, BUDGET_BALANCE_DEFAULT * 3, BUDGET_DEADLINE_IN_BLOCKS_DEFAULT);
    create_budget(sam, budget_type::post, BUDGET_BALANCE_DEFAULT * 4, BUDGET_DEADLINE_IN_BLOCKS_DEFAULT);

    BOOST_REQUIRE_EQUAL(post_budget_service.get_budgets().size(), 4u);

    using actors_type = std::vector<account_name_type>;

    actors_type expected_actors{ sam.name, alice.name, bob.name };

    BOOST_REQUIRE_EQUAL(post_budget_service.get_top_budgets(db.head_block_time(), expected_actors.size()).size(),
                        expected_actors.size());
    size_t ci = 0;
    for (const post_budget_object& budget : post_budget_service.get_top_budgets(db.head_block_time(), 3u))
    {
        BOOST_CHECK_EQUAL(budget.owner, expected_actors[ci++]);
    }
}

BOOST_AUTO_TEST_SUITE_END()
}
