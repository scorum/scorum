#include <boost/test/unit_test.hpp>

#include <scorum/chain/services/budgets.hpp>
#include <scorum/chain/schema/budget_objects.hpp>
#include <scorum/common_api/config_api.hpp>
#include "budget_check_common.hpp"

#include <boost/uuid/uuid_generators.hpp>
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

    auto calc_total_owner_pending_income()
    {
        auto budgets = post_budget_service.get_budgets();

        asset r;

        for (auto budget : budgets)
        {
            r += budget.get().owner_pending_income;
        }

        return r;
    }

    auto calc_total_volume()
    {
        auto budgets = post_budget_service.get_budgets();

        asset r;

        for (auto budget : budgets)
        {
            r += budget.get().balance;
        }

        return r;
    }

    auto calc_total_budget_pending_outgo()
    {
        auto budgets = post_budget_service.get_budgets();

        asset r;

        for (auto budget : budgets)
        {
            r += budget.get().budget_pending_outgo;
        }

        return r;
    }

    Actor alice;
    Actor bob;
    Actor sam;

    boost::uuids::uuid ns_uuid = boost::uuids::string_generator()("00000000-0000-0000-0000-000000000001");
    boost::uuids::name_generator uuid_gen = boost::uuids::name_generator(ns_uuid);
};

BOOST_FIXTURE_TEST_SUITE(budget_service_check, budget_service_getters_check_fixture)

SCORUM_TEST_CASE(check_that_after_budget_closed_all_stat_is_zero)
{
    create_budget(uuid_gen("alice"), alice, budget_type::post);

    this->generate_blocks(BUDGET_BALANCE_DEFAULT / BUDGET_PERBLOCK_DEFAULT);

    auto dgp = this->db.get<dynamic_global_property_object>();

    BOOST_CHECK_EQUAL(ASSET_SCR(0), dgp.advertising.post_budgets.volume);
    BOOST_CHECK_EQUAL(ASSET_SCR(0), dgp.advertising.post_budgets.budget_pending_outgo);
    BOOST_CHECK_EQUAL(ASSET_SCR(0), dgp.advertising.post_budgets.owner_pending_income);
}

SCORUM_TEST_CASE(validate_advertising_total_stats)
{
    create_budget(uuid_gen("alice0"), alice, budget_type::post, 50, 1, BUDGET_DEADLINE_IN_BLOCKS_DEFAULT);
    create_budget(uuid_gen("alice1"), alice, budget_type::post, 40, 1, BUDGET_DEADLINE_IN_BLOCKS_DEFAULT);
    create_budget(uuid_gen("alice2"), alice, budget_type::post, 30, 1, BUDGET_DEADLINE_IN_BLOCKS_DEFAULT);
    create_budget(uuid_gen("alice3"), alice, budget_type::post, 20, 1, BUDGET_DEADLINE_IN_BLOCKS_DEFAULT);
    create_budget(uuid_gen("alice4"), alice, budget_type::post, 10, 1, BUDGET_DEADLINE_IN_BLOCKS_DEFAULT);

    // clang-format off
    const std::vector<std::vector<asset>> expectation = {
        { ASSET_SCR(120), ASSET_SCR(9),  ASSET_SCR(21) },
        { ASSET_SCR(90),  ASSET_SCR(18), ASSET_SCR(42) },
        { ASSET_SCR(60),  ASSET_SCR(27), ASSET_SCR(63) },
        { ASSET_SCR(30),  ASSET_SCR(36), ASSET_SCR(84) },
        { ASSET_SCR(0),   ASSET_SCR(0),  ASSET_SCR(0)  } };
    // clang-format on

    for (uint32_t i = 0; i < BUDGET_DEADLINE_IN_BLOCKS_DEFAULT; ++i)
    {
        this->generate_block();

        auto dgp = this->db.get<dynamic_global_property_object>();

        BOOST_CHECK_EQUAL(calc_total_volume(), dgp.advertising.post_budgets.volume);
        BOOST_CHECK_EQUAL(calc_total_budget_pending_outgo(), dgp.advertising.post_budgets.budget_pending_outgo);
        BOOST_CHECK_EQUAL(calc_total_owner_pending_income(), dgp.advertising.post_budgets.owner_pending_income);

        BOOST_CHECK_EQUAL(expectation[i][0], dgp.advertising.post_budgets.volume);
        BOOST_CHECK_EQUAL(expectation[i][1], dgp.advertising.post_budgets.budget_pending_outgo);
        BOOST_CHECK_EQUAL(expectation[i][2], dgp.advertising.post_budgets.owner_pending_income);
    }
}

