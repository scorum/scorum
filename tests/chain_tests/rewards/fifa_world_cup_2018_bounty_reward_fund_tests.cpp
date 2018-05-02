#include <boost/test/unit_test.hpp>

#include "blogging_common.hpp"

#include <scorum/chain/services/reward_funds.hpp>
#include <scorum/chain/services/budget.hpp>

#include <scorum/chain/schema/budget_object.hpp>

using namespace scorum::chain;

namespace database_fixture {
struct fifa_world_cup_2018_bounty_reward_fund_fixture : public blogging_common_fixture
{
    fifa_world_cup_2018_bounty_reward_fund_fixture()
        : fifa_world_cup_2018_bounty_reward_fund_service(db.fifa_world_cup_2018_bounty_reward_fund_service())
        , reward_fund_sp_service(db.reward_fund_sp_service())
        , budget_service(db.budget_service())
    {
        const auto& fund_budget = budget_service.get_fund_budget();
        asset initial_per_block_reward = asset(fund_budget.per_block, SP_SYMBOL);

        asset witness_reward = initial_per_block_reward * SCORUM_WITNESS_PER_BLOCK_REWARD_PERCENT / SCORUM_100_PERCENT;
        content_reward = initial_per_block_reward - witness_reward;
    }

    fifa_world_cup_2018_bounty_reward_fund_service_i& fifa_world_cup_2018_bounty_reward_fund_service;
    reward_fund_sp_service_i& reward_fund_sp_service;
    budget_service_i& budget_service;

    asset content_reward = ASSET_NULL_SP;
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

    auto post_permlink = create_next_post_permlink();

    post(alice, post_permlink); // alice post

    auto start_t = dgp_service.head_block_time();

    generate_blocks(start_t + SCORUM_REVERSE_AUCTION_WINDOW_SECONDS.to_seconds());

    vote(alice, post_permlink, sam); // sam upvote alice post

    comment(alice, post_permlink, bob); // bob comment

    start_t = dgp_service.head_block_time();

    generate_blocks(start_t + SCORUM_REVERSE_AUCTION_WINDOW_SECONDS.to_seconds());

    vote(bob, get_comment_permlink(post_permlink), simon); // simon upvote bob comment

    comment(alice, post_permlink, sam); // sam comment

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
