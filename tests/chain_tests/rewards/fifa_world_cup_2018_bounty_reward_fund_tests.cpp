#include <boost/test/unit_test.hpp>

#include "database_blog_integration.hpp"

#include <scorum/chain/services/reward_funds.hpp>
#include <scorum/chain/services/budget.hpp>
#include <scorum/chain/services/account.hpp>
#include <scorum/chain/services/dynamic_global_property.hpp>

#include <scorum/chain/schema/budget_object.hpp>

using namespace scorum::chain;

namespace database_fixture {
struct fifa_world_cup_2018_bounty_reward_fund_fixture : public database_blog_integration_fixture
{
    fifa_world_cup_2018_bounty_reward_fund_fixture()
        : fifa_world_cup_2018_bounty_reward_fund_service(db.content_fifa_world_cup_2018_bounty_reward_fund_service())
        , reward_fund_sp_service(db.content_reward_fund_sp_service())
        , budget_service(db.budget_service())
        , account_service(db.account_service())
        , dgp_service(db.dynamic_global_property_service())
    {
        open_database();

        const int feed_amount = 99000;

        actor(initdelegate).create_account(alice);
        actor(initdelegate).give_scr(alice, feed_amount);
        actor(initdelegate).give_sp(alice, feed_amount);

        actor(initdelegate).create_account(bob);
        actor(initdelegate).give_scr(bob, feed_amount);
        actor(initdelegate).give_sp(bob, feed_amount);

        actor(initdelegate).create_account(sam);
        actor(initdelegate).give_scr(sam, feed_amount / 2);
        actor(initdelegate).give_sp(sam, feed_amount / 2);

        actor(initdelegate).create_account(simon);
        actor(initdelegate).give_scr(simon, feed_amount / 2);
        actor(initdelegate).give_sp(simon, feed_amount / 2);

        const auto& fund_budget = budget_service.get_fund_budget();
        asset initial_per_block_reward = fund_budget.per_block;

        asset witness_reward = initial_per_block_reward * SCORUM_WITNESS_PER_BLOCK_REWARD_PERCENT / SCORUM_100_PERCENT;
        asset active_voters_reward
            = initial_per_block_reward * SCORUM_ACTIVE_SP_HOLDERS_PER_BLOCK_REWARD_PERCENT / SCORUM_100_PERCENT;
        content_reward = initial_per_block_reward - witness_reward - active_voters_reward;
    }

    content_fifa_world_cup_2018_bounty_reward_fund_service_i& fifa_world_cup_2018_bounty_reward_fund_service;
    content_reward_fund_sp_service_i& reward_fund_sp_service;
    budget_service_i& budget_service;
    account_service_i& account_service;
    dynamic_global_property_service_i& dgp_service;

    asset content_reward = ASSET_NULL_SP;
    Actor alice = "alice";
    Actor bob = "bob";
    Actor sam = "sam";
    Actor simon = "simon";
};
}

using namespace database_fixture;

BOOST_FIXTURE_TEST_SUITE(fifa_world_cup_2018_bounty_reward_fund_tests, fifa_world_cup_2018_bounty_reward_fund_fixture)

BOOST_AUTO_TEST_CASE(bounty_fund_creation_check)
{
    BOOST_REQUIRE(!fifa_world_cup_2018_bounty_reward_fund_service.is_exists());

    generate_blocks(SCORUM_BLOGGING_START_DATE - SCORUM_BLOCK_INTERVAL);

    auto balance = reward_fund_sp_service.get().activity_reward_balance;

    generate_block();

    balance += content_reward;

    BOOST_REQUIRE(fifa_world_cup_2018_bounty_reward_fund_service.is_exists());
    BOOST_REQUIRE_EQUAL(fifa_world_cup_2018_bounty_reward_fund_service.get().activity_reward_balance, balance);
    BOOST_REQUIRE_EQUAL(reward_fund_sp_service.get().activity_reward_balance, ASSET_NULL_SP);
}

