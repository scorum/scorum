#include <boost/test/unit_test.hpp>

#include "database_default_integration.hpp"

#include <scorum/chain/services/reward_fund.hpp>
#include <scorum/chain/services/budget.hpp>
#include <scorum/chain/services/comment.hpp>
#include <scorum/chain/services/comment_vote.hpp>
#include <scorum/chain/services/dynamic_global_property.hpp>
#include <scorum/chain/services/account.hpp>

#include <scorum/chain/schema/budget_object.hpp>
#include <scorum/chain/schema/comment_objects.hpp>
#include <scorum/chain/schema/dynamic_global_property_object.hpp>
#include <scorum/chain/schema/account_objects.hpp>

#include <sstream>

using namespace scorum::chain;

namespace database_fixture {
struct fifa_world_cup_2018_bounty_reward_fund_fixture : public database_default_integration_fixture
{
    fifa_world_cup_2018_bounty_reward_fund_fixture()
        : fifa_world_cup_2018_bounty_reward_fund_service(db.fifa_world_cup_2018_bounty_reward_fund_service())
        , reward_fund_sp_service(db.reward_fund_sp_service())
        , budget_service(db.budget_service())
        , comment_service(db.comment_service())
        , comment_vote_service(db.comment_vote_service())
        , account_service(db.account_service())
        , dgp_service(db.dynamic_global_property_service())
        , alice("alice")
        , bob("bob")
        , sam("sam")
        , sam2("sam2")
    {
        const auto& fund_budget = budget_service.get_fund_budget();
        asset initial_per_block_reward = asset(fund_budget.per_block, SP_SYMBOL);

        asset witness_reward = initial_per_block_reward * SCORUM_WITNESS_PER_BLOCK_REWARD_PERCENT / SCORUM_100_PERCENT;
        content_reward = initial_per_block_reward - witness_reward;

        actor(initdelegate).create_account(alice);
        actor(initdelegate).give_scr(alice, feed_amount);
        actor(initdelegate).give_sp(alice, feed_amount);

        actor(initdelegate).create_account(bob);
        actor(initdelegate).give_scr(bob, feed_amount / 2);
        actor(initdelegate).give_sp(bob, feed_amount / 2);

        actor(initdelegate).create_account(sam);
        actor(initdelegate).give_scr(sam, feed_amount / 3);
        actor(initdelegate).give_sp(sam, feed_amount / 3);

        actor(initdelegate).create_account(sam2);
        actor(initdelegate).give_scr(sam2, feed_amount / 4);
        actor(initdelegate).give_sp(sam2, feed_amount / 4);
    }

    std::string create_next_permlink()
    {
        static int next = 0;
        std::stringstream store;
        store << "blog-" << ++next;
        return store.str();
    }

    const comment_object& alice_post(const std::string& permlink)
    {
        comment_operation comment;

        comment.author = alice.name;
        comment.permlink = permlink;
        comment.parent_permlink = "posts";
        comment.title = "foo";
        comment.body = "bar";

        push_operation_only(comment, alice.private_key);

        return comment_service.get(alice.name, permlink);
    }

    std::string get_comment_permlink(const std::string& post_permlink)
    {
        return std::string("re-") + post_permlink;
    }

    const comment_object& comment(const Actor& author, const std::string& post_permlink)
    {
        comment_operation comment;

        comment.author = author.name;
        comment.permlink = get_comment_permlink(post_permlink);
        comment.parent_author = alice.name;
        comment.parent_permlink = post_permlink;
        comment.title = "re: foo";
        comment.body = "re: bar";

        push_operation_only(comment, author.private_key);

        return comment_service.get(author.name, comment.permlink);
    }

    void vote_for_post(const Actor& voter, const std::string& post_permlink)
    {
        vote_operation vote;

        vote.voter = voter.name;
        vote.author = alice.name;
        vote.permlink = post_permlink;
        vote.weight = (int16_t)100;

        push_operation_only(vote, voter.private_key);
    }

    void vote_for_comment(const Actor& author, const Actor& voter, const std::string& post_permlink)
    {
        vote_operation vote;

        vote.voter = voter.name;
        vote.author = author.name;
        vote.permlink = get_comment_permlink(post_permlink);
        vote.weight = (int16_t)100;

        push_operation_only(vote, voter.private_key);
    }

    fifa_world_cup_2018_bounty_reward_fund_service_i& fifa_world_cup_2018_bounty_reward_fund_service;
    reward_fund_sp_service_i& reward_fund_sp_service;
    budget_service_i& budget_service;
    comment_service_i& comment_service;
    comment_vote_service_i& comment_vote_service;
    account_service_i& account_service;
    dynamic_global_property_service_i& dgp_service;

    asset content_reward = ASSET_NULL_SP;

    Actor alice;
    Actor bob;
    Actor sam;
    Actor sam2;

    const int feed_amount = 99000;
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

    auto post_permlink = create_next_permlink();

    alice_post(post_permlink); // alice post

    auto start_t = dgp_service.head_block_time();

    generate_blocks(start_t + SCORUM_REVERSE_AUCTION_WINDOW_SECONDS.to_seconds());

    vote_for_post(sam, post_permlink);

    comment(bob, post_permlink); // bob comment

    start_t = dgp_service.head_block_time();

    generate_blocks(start_t + SCORUM_REVERSE_AUCTION_WINDOW_SECONDS.to_seconds());

    vote_for_comment(bob, sam2, post_permlink);

    comment(sam, post_permlink); // sam comment

    start_t = dgp_service.head_block_time();

    generate_blocks(start_t + SCORUM_CASHOUT_WINDOW_SECONDS);

    auto alice_old_balance = account_service.get_account(alice.name).scorumpower;
    auto bob_old_balance = account_service.get_account(bob.name).scorumpower;
    auto sam_old_balance = account_service.get_account(sam.name).scorumpower;

    BOOST_REQUIRE_LT(dgp_service.head_block_time().sec_since_epoch(),
                     SCORUM_BLOGGING_BOUNTY_CASHOUT_DATE.sec_since_epoch());

    generate_blocks(SCORUM_BLOGGING_BOUNTY_CASHOUT_DATE);

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
}

BOOST_AUTO_TEST_SUITE_END()
