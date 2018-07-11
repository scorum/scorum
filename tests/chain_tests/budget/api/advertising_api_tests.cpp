#include <boost/test/unit_test.hpp>

#include <scorum/app/api_context.hpp>
#include <scorum/app/advertising_api.hpp>

#include "budget/budget_check_common.hpp"

using namespace scorum::app;

namespace std {

std::ostream& operator<<(std::ostream& os, budget_type t)
{
    os << fc::reflector<budget_type>::to_string(t);
    return os;
}

std::ostringstream& operator<<(std::ostringstream& os, budget_type t)
{
    static_cast<std::ostream&>(os) << t;
    return os;
}
}

namespace adv_api_tests {

struct advertising_api_fixture : public budget_check_common::budget_check_fixture
{
    advertising_api_fixture()
        : _api_ctx(app, ADVERTISING_API_NAME, std::make_shared<api_session_data>())
        , _api(_api_ctx)
    {
        actor(initdelegate).create_account(alice);
        actor(initdelegate).create_account(sam);

        actor(initdelegate).give_sp(alice, 1e9);
        actor(initdelegate).give_scr(alice, BUDGET_BALANCE_DEFAULT * 100);
        actor(initdelegate).give_sp(sam, 1e9);
        actor(initdelegate).give_scr(sam, BUDGET_BALANCE_DEFAULT * 100);
    }

    api_context _api_ctx;
    advertising_api _api;