BOOST_AUTO_TEST_CASE(bounty_fund_distribution_check)
{
    BOOST_REQUIRE(!fifa_world_cup_2018_bounty_reward_fund_service.is_exists());

    generate_blocks(SCORUM_BLOGGING_START_DATE);

    BOOST_REQUIRE(fifa_world_cup_2018_bounty_reward_fund_service.is_exists());

    auto bounty_fund = fifa_world_cup_2018_bounty_reward_fund_service.get().activity_reward_balance;

    BOOST_REQUIRE_GT(bounty_fund, ASSET_NULL_SP);

    auto alice_post = create_post(alice).push();

    auto start_t = dgp_service.head_block_time();

    generate_blocks(start_t + SCORUM_REVERSE_AUCTION_WINDOW_SECONDS.to_seconds());

    alice_post.vote(sam).push(); // sam upvote alice post
    auto bob_comm = alice_post.create_comment(bob).push(); // bob comment

    start_t = dgp_service.head_block_time();

    generate_blocks(start_t + SCORUM_REVERSE_AUCTION_WINDOW_SECONDS.to_seconds());

    bob_comm.vote(simon).push(); // simon upvote bob comment
    alice_post.create_comment(sam).push();

    start_t = dgp_service.head_block_time();

    generate_blocks(start_t + SCORUM_CASHOUT_WINDOW_SECONDS);

    auto alice_old_balance = account_service.get_account(alice.name).scorumpower;
    auto bob_old_balance = account_service.get_account(bob.name).scorumpower;
    auto sam_old_balance = account_service.get_account(sam.name).scorumpower;

    BOOST_REQUIRE_LT(dgp_service.head_block_time().sec_since_epoch(),
                     SCORUM_FIFA_WORLD_CUP_2018_BOUNTY_CASHOUT_DATE.sec_since_epoch());

    generate_blocks(SCORUM_FIFA_WORLD_CUP_2018_BOUNTY_CASHOUT_DATE);

    BOOST_REQUIRE_EQUAL(fifa_world_cup_2018_bounty_reward_fund_service.get().activity_reward_balance, ASSET_NULL_SP);

    auto alice_balance = account_service.get_account(alice.name).scorumpower;
    auto bob_balance = account_service.get_account(bob.name).scorumpower;
    auto sam_balance = account_service.get_account(sam.name).scorumpower;

    auto alice_balance_delta = alice_balance - alice_old_balance;
    auto bob_balance_delta = bob_balance - bob_old_balance;
    auto sam_balance_delta = sam_balance - sam_old_balance;

    BOOST_REQUIRE_GT(alice_balance_delta, ASSET_NULL_SP);
    BOOST_REQUIRE_GT(bob_balance_delta, ASSET_NULL_SP);
    BOOST_REQUIRE_EQUAL(sam_balance_delta, ASSET_NULL_SP); // no votes for sam comment

    BOOST_REQUIRE_EQUAL(alice_balance_delta + bob_balance_delta, bounty_fund);

    BOOST_TEST_MESSAGE("--- Test no double reward");

    alice_old_balance = account_service.get_account(alice.name).scorumpower;
    bob_old_balance = account_service.get_account(bob.name).scorumpower;

    generate_blocks(dgp_service.head_block_time() + SCORUM_CASHOUT_WINDOW_SECONDS);

    alice_balance = account_service.get_account(alice.name).scorumpower;
    bob_balance = account_service.get_account(bob.name).scorumpower;

    alice_balance_delta = alice_balance - alice_old_balance;
    bob_balance_delta = bob_balance - bob_old_balance;

    BOOST_REQUIRE_EQUAL(alice_balance_delta, ASSET_NULL_SP);
    BOOST_REQUIRE_EQUAL(bob_balance_delta, ASSET_NULL_SP);
}

BOOST_AUTO_TEST_SUITE_END()
