#include <boost/test/unit_test.hpp>

#include "database_blog_integration.hpp"

#include <scorum/chain/services/reward_funds.hpp>
#include <scorum/chain/services/budget.hpp>
#include <scorum/chain/services/account.hpp>
#include <scorum/chain/services/dynamic_global_property.hpp>

#include <scorum/chain/schema/budget_object.hpp>

namespace fifa_world_cup_2018_bounty_reward_fund_tests {
using namespace database_fixture;
using namespace scorum::chain;

struct base_fifa_world_cup_2018_bounty_reward_fund_fixture : public database_blog_integration_fixture
{
    base_fifa_world_cup_2018_bounty_reward_fund_fixture()
        : fifa_world_cup_2018_bounty_reward_fund_service(db.content_fifa_world_cup_2018_bounty_reward_fund_service())
        , reward_fund_sp_service(db.content_reward_fund_sp_service())
        , budget_service(db.budget_service())
        , account_service(db.account_service())
        , dgp_service(db.dynamic_global_property_service())
    {
        open_database();

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
};

class rewards_stat
{
    struct rewards_stat_data
    {
        share_type author_reward;
        share_type curators_reward;
        share_type beneficiaries_reward;
        share_type commenting_reward;
    };

public:
    typedef void result_type;

    rewards_stat() = delete;

    rewards_stat(database& db)
        : _db(db)
    {
        _conn = db.post_apply_operation.connect([&](const operation_notification& note) { note.op.visit(*this); });
    }

    ~rewards_stat()
    {
        _conn.disconnect();
    }

    void reset()
    {
        _stats.clear();
    }

    int64_t author_reward(const Actor& a)
    {
        return _stats[a.name].author_reward.value;
    }

    int64_t curators_reward(const Actor& a)
    {
        return _stats[a.name].curators_reward.value;
    }

    int64_t beneficiaries_reward(const Actor& a)
    {
        return _stats[a.name].beneficiaries_reward.value;
    }

    int64_t commenting_reward(const Actor& a)
    {
        return _stats[a.name].commenting_reward.value;
    }

    void operator()(const comment_reward_operation& op)
    {
        _stats[op.author].author_reward += op.author_reward.amount;
        _stats[op.author].beneficiaries_reward += op.beneficiaries_reward.amount;
        _stats[op.author].commenting_reward += op.commenting_reward.amount;
    }

    void operator()(const curation_reward_operation& op)
    {
        _stats[op.curator].curators_reward += op.reward.amount;
    }

    template <typename Op> void operator()(Op&&) const
    {
    } /// ignore all other ops

private:
    database& _db;
    fc::scoped_connection _conn;
    std::map<account_name_type, rewards_stat_data> _stats;
};

struct fifa_world_cup_2018_bounty_reward_fund_fixture : public base_fifa_world_cup_2018_bounty_reward_fund_fixture
{
    fifa_world_cup_2018_bounty_reward_fund_fixture()
    {
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

        actor(initdelegate).create_account(lee);
        actor(initdelegate).give_scr(lee, feed_amount / 2);
        actor(initdelegate).give_sp(lee, feed_amount / 2);

        actor(initdelegate).create_account(simon);
        actor(initdelegate).give_scr(simon, feed_amount / 2);
        actor(initdelegate).give_sp(simon, feed_amount / 2);

        actor(initdelegate).create_account(andrew);
        actor(initdelegate).give_scr(andrew, feed_amount / 2);
        actor(initdelegate).give_sp(andrew, feed_amount / 2);

        BOOST_REQUIRE_LT(SCORUM_REVERSE_AUCTION_WINDOW_SECONDS.to_seconds() * 10, SCORUM_CASHOUT_WINDOW_SECONDS);

        BOOST_REQUIRE(!fifa_world_cup_2018_bounty_reward_fund_service.is_exists());

        generate_blocks(SCORUM_BLOGGING_START_DATE);

        bounty_fund = fifa_world_cup_2018_bounty_reward_fund_service.get().activity_reward_balance;

        BOOST_REQUIRE_GT(bounty_fund, ASSET_NULL_SP);
    }

