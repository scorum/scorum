#include <boost/test/unit_test.hpp>

#include <scorum/app/api_context.hpp>
#include <scorum/app/detail/advertising_api.hpp>
#include <scorum/chain/database/database.hpp>

#include <scorum/chain/services/budgets.hpp>
#include <scorum/chain/services/account.hpp>
#include <scorum/chain/services/dynamic_global_property.hpp>
#include <scorum/chain/services/advertising_property.hpp>

#include <defines.hpp>
#include <hippomocks.h>
#include "object_wrapper.hpp"

using namespace scorum::chain;
using namespace scorum::protocol;
using namespace scorum::app;

namespace {
struct advertising_api_fixture : public shared_memory_fixture
{
    advertising_api_fixture()
        : dummy_db(0)
    {
        mocks.OnCall(services, data_service_factory_i::advertising_property_service).ReturnByRef(*adv_service);
        mocks.OnCall(services, data_service_factory_i::account_service).ReturnByRef(*account_service);
        mocks.OnCall(services, data_service_factory_i::banner_budget_service).ReturnByRef(*banner_budget_service);
        mocks.OnCall(services, data_service_factory_i::post_budget_service).ReturnByRef(*post_budget_service);
        mocks.OnCall(services, data_service_factory_i::dynamic_global_property_service).ReturnByRef(*dyn_props_service);

        api = std::make_unique<advertising_api::impl>(dummy_db, *services);
    }

    MockRepository mocks;
    data_service_factory_i* services = mocks.Mock<data_service_factory_i>();

    account_service_i* account_service = mocks.Mock<account_service_i>();
    post_budget_service_i* post_budget_service = mocks.Mock<post_budget_service_i>();
    banner_budget_service_i* banner_budget_service = mocks.Mock<banner_budget_service_i>();
    dynamic_global_property_service_i* dyn_props_service = mocks.Mock<dynamic_global_property_service_i>();
    advertising_property_service_i* adv_service = mocks.Mock<advertising_property_service_i>();

    database dummy_db;
    std::unique_ptr<advertising_api::impl> api;

    post_budget_object create_post_budget(const std::string& owner, int64_t id)
    {
        auto obj = create_object<post_budget_object>(shm);
        obj.id = id;
        obj.owner = owner;
        obj.created = fc::time_point_sec((uint32_t)id);
        fc::from_string(obj.json_metadata, "post" + std::to_string(id));

        return obj;
    }
    banner_budget_object create_banner_budget(const std::string& owner, int64_t id)
    {
        auto obj = create_object<banner_budget_object>(shm);
        obj.id = id;
        obj.owner = owner;
        obj.created = fc::time_point_sec((uint32_t)id);
        fc::from_string(obj.json_metadata, "banner" + std::to_string(id));

        return obj;
    }
};

BOOST_FIXTURE_TEST_SUITE(advertising_api_tests, advertising_api_fixture)

SCORUM_TEST_CASE(check_get_post_budget_positive)
{
    auto id = 0;
    auto p = create_post_budget("alice", id);
    mocks.OnCall(post_budget_service, post_budget_service_i::find).With(id).ReturnByRef(&p);

    auto budget = api->get_budget(id, budget_type::post);
    BOOST_REQUIRE(budget.valid());
    BOOST_REQUIRE_EQUAL(budget->id, id);
}

SCORUM_TEST_CASE(check_get_banner_budget_positive)
{
    auto id = 0;
    auto b = create_banner_budget("alice", id);
    mocks.OnCall(banner_budget_service, banner_budget_service_i::find).With(id).ReturnByRef(&b);

    auto budget = api->get_budget(id, budget_type::banner);
    BOOST_REQUIRE(budget.valid());
    BOOST_REQUIRE_EQUAL(budget->id, id);
}

SCORUM_TEST_CASE(check_get_post_budget_negative)
{
    mocks.OnCall(post_budget_service, post_budget_service_i::find).ReturnByRef(nullptr);

    auto budget = api->get_budget(0, budget_type::post);
    BOOST_REQUIRE(!budget.valid());
}

SCORUM_TEST_CASE(check_get_banner_budget_negative)
{
    mocks.OnCall(banner_budget_service, banner_budget_service_i::find).ReturnByRef(nullptr);

    auto budget = api->get_budget(0, budget_type::banner);
    BOOST_REQUIRE(!budget.valid());
}

SCORUM_TEST_CASE(check_get_user_budgets_ordered)
{
    auto p1 = create_post_budget("alice", 1);
    auto b1 = create_banner_budget("alice", 2);
    auto p2 = create_post_budget("alice", 3);
    auto b2 = create_banner_budget("alice", 4);
    post_budget_service_i::budgets_type alice_posts = { std::cref(p1), std::cref(p2) };
    banner_budget_service_i::budgets_type alice_banners = { std::cref(b1), std::cref(b2) };
    post_budget_service_i::budgets_type bob_posts;
    banner_budget_service_i::budgets_type bob_banners;

    mocks
        .ExpectCallOverload(
            post_budget_service,
            (post_budget_service_i::budgets_type(post_budget_service_i::*)(const account_name_type&) const)
                & post_budget_service_i::get_budgets)
        .With("alice")
        .ReturnByRef(alice_posts);

    mocks
        .ExpectCallOverload(
            banner_budget_service,
            (banner_budget_service_i::budgets_type(banner_budget_service_i::*)(const account_name_type&) const)
                & banner_budget_service_i::get_budgets)
        .With("alice")
        .ReturnByRef(alice_banners);

    auto alice_budgets = api->get_user_budgets("alice");

    BOOST_REQUIRE_EQUAL(alice_budgets.size(), 4u);
    BOOST_CHECK(
        (std::is_sorted(alice_budgets.begin(), alice_budgets.end(),
                        [](const budget_api_obj& l, const budget_api_obj& r) { return l.created > r.created; })));
}

SCORUM_TEST_CASE(check_get_user_budgets_empty)
{
    post_budget_service_i::budgets_type alice_posts;
    banner_budget_service_i::budgets_type alice_banners;

    mocks
        .ExpectCallOverload(
            post_budget_service,
            (post_budget_service_i::budgets_type(post_budget_service_i::*)(const account_name_type&) const)
                & post_budget_service_i::get_budgets)
        .With("alice")
        .ReturnByRef(alice_posts);

    mocks
        .ExpectCallOverload(
            banner_budget_service,
            (banner_budget_service_i::budgets_type(banner_budget_service_i::*)(const account_name_type&) const)
                & banner_budget_service_i::get_budgets)
        .With("alice")
        .ReturnByRef(alice_banners);

    auto bob_budgets = api->get_user_budgets("alice");

    BOOST_REQUIRE_EQUAL(bob_budgets.size(), 0u);
}

BOOST_AUTO_TEST_SUITE_END()
}