SCORUM_TEST_CASE(get_budget_check)
{
    auto alice_uuid = uuid_gen("alice");
    auto bob_uuid = uuid_gen("bob");
    create_budget(alice_uuid, alice, budget_type::post);
    create_budget(bob_uuid, bob, budget_type::banner);

    BOOST_REQUIRE_EQUAL(post_budget_service.get(alice_uuid).owner, alice.name);
    BOOST_REQUIRE_EQUAL(banner_budget_service.get(bob_uuid).owner, bob.name);
}

SCORUM_TEST_CASE(get_all_budgets_check)
{
    create_budget(uuid_gen("alice0"), alice, budget_type::post);
    create_budget(uuid_gen("alice1"), alice, budget_type::post);
    create_budget(uuid_gen("alice2"), alice, budget_type::banner);
    create_budget(uuid_gen("bob0"), bob, budget_type::post);
    create_budget(uuid_gen("bob1"), bob, budget_type::banner);

    BOOST_REQUIRE_EQUAL(post_budget_service.get_budgets().size(), 3u);
    BOOST_REQUIRE_EQUAL(banner_budget_service.get_budgets().size(), 2u);
}

SCORUM_TEST_CASE(get_owned_budgets_check)
{
    create_budget(uuid_gen("alice"), alice, budget_type::post);
    create_budget(uuid_gen("bob0"), bob, budget_type::post);
    create_budget(uuid_gen("bob1"), bob, budget_type::banner);

    BOOST_REQUIRE_EQUAL(post_budget_service.get_budgets(alice.name).size(), 1u);
    BOOST_REQUIRE_EQUAL(post_budget_service.get_budgets(bob.name).size(), 1u);
    BOOST_REQUIRE_EQUAL(banner_budget_service.get_budgets(bob.name).size(), 1u);
}

BOOST_AUTO_TEST_SUITE_END()

struct budget_service_lookup_check_fixture : public budget_service_getters_check_fixture
{
    budget_service_lookup_check_fixture()
    {
        create_budget(uuid_gen("alice"), alice, budget_type::post);
        create_budget(uuid_gen("bob0"), bob, budget_type::post);
        create_budget(uuid_gen("bob1"), bob, budget_type::banner);
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
        create_budget(uuid_gen("alice"), alice, type);
        create_budget(uuid_gen("bob"), bob, type);

        BOOST_REQUIRE_EQUAL(service.get_budgets().size(), 2u);

        BOOST_REQUIRE_EQUAL(service.get_top_budgets(db.head_block_time(), 1u).size(), 1u);
        BOOST_REQUIRE_EQUAL(service.get_top_budgets(db.head_block_time(), 2u).size(), 2u);

        BOOST_REQUIRE_EQUAL(service.get_top_budgets(db.head_block_time()).size(), 2u);
    }

    template <typename ServiceType> void test_start_time_check(ServiceType& service, budget_type type)
    {
        const int start_in_blocks = 2;

        create_budget(uuid_gen("alice0"), alice, type, BUDGET_BALANCE_DEFAULT, start_in_blocks,
                      BUDGET_DEADLINE_IN_BLOCKS_DEFAULT + start_in_blocks);
        create_budget(uuid_gen("alice1"), alice, type, BUDGET_BALANCE_DEFAULT, start_in_blocks * 2,
                      BUDGET_DEADLINE_IN_BLOCKS_DEFAULT + start_in_blocks * 2);
        create_budget(uuid_gen("bob"), bob, type, BUDGET_BALANCE_DEFAULT, start_in_blocks,
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
    create_budget(uuid_gen("alice0"), alice, budget_type::post, BUDGET_BALANCE_DEFAULT,
                  BUDGET_DEADLINE_IN_BLOCKS_DEFAULT);
    create_budget(uuid_gen("bob"), bob, budget_type::post, BUDGET_BALANCE_DEFAULT * 2,
                  BUDGET_DEADLINE_IN_BLOCKS_DEFAULT);
    create_budget(uuid_gen("alice1"), alice, budget_type::post, BUDGET_BALANCE_DEFAULT * 3,
                  BUDGET_DEADLINE_IN_BLOCKS_DEFAULT);
    create_budget(uuid_gen("sam"), sam, budget_type::post, BUDGET_BALANCE_DEFAULT * 4,
                  BUDGET_DEADLINE_IN_BLOCKS_DEFAULT);

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