    void create_payed_activity_case()
    {
        BOOST_TEST_MESSAGE("--- Create payed before FIFA activity");

        auto alice_post = create_post(alice).push(); // Alice create post

        auto start_t = dgp_service.head_block_time();

        generate_blocks(start_t + SCORUM_REVERSE_AUCTION_WINDOW_SECONDS.to_seconds());

        alice_post.vote(simon).push(); // Simon upvote alice post

        auto bob_comm = alice_post.create_comment(bob).push(); // Bob create comment

        {
            // Bob set Sam like beneficiar
            comment_options_operation op;
            op.author = bob_comm.author();
            op.permlink = bob_comm.permlink();
            op.allow_curation_rewards = true;
            comment_payout_beneficiaries b;
            b.beneficiaries.push_back(beneficiary_route_type(sam.name, 20 * SCORUM_1_PERCENT));
            op.extensions.insert(b);
            push_operation(op, bob.private_key);
        }

        start_t = dgp_service.head_block_time();

        generate_blocks(start_t + SCORUM_REVERSE_AUCTION_WINDOW_SECONDS.to_seconds());

        bob_comm.vote(simon).push(); // Simon upvote Bob comment
        alice_post.create_comment(sam).push(); // Sam create comment

        start_t = dgp_service.head_block_time();

        generate_blocks(start_t + SCORUM_CASHOUT_WINDOW_SECONDS); // cashout for all
    }

    void create_unpayed_activity_case()
    {
        BOOST_TEST_MESSAGE("--- Create unpayed before FIFA activity");

        auto post_without_cashout = SCORUM_FIFA_WORLD_CUP_2018_BOUNTY_CASHOUT_DATE - SCORUM_CASHOUT_WINDOW_SECONDS / 2;
        BOOST_REQUIRE_LT(dgp_service.head_block_time().sec_since_epoch(), post_without_cashout.sec_since_epoch());

        generate_blocks(post_without_cashout);

        auto lee_post = create_post(lee).push(); // Lee create post

        auto start_t = dgp_service.head_block_time();

        generate_blocks(start_t + SCORUM_REVERSE_AUCTION_WINDOW_SECONDS.to_seconds());

        lee_post.vote(andrew).push(); // Andrew upvote Lee post

        BOOST_REQUIRE_LT(dgp_service.head_block_time().sec_since_epoch(),
                         SCORUM_FIFA_WORLD_CUP_2018_BOUNTY_CASHOUT_DATE.sec_since_epoch());
    }

    Actor alice = "alice";
    Actor bob = "bob";
    Actor sam = "sam";
    Actor lee = "lee";
    Actor simon = "simon";
    Actor andrew = "andrew";