    Actor alice = "alice";
    Actor sam = "sam";
};

BOOST_FIXTURE_TEST_SUITE(advertising_api_tests, advertising_api_fixture)

SCORUM_TEST_CASE(check_get_moderator)
{
    BOOST_REQUIRE(!_api.get_moderator().valid());

    development_committee_empower_advertising_moderator_operation empower_moder_op;
    empower_moder_op.account = alice.name;

    proposal_create_operation prop_create_op;
    prop_create_op.creator = initdelegate.name;
    prop_create_op.operation = empower_moder_op;
    prop_create_op.lifetime_sec = SCORUM_PROPOSAL_LIFETIME_MIN_SECONDS;

    push_operation(prop_create_op, initdelegate.private_key);

    proposal_vote_operation prop_vote_op;
    prop_vote_op.proposal_id = 0;
    prop_vote_op.voting_account = initdelegate.name;

    push_operation(prop_vote_op, initdelegate.private_key);

    BOOST_CHECK_EQUAL(_api.get_moderator()->name, alice.name);
}

SCORUM_TEST_CASE(check_get_user_budgets_positive)
{
    auto b1 = create_budget(alice, budget_type::banner);
    generate_block();
    auto b2 = create_budget(alice, budget_type::post);
    generate_block();
    auto b3 = create_budget(alice, budget_type::banner);
    generate_block();

    auto banner_0 = _api.get_budget(0, budget_type::banner);
    auto post_0 = _api.get_budget(0, budget_type::post);
    auto banner_1 = _api.get_budget(1, budget_type::banner);

    BOOST_CHECK_EQUAL(banner_0->json_metadata, b1.json_metadata);
    BOOST_CHECK_EQUAL(post_0->json_metadata, b2.json_metadata);
    BOOST_CHECK_EQUAL(banner_1->json_metadata, b3.json_metadata);
}

SCORUM_TEST_CASE(check_get_user_budgets_negative)
{
    create_budget(alice, budget_type::banner);
    generate_block();

    auto banner_1 = _api.get_budget(1, budget_type::banner);
    auto post_0 = _api.get_budget(0, budget_type::post);

    BOOST_CHECK(!banner_1.valid());
    BOOST_CHECK(!post_0.valid());
}

SCORUM_TEST_CASE(check_get_budget)
{
    auto b1 = create_budget(alice, budget_type::banner);
    generate_block();
    auto b2 = create_budget(alice, budget_type::post);
    generate_block();

    auto budgets = _api.get_user_budgets(alice.name);
    BOOST_REQUIRE_EQUAL(budgets.size(), 2);

    BOOST_CHECK_EQUAL(budgets[0].type, budget_type::post);
    BOOST_CHECK_EQUAL(budgets[0].json_metadata, b2.json_metadata);

    BOOST_CHECK_EQUAL(budgets[1].type, budget_type::banner);
    BOOST_CHECK_EQUAL(budgets[1].json_metadata, b1.json_metadata);
}

SCORUM_TEST_CASE(check_get_post_budget_winners_budgets_count_less_than_adv_slots)
{
    auto b1 = create_budget(alice, budget_type::post, 100, 10);
    generate_block();
    auto b2 = create_budget(alice, budget_type::post, 200, 10);
    generate_block();
    auto b3 = create_budget(alice, budget_type::post, 150, 10);
    generate_block();

    auto budgets = _api.get_current_winners(budget_type::post);
    BOOST_REQUIRE_EQUAL(budgets.size(), 3);

    BOOST_CHECK_EQUAL(budgets[0].type, budget_type::post);
    BOOST_CHECK_EQUAL(budgets[0].json_metadata, b2.json_metadata);

    BOOST_CHECK_EQUAL(budgets[1].type, budget_type::post);
    BOOST_CHECK_EQUAL(budgets[1].json_metadata, b3.json_metadata);

    BOOST_CHECK_EQUAL(budgets[2].type, budget_type::post);
    BOOST_CHECK_EQUAL(budgets[2].json_metadata, b1.json_metadata);
}

SCORUM_TEST_CASE(check_get_post_budget_winners_winners_count_less_than_budgets_count)
{
    auto b1 = create_budget(alice, budget_type::post, 300, 10);
    generate_block();
    create_budget(alice, budget_type::post, 50, 10);
    generate_block();
    auto b3 = create_budget(sam, budget_type::post, 200, 10);
    generate_block();
    create_budget(alice, budget_type::post, 100, 10);
    generate_block();
    auto b5 = create_budget(alice, budget_type::post, 150, 10);
    generate_block();
    auto b6 = create_budget(sam, budget_type::post, 250, 10);
    generate_block();

    auto vcg_coeffs = SCORUM_DEFAULT_BUDGETS_VCG_SET;
    BOOST_REQUIRE_EQUAL(vcg_coeffs.size(), 4);

    auto budgets = _api.get_current_winners(budget_type::post);
    BOOST_REQUIRE_EQUAL(budgets.size(), vcg_coeffs.size());
    BOOST_CHECK_EQUAL(budgets[0].json_metadata, b1.json_metadata);
    BOOST_CHECK_EQUAL(budgets[1].json_metadata, b6.json_metadata);
    BOOST_CHECK_EQUAL(budgets[2].json_metadata, b3.json_metadata);
    BOOST_CHECK_EQUAL(budgets[3].json_metadata, b5.json_metadata);
}

SCORUM_TEST_CASE(get_budget_winners_check_post_and_banners_do_not_affect_each_other)
{
    auto p1 = create_budget(alice, budget_type::post, 100, 10);
    generate_block();
    auto p2 = create_budget(sam, budget_type::post, 200, 10);
    generate_block();
    auto b1 = create_budget(alice, budget_type::banner, 150, 10);
    generate_block();
    auto b2 = create_budget(sam, budget_type::banner, 50, 10);
    generate_block();
    auto b3 = create_budget(alice, budget_type::banner, 70, 10);
    generate_block();

    auto post_budgets = _api.get_current_winners(budget_type::post);
    auto banner_budgets = _api.get_current_winners(budget_type::banner);

    BOOST_REQUIRE_EQUAL(post_budgets.size(), 2);
    BOOST_REQUIRE_EQUAL(banner_budgets.size(), 3);

    BOOST_CHECK_EQUAL(post_budgets[0].json_metadata, p2.json_metadata);
    BOOST_CHECK_EQUAL(post_budgets[1].json_metadata, p1.json_metadata);

    BOOST_CHECK_EQUAL(banner_budgets[0].json_metadata, b1.json_metadata);
    BOOST_CHECK_EQUAL(banner_budgets[1].json_metadata, b3.json_metadata);
    BOOST_CHECK_EQUAL(banner_budgets[2].json_metadata, b2.json_metadata);
}

BOOST_AUTO_TEST_SUITE_END()
}