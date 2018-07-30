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

SCORUM_TEST_CASE(get_budget_winners_check_post_and_banners_do_not_affect_each_other)
{
    auto p1 = create_budget(alice, budget_type::post, 100, 10); // id = 0
    generate_block();
    auto p2 = create_budget(sam, budget_type::post, 200, 10); // id = 1
    generate_block();
    auto b1 = create_budget(alice, budget_type::banner, 150, 10); // id = 0
    generate_block();
    auto b2 = create_budget(sam, budget_type::banner, 50, 10); // id = 1
    generate_block();
    auto b3 = create_budget(alice, budget_type::banner, 70, 10); // id = 2
    generate_block();

    auto post_budgets = _api.get_current_winners(budget_type::post);
    auto banner_budgets = _api.get_current_winners(budget_type::banner);

    BOOST_REQUIRE_EQUAL(post_budgets.size(), 2u);
    BOOST_REQUIRE_EQUAL(banner_budgets.size(), 3u);

    BOOST_CHECK_EQUAL(post_budgets[0].id, 1u);
    BOOST_CHECK_EQUAL(post_budgets[1].id, 0u);

    BOOST_CHECK_EQUAL(banner_budgets[0].id, 0u);
    BOOST_CHECK_EQUAL(banner_budgets[1].id, 2u);
    BOOST_CHECK_EQUAL(banner_budgets[2].id, 1u);
}

BOOST_AUTO_TEST_SUITE_END()
}