    asset bounty_fund = ASSET_NULL_SP;
};

BOOST_AUTO_TEST_SUITE(fifa_world_cup_2018_bounty_reward_fund_tests)

BOOST_FIXTURE_TEST_CASE(bounty_fund_creation_check, base_fifa_world_cup_2018_bounty_reward_fund_fixture)
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

BOOST_FIXTURE_TEST_CASE(base_fund_distribution_check, fifa_world_cup_2018_bounty_reward_fund_fixture)
{
    rewards_stat rewards(db);

    create_payed_activity_case();

    BOOST_TEST_MESSAGE("--- Test FIFA reward");

    // disregard previous payments stat
    rewards.reset();

    generate_blocks(SCORUM_FIFA_WORLD_CUP_2018_BOUNTY_CASHOUT_DATE);

    BOOST_REQUIRE_EQUAL(fifa_world_cup_2018_bounty_reward_fund_service.get().activity_reward_balance, ASSET_NULL_SP);

    BOOST_CHECK_GT(rewards.author_reward(alice), 0);
    BOOST_CHECK_GT(rewards.author_reward(bob), 0);
    BOOST_CHECK_GT(rewards.beneficiaries_reward(bob), 0);
    BOOST_CHECK_EQUAL(rewards.author_reward(sam), 0); // no votes for Sam comment

    BOOST_REQUIRE_EQUAL(rewards.author_reward(alice) + rewards.author_reward(bob) + rewards.beneficiaries_reward(bob),
                        bounty_fund.amount.value);

    BOOST_TEST_MESSAGE("--- Test no double reward");

    rewards.reset();

    generate_blocks(dgp_service.head_block_time() + SCORUM_CASHOUT_WINDOW_SECONDS);

    BOOST_CHECK_EQUAL(rewards.author_reward(alice), 0);
    BOOST_CHECK_EQUAL(rewards.author_reward(bob), 0);
}

BOOST_FIXTURE_TEST_CASE(fifa_cashout_only_for_payed_comments_check, fifa_world_cup_2018_bounty_reward_fund_fixture)
{
    rewards_stat rewards(db);

    create_payed_activity_case();

    create_unpayed_activity_case();

    BOOST_CHECK_EQUAL(rewards.author_reward(lee), 0);

    BOOST_TEST_MESSAGE("--- Test FIFA reward");

    generate_blocks(SCORUM_FIFA_WORLD_CUP_2018_BOUNTY_CASHOUT_DATE);

    BOOST_REQUIRE_EQUAL(fifa_world_cup_2018_bounty_reward_fund_service.get().activity_reward_balance, ASSET_NULL_SP);

    BOOST_CHECK_EQUAL(rewards.author_reward(lee), 0);

    BOOST_TEST_MESSAGE("--- Test that Lee will receive ordinary reward");

    generate_blocks(dgp_service.head_block_time() + SCORUM_CASHOUT_WINDOW_SECONDS);

    BOOST_CHECK_GT(rewards.author_reward(lee), 0);
}

BOOST_FIXTURE_TEST_CASE(no_fifa_cashout_for_curators_check, fifa_world_cup_2018_bounty_reward_fund_fixture)
{
    rewards_stat rewards(db);

    create_payed_activity_case();

    BOOST_CHECK_GT(rewards.author_reward(alice), 0); // Alice wrote post with votes and comments
    BOOST_CHECK_GT(rewards.author_reward(bob), 0); // Bob wrote voted comment
    BOOST_CHECK_GT(rewards.commenting_reward(alice), 0); // Alice has child comment
    BOOST_CHECK_EQUAL(rewards.author_reward(sam), 0); // Sam wrote unvoted comment
    BOOST_CHECK_GT(rewards.curators_reward(simon), 0); // Simon did vote

    create_unpayed_activity_case();

    BOOST_CHECK_EQUAL(rewards.author_reward(lee), 0); // No time out yet
    BOOST_CHECK_EQUAL(rewards.curators_reward(andrew), 0); // No time out yet

    BOOST_TEST_MESSAGE("--- Test FIFA reward");

    rewards.reset();

    generate_blocks(SCORUM_FIFA_WORLD_CUP_2018_BOUNTY_CASHOUT_DATE);

    BOOST_REQUIRE_EQUAL(fifa_world_cup_2018_bounty_reward_fund_service.get().activity_reward_balance, ASSET_NULL_SP);

    BOOST_CHECK_GT(rewards.author_reward(alice), 0); // Alice wrote payed post
    BOOST_CHECK_GT(rewards.author_reward(bob), 0); // Bob wrote payed comment
    BOOST_CHECK_GT(rewards.beneficiaries_reward(bob), 0); // Bob payed to beneficiar
    BOOST_CHECK_GT(rewards.commenting_reward(alice), 0); // Alice has child comment
    BOOST_CHECK_EQUAL(rewards.author_reward(sam), 0); // no votes for Sam comment
    BOOST_CHECK_EQUAL(rewards.author_reward(lee),
                      0); // Lee had not reached cashout time before FIFA cashout is happened
    BOOST_CHECK_EQUAL(rewards.curators_reward(simon), 0); // No payout for curators
    BOOST_CHECK_EQUAL(rewards.curators_reward(andrew), 0); // No payout for curators

    BOOST_TEST_MESSAGE("--- Test that Lee curator will receive ordinary reward");

    generate_blocks(dgp_service.head_block_time() + SCORUM_CASHOUT_WINDOW_SECONDS);

    BOOST_CHECK_GT(rewards.author_reward(lee), 0); // for post
    BOOST_CHECK_GT(rewards.curators_reward(andrew), 0); // for vote
}

BOOST_AUTO_TEST_SUITE_END()
}
