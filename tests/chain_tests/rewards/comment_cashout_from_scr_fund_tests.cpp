#include <boost/test/unit_test.hpp>

#include "database_blog_integration.hpp"

#include <scorum/chain/services/reward_funds.hpp>
#include <scorum/chain/services/budget.hpp>
#include <scorum/chain/services/reward_balancer.hpp>
#include <scorum/chain/services/account.hpp>

#include <scorum/chain/schema/budget_object.hpp>
#include <scorum/chain/schema/reward_balancer_objects.hpp>

using namespace scorum::chain;

namespace database_fixture {
struct comment_cashout_from_scr_fund_fixture : public database_blog_integration_fixture
{
    comment_cashout_from_scr_fund_fixture()
        : budget_service(db.budget_service())
        , reward_fund_scr_service(db.content_reward_fund_scr_service())
        , reward_balancer(db.content_reward_scr_service())
        , account_service(db.obtain_service<dbs_account>())
        , alice("alice")
        , sam("sam")
    {
        open_database();

        const int feed_amount = 99000;

        actor(initdelegate).create_account(alice);
        actor(initdelegate).give_scr(alice, feed_amount);
        actor(initdelegate).give_sp(alice, feed_amount);

        actor(initdelegate).create_account(sam);
        actor(initdelegate).give_scr(sam, feed_amount / 2);
        actor(initdelegate).give_sp(sam, feed_amount / 2);

        generate_block();

        const int deadline_block_count = 10;
        const auto& owner = account_service.get_account(alice.name);

        budget_service.create_budget(owner, ASSET_SCR(deadline_block_count),
                                     db.head_block_time() + SCORUM_BLOCK_INTERVAL * deadline_block_count);

        generate_blocks(deadline_block_count);

        BOOST_REQUIRE_EQUAL(budget_service.get_budgets(alice.name).size(), 0u);

        activity_reward_balance = reward_fund_scr_service.get().activity_reward_balance;

        BOOST_REQUIRE_GT(activity_reward_balance, ASSET_NULL_SCR);

        const content_reward_balancer_scr_object& rb = reward_balancer.get();

        BOOST_REQUIRE_EQUAL(rb.balance, ASSET_NULL_SCR);
    }

    budget_service_i& budget_service;
    content_reward_fund_scr_service_i& reward_fund_scr_service;
    content_reward_scr_service_i& reward_balancer;
    account_service_i& account_service;

    asset activity_reward_balance = ASSET_NULL_SP;
    Actor alice;
    Actor sam;
};
}

using namespace database_fixture;

BOOST_FIXTURE_TEST_SUITE(comment_cashout_from_scr_fund_tests, comment_cashout_from_scr_fund_fixture)

BOOST_AUTO_TEST_CASE(cashout_check)
{
    auto alice_old_balance = account_service.get_account(alice.name).balance;
    auto sam_old_balance = account_service.get_account(sam.name).balance;

    auto alice_post = create_post(alice).push();

    const int vote_interval = SCORUM_CASHOUT_WINDOW_SECONDS / 2;
    generate_blocks(db.head_block_time() + vote_interval);

    alice_post.vote(sam).push(); // sam upvote alice post

    generate_blocks(db.head_block_time() + SCORUM_CASHOUT_WINDOW_SECONDS - vote_interval);

    auto alice_balance = account_service.get_account(alice.name).balance;
    auto sam_balance = account_service.get_account(sam.name).balance;

    auto alice_balance_delta = alice_balance - alice_old_balance;
    auto sam_balance_delta = sam_balance - sam_old_balance;

    BOOST_REQUIRE_GT(alice_balance_delta, ASSET_NULL_SCR);
    BOOST_REQUIRE_GT(sam_balance_delta, ASSET_NULL_SCR);

    BOOST_REQUIRE_EQUAL(alice_balance_delta + sam_balance_delta, activity_reward_balance);
}

BOOST_AUTO_TEST_CASE(no_double_cashout_check)
{
    auto alice_old_balance = account_service.get_account(alice.name).balance;
    auto sam_old_balance = account_service.get_account(sam.name).balance;

    auto alice_post = create_post(alice).push();

    const int vote_interval = SCORUM_CASHOUT_WINDOW_SECONDS / 2;

    generate_blocks(db.head_block_time() + vote_interval);

    alice_post.vote(sam).push(); // sam upvote alice post

    generate_blocks(db.head_block_time() + SCORUM_CASHOUT_WINDOW_SECONDS - vote_interval);

    auto alice_balance = account_service.get_account(alice.name).balance;
    auto sam_balance = account_service.get_account(sam.name).balance;

    BOOST_REQUIRE_GT(alice_balance - alice_old_balance, ASSET_NULL_SCR);
    BOOST_REQUIRE_GT(sam_balance - sam_old_balance, ASSET_NULL_SCR);

    alice_old_balance = account_service.get_account(alice.name).balance;
    sam_old_balance = account_service.get_account(sam.name).balance;

    generate_blocks(db.head_block_time() + SCORUM_CASHOUT_WINDOW_SECONDS);

    alice_balance = account_service.get_account(alice.name).balance;
    sam_balance = account_service.get_account(sam.name).balance;

    BOOST_REQUIRE_EQUAL(alice_balance - alice_old_balance, ASSET_NULL_SCR);
    BOOST_REQUIRE_EQUAL(sam_balance - sam_old_balance, ASSET_NULL_SCR);
}

BOOST_AUTO_TEST_SUITE_END()
