#include <boost/test/unit_test.hpp>

#include <scorum/chain/database/database.hpp>

#include <scorum/chain/database_exceptions.hpp>
#include <scorum/chain/schema/scorum_objects.hpp>

#include <scorum/chain/schema/account_objects.hpp>
#include <scorum/chain/services/account.hpp>
#include <scorum/chain/services/comment.hpp>
#include <scorum/chain/services/comment_statistic.hpp>
#include <scorum/chain/schema/comment_objects.hpp>

#include "database_blog_integration.hpp"
#include "actoractions.hpp"

#include <map>

using namespace database_fixture;

struct comment_stats
{
    share_type author_reward;
    share_type curator_reward;
    share_type benefactor_reward;
    share_type fund_reward;
};

struct comments_hierarchy_reward_visitor
{
    typedef void result_type;

    database& _db;
    fc::scoped_connection conn;

    std::map<std::string, comment_stats> stats;

    comments_hierarchy_reward_visitor(database& db)
        : _db(db)
    {
        conn = db.post_apply_operation.connect([&](const operation_notification& note) { note.op.visit(*this); });
    }

    ~comments_hierarchy_reward_visitor()
    {
        conn.disconnect();
    }

    void operator()(const comment_benefactor_reward_operation& op)
    {
        stats[op.author].benefactor_reward += op.reward.amount;
    }

    void operator()(const curation_reward_operation& op)
    {
        stats[op.comment_author].curator_reward += op.reward.amount;
    }

    void operator()(const author_reward_operation& op)
    {
        stats[op.author].author_reward += op.reward.amount;
    }

    void operator()(const comment_reward_operation& op)
    {
        stats[op.author].fund_reward += op.fund_reward.amount;
    }

    template <typename Op> void operator()(Op&&) const
    {
    } /// ignore all other ops
};

struct comments_hierarchy_reward_fixture : public database_blog_integration_fixture
{
    comment_statistic_sp_service_i& comment_statistic_sp_service;
    account_service_i& account_service;

    Actor user00;
    Actor user10;
    Actor user11;
    Actor user20;
    Actor user21;
    Actor voter00;
    Actor voter10;
    Actor voter11;
    Actor voter20;
    Actor voter21;

    comments_hierarchy_reward_fixture()
        : comment_statistic_sp_service(db.obtain_service<dbs_comment_statistic_sp>())
        , account_service(db.obtain_service<dbs_account>())
    {
        open_database();

        user00 = create_actor("user00");
        user10 = create_actor("user10");
        user11 = create_actor("user11");
        user20 = create_actor("user20");
        user21 = create_actor("user21");
        voter00 = create_actor("voter00");
        voter10 = create_actor("voter10");
        voter11 = create_actor("voter11");
        voter20 = create_actor("voter20");
        voter21 = create_actor("voter21");
    }

    Actor create_actor(const char* name)
    {
        Actor acc(name);

        actor(initdelegate).create_account(acc);

        actor(initdelegate).give_sp(acc, 1e5);

        return acc;
    }

    share_type check_comment_rewards(const Actor& acc, const comment_stats& stat, share_type payout_from_children)
    {
        share_type user_parent_reward = (stat.fund_reward - stat.curator_reward + payout_from_children)
            * SCORUM_PARENT_COMMENT_REWARD_PERCENT / SCORUM_100_PERCENT;

        BOOST_TEST_MESSAGE("");
        BOOST_TEST_MESSAGE("Comment author: " << acc.name);
        BOOST_TEST_MESSAGE("Comment publication reward: " << stat.fund_reward);
        BOOST_TEST_MESSAGE("Comment commenting reward: " << payout_from_children);
        BOOST_TEST_MESSAGE("Author reward: " << stat.author_reward);
        BOOST_TEST_MESSAGE("Curators reward: " << stat.curator_reward);
        BOOST_TEST_MESSAGE("Beneficiary reward: " << stat.benefactor_reward);
        BOOST_TEST_MESSAGE("Parent comment reward: " << user_parent_reward);
        BOOST_TEST_MESSAGE("");

        BOOST_REQUIRE_EQUAL(user_parent_reward + stat.author_reward + stat.curator_reward + stat.benefactor_reward,
                            stat.fund_reward + payout_from_children);

        return user_parent_reward;
    }

    void check_post_rewards(const comment_stats& stat, share_type payout_from_children)
    {
        BOOST_TEST_MESSAGE("");
        BOOST_TEST_MESSAGE("Comment publication reward: " << stat.fund_reward);
        BOOST_TEST_MESSAGE("Comment commenting reward: " << payout_from_children);
        BOOST_TEST_MESSAGE("Author reward: " << stat.author_reward);
        BOOST_TEST_MESSAGE("Curators reward: " << stat.curator_reward);
        BOOST_TEST_MESSAGE("Beneficiary reward: " << stat.benefactor_reward);
        BOOST_TEST_MESSAGE("");

        BOOST_REQUIRE_EQUAL(stat.author_reward + stat.benefactor_reward + stat.curator_reward,
                            stat.fund_reward + payout_from_children);
    }
};

BOOST_FIXTURE_TEST_SUITE(comment_hierarchy_reward_tests, comments_hierarchy_reward_fixture)

BOOST_AUTO_TEST_CASE(check_all_comments_on_same_block)
{
    /*
        user00 -> block1
            user10 -> block1
                user20 -> block1
    */

    auto p = create_post(user00).push();
    auto c10 = p.create_comment(user10).push();
    auto c20 = c10.create_comment(user20).in_block(SCORUM_REVERSE_AUCTION_WINDOW_SECONDS);

    p.vote(voter00).push();
    c10.vote(voter10).push();
    c20.vote(voter20).push();

    comments_hierarchy_reward_visitor v(db);

    // go to cashout
    generate_blocks(db.head_block_time() + fc::seconds(SCORUM_CASHOUT_WINDOW_SECONDS)
                    - SCORUM_REVERSE_AUCTION_WINDOW_SECONDS);
    auto user20_parent_reward = check_comment_rewards(user20, v.stats[user20.name], 0);
    auto user10_parent_reward = check_comment_rewards(user10, v.stats[user10.name], user20_parent_reward);
    check_post_rewards(v.stats[user00.name], user10_parent_reward);
}

BOOST_AUTO_TEST_CASE(check_each_comment_on_diff_block)
{
    /*
        user00 -> block1
            user10 -> block2
                user20 -> block3
    */

    uint32_t delay = 10 * SCORUM_BLOCK_INTERVAL;
    uint32_t blocks_after_post = 2;

    auto p = create_post(user00).in_block(delay);
    auto c10 = p.create_comment(user10).in_block(delay);
    auto c20 = c10.create_comment(user20).in_block(SCORUM_REVERSE_AUCTION_WINDOW_SECONDS);

    p.vote(voter00).push();
    c10.vote(voter10).push();
    c20.vote(voter20).push();

    BOOST_TEST_MESSAGE("User00 cashout");
    {
        comments_hierarchy_reward_visitor v(db);

        // go to user00 cashout
        generate_blocks(db.head_block_time() + fc::seconds(SCORUM_CASHOUT_WINDOW_SECONDS)
                        - fc::seconds(blocks_after_post * delay) - SCORUM_REVERSE_AUCTION_WINDOW_SECONDS);
        check_post_rewards(v.stats[user00.name], 0);
    }
    BOOST_TEST_MESSAGE("User10 cashout");
    {
        comments_hierarchy_reward_visitor v(db);

        // go to user10 cashout
        generate_blocks(db.head_block_time() + fc::seconds(delay));
        auto user10_parent_reward = check_comment_rewards(user10, v.stats[user10.name], 0);
        check_post_rewards(v.stats[user00.name], user10_parent_reward);
    }
    BOOST_TEST_MESSAGE("User20 cashout");
    {
        comments_hierarchy_reward_visitor v(db);

        // go to user20 cashout
        generate_blocks(db.head_block_time() + fc::seconds(delay));
        auto user20_parent_reward = check_comment_rewards(user20, v.stats[user20.name], 0);
        auto user10_parent_reward = check_comment_rewards(user10, v.stats[user10.name], user20_parent_reward);
        check_post_rewards(v.stats[user00.name], user10_parent_reward);
    }
}

BOOST_AUTO_TEST_CASE(check_multiple_children_on_diff_block)
{
    /*
        user00 -> block1
            user10 -> block2
                user20 -> block3
                user21 -> block4
            user11 -> block5
    */

    uint32_t delay = 10 * SCORUM_BLOCK_INTERVAL;
    uint32_t blocks_after_post = 4;

    auto p = create_post(user00).in_block(delay);
    auto c10 = p.create_comment(user10).in_block(delay);
    auto c20 = c10.create_comment(user20).in_block(delay);
    auto c21 = c10.create_comment(user21).in_block(delay);
    auto c11 = p.create_comment(user11).in_block(SCORUM_REVERSE_AUCTION_WINDOW_SECONDS);

    p.vote(voter00).push();
    c10.vote(voter10).push();
    c11.vote(voter11).push();
    c20.vote(voter20).push();
    c21.vote(voter21).push();

    BOOST_TEST_MESSAGE("User00 cashout");
    {
        comments_hierarchy_reward_visitor v(db);

        // go to user00 cashout
        generate_blocks(db.head_block_time() + fc::seconds(SCORUM_CASHOUT_WINDOW_SECONDS)
                        - fc::seconds(blocks_after_post * delay) - SCORUM_REVERSE_AUCTION_WINDOW_SECONDS);
        check_post_rewards(v.stats[user00.name], 0);
    }
    BOOST_TEST_MESSAGE("User10 cashout");
    {
        comments_hierarchy_reward_visitor v(db);

        // go to user10 cashout
        generate_blocks(db.head_block_time() + fc::seconds(delay));
        auto user10_parent_reward = check_comment_rewards(user10, v.stats[user10.name], 0);
        check_post_rewards(v.stats[user00.name], user10_parent_reward);
    }
    BOOST_TEST_MESSAGE("User20 cashout");
    {
        comments_hierarchy_reward_visitor v(db);

        // go to user20 cashout
        generate_blocks(db.head_block_time() + fc::seconds(delay));
        auto user20_parent_reward = check_comment_rewards(user20, v.stats[user20.name], 0);
        auto user10_parent_reward = check_comment_rewards(user10, v.stats[user10.name], user20_parent_reward);
        check_post_rewards(v.stats[user00.name], user10_parent_reward);
    }
    BOOST_TEST_MESSAGE("User21 cashout");
    {
        comments_hierarchy_reward_visitor v(db);

        // go to user21 cashout
        generate_blocks(db.head_block_time() + fc::seconds(delay));
        auto user21_parent_reward = check_comment_rewards(user21, v.stats[user21.name], 0);
        auto user10_parent_reward = check_comment_rewards(user10, v.stats[user10.name], user21_parent_reward);
        check_post_rewards(v.stats[user00.name], user10_parent_reward);
    }
    BOOST_TEST_MESSAGE("User11 cashout");
    {
        comments_hierarchy_reward_visitor v(db);

        // go to user11 cashout
        generate_blocks(db.head_block_time() + fc::seconds(delay));
        auto user11_parent_reward = check_comment_rewards(user11, v.stats[user11.name], 0);
        check_post_rewards(v.stats[user00.name], user11_parent_reward);
    }
}

BOOST_AUTO_TEST_CASE(check_multiple_children_each_level_on_same_block)
{
    /*
        user00 -> block1
            user10 -> block2
                user20 -> block3
                user21 -> block3
            user11 -> block2
    */

    uint32_t delay = 10 * SCORUM_BLOCK_INTERVAL;
    uint32_t blocks_after_post = 2;

    auto p = create_post(user00).in_block(delay);

    auto c10 = p.create_comment(user10).push();
    auto c11 = p.create_comment(user11).in_block(delay);

    auto c20 = c10.create_comment(user20).push();
    auto c21 = c10.create_comment(user21).in_block(SCORUM_REVERSE_AUCTION_WINDOW_SECONDS);

    p.vote(voter00).push();
    c10.vote(voter10).push();
    c11.vote(voter11).push();
    c20.vote(voter20).push();
    c21.vote(voter21).push();

    BOOST_TEST_MESSAGE("User00 cashout");
    {
        comments_hierarchy_reward_visitor v(db);

        // go to user00 cashout
        generate_blocks(db.head_block_time() + fc::seconds(SCORUM_CASHOUT_WINDOW_SECONDS)
                        - fc::seconds(blocks_after_post * delay) - SCORUM_REVERSE_AUCTION_WINDOW_SECONDS);
        check_post_rewards(v.stats[user00.name], 0);
    }
    BOOST_TEST_MESSAGE("User10 & User11 cashout");
    {
        comments_hierarchy_reward_visitor v(db);

        // go to user10 & user11 cashout
        generate_blocks(db.head_block_time() + fc::seconds(delay));
        auto user10_parent_reward = check_comment_rewards(user10, v.stats[user10.name], 0);
        auto user11_parent_reward = check_comment_rewards(user11, v.stats[user11.name], 0);
        check_post_rewards(v.stats[user00.name], user10_parent_reward + user11_parent_reward);
    }
    BOOST_TEST_MESSAGE("User20 & User21 cashout");
    {
        comments_hierarchy_reward_visitor v(db);

        // go to user20 & user21 cashout
        generate_blocks(db.head_block_time() + fc::seconds(delay));
        auto user20_parent_reward = check_comment_rewards(user20, v.stats[user20.name], 0);
        auto user21_parent_reward = check_comment_rewards(user21, v.stats[user21.name], 0);
        auto user10_parent_reward
            = check_comment_rewards(user10, v.stats[user10.name], user20_parent_reward + user21_parent_reward);
        check_post_rewards(v.stats[user00.name], user10_parent_reward);
    }
}

BOOST_AUTO_TEST_CASE(check_children_parents_mixing_blocks)
{
    /*
        user00 -> block1
            user10 -> block2
                user20 -> block2
                user21 -> block3
            user11 -> block3
    */

    uint32_t delay = 10 * SCORUM_BLOCK_INTERVAL;
    uint32_t blocks_after_post = 2;

    auto p = create_post(user00).in_block(delay);

    auto c10 = p.create_comment(user10).push();
    auto c20 = c10.create_comment(user20).in_block(delay);

    auto c21 = c10.create_comment(user21).push();
    auto c11 = p.create_comment(user11).in_block(SCORUM_REVERSE_AUCTION_WINDOW_SECONDS);

    p.vote(voter00).push();
    c10.vote(voter10).push();
    c11.vote(voter11).push();
    c20.vote(voter20).push();
    c21.vote(voter21).push();

    BOOST_TEST_MESSAGE("User00 cashout");
    {
        comments_hierarchy_reward_visitor v(db);

        // go to user00 cashout
        generate_blocks(db.head_block_time() + fc::seconds(SCORUM_CASHOUT_WINDOW_SECONDS)
                        - fc::seconds(blocks_after_post * delay) - SCORUM_REVERSE_AUCTION_WINDOW_SECONDS);
        check_post_rewards(v.stats[user00.name], 0);
    }
    BOOST_TEST_MESSAGE("User10 & User20 cashout");
    {
        comments_hierarchy_reward_visitor v(db);

        // go to user10 & user20 cashout
        generate_blocks(db.head_block_time() + fc::seconds(delay));
        auto user20_parent_reward = check_comment_rewards(user20, v.stats[user20.name], 0);
        auto user10_parent_reward = check_comment_rewards(user10, v.stats[user10.name], user20_parent_reward);
        check_post_rewards(v.stats[user00.name], user10_parent_reward);
    }
    BOOST_TEST_MESSAGE("User11 & User21 cashout");
    {
        comments_hierarchy_reward_visitor v(db);

        // go to user11 & user21 cashout
        generate_blocks(db.head_block_time() + fc::seconds(delay));
        auto user21_parent_reward = check_comment_rewards(user21, v.stats[user21.name], 0);
        auto user10_parent_reward = check_comment_rewards(user10, v.stats[user10.name], user21_parent_reward);
        auto user11_parent_reward = check_comment_rewards(user11, v.stats[user11.name], 0);
        check_post_rewards(v.stats[user00.name], user10_parent_reward + user11_parent_reward);
    }
}

BOOST_AUTO_TEST_CASE(check_beneficiares_rewards)
{
    /*
        user00 -> block1
            user10 -> block2 + benefaciary
                user20 -> block3
    */

    uint32_t benef_percent = 50;
    uint32_t delay = 10 * SCORUM_BLOCK_INTERVAL;

    Actor user10_benef = create_actor("user10benef");

    auto p = create_post(user00).in_block(delay);
    auto c10 = p.create_comment(user10).in_block(delay);
    auto c20 = c10.create_comment(user20).in_block(SCORUM_REVERSE_AUCTION_WINDOW_SECONDS);

    comment_options_operation op;
    op.author = c10.author();
    op.permlink = c10.permlink();
    op.allow_curation_rewards = true;
    comment_payout_beneficiaries b;
    b.beneficiaries.push_back(beneficiary_route_type(user10_benef.name, benef_percent * SCORUM_1_PERCENT));
    op.extensions.insert(b);
    signed_transaction tx;
    tx.operations.push_back(op);
    tx.set_expiration(db.head_block_time() + SCORUM_MIN_TRANSACTION_EXPIRATION_LIMIT);
    tx.sign(initdelegate.private_key, db.get_chain_id());
    db.push_transaction(tx, default_skip);

    p.vote(voter00).push();
    c10.vote(voter10).push();
    c20.vote(voter20).push();

    comments_hierarchy_reward_visitor v(db);

    BOOST_CHECK_EQUAL((v.stats[user10.name].author_reward + v.stats[user10.name].benefactor_reward) * benef_percent
                          / 100,
                      v.stats[user10.name].benefactor_reward);
}

BOOST_AUTO_TEST_CASE(check_comment_double_cashouts)
{
    /*
        user00 -> block1
            user10 -> block2
    */

    uint32_t delay = 10 * SCORUM_BLOCK_INTERVAL;
    uint32_t blocks_after_post = 1;

    auto p = create_post(user00).in_block(delay);
    auto c10 = p.create_comment(user10).in_block(SCORUM_REVERSE_AUCTION_WINDOW_SECONDS);

    p.vote(voter00).push();
    c10.vote(voter10).push();

    BOOST_TEST_MESSAGE("User00 cashout");
    {
        comments_hierarchy_reward_visitor v(db);

        // go to user00 cashout
        generate_blocks(db.head_block_time() + fc::seconds(SCORUM_CASHOUT_WINDOW_SECONDS)
                        - fc::seconds(blocks_after_post * delay) - SCORUM_REVERSE_AUCTION_WINDOW_SECONDS);
        check_post_rewards(v.stats[user00.name], 0);
    }
    auto user00_sp_after_user00_cashout = account_service.get_account(user00.name).scorumpower.amount;

    comments_hierarchy_reward_visitor v(db);

    BOOST_TEST_MESSAGE("User10 cashout");
    // go to user10 cashout
    generate_blocks(db.head_block_time() + fc::seconds(delay));
    auto user10_parent_reward = check_comment_rewards(user10, v.stats[user10.name], 0);
    check_post_rewards(v.stats[user00.name], user10_parent_reward);

    auto user00_sp_after_user10_cashout = account_service.get_account(user00.name).scorumpower.amount;

    BOOST_REQUIRE_EQUAL(user00_sp_after_user10_cashout - user00_sp_after_user00_cashout, user10_parent_reward);
}

BOOST_AUTO_TEST_CASE(check_accounts_balance)
{
    /*
        user00 -> block1
            user10 -> block2
                user20 -> block3
    */

    uint32_t delay = 10 * SCORUM_BLOCK_INTERVAL;
    uint32_t blocks_after_post = 2;

    auto user00_sp_before = account_service.get_account(user00.name).scorumpower.amount;
    auto user10_sp_before = account_service.get_account(user10.name).scorumpower.amount;
    auto user20_sp_before = account_service.get_account(user20.name).scorumpower.amount;

    auto p = create_post(user00).in_block(delay);
    auto c10 = p.create_comment(user10).in_block();
    auto c20 = c10.create_comment(user20).in_block(SCORUM_REVERSE_AUCTION_WINDOW_SECONDS);

    p.vote(voter00).in_block();
    c10.vote(voter10).in_block();
    c20.vote(voter20).in_block();

    comments_hierarchy_reward_visitor v(db);

    // go to user00 cashout
    generate_blocks(db.head_block_time() + fc::seconds(SCORUM_CASHOUT_WINDOW_SECONDS)
                    - fc::seconds(blocks_after_post * delay) - SCORUM_REVERSE_AUCTION_WINDOW_SECONDS);
    // go to user10 cashout
    generate_blocks(db.head_block_time() + fc::seconds(delay));
    // go to user20 cashout
    generate_blocks(db.head_block_time() + fc::seconds(delay));

    auto user00_sp_after = account_service.get_account(user00.name).scorumpower.amount;
    auto user10_sp_after = account_service.get_account(user10.name).scorumpower.amount;
    auto user20_sp_after = account_service.get_account(user20.name).scorumpower.amount;

    BOOST_CHECK_EQUAL(user00_sp_after - user00_sp_before, v.stats[user00.name].author_reward);
    BOOST_CHECK_EQUAL(user10_sp_after - user10_sp_before, v.stats[user10.name].author_reward);
    BOOST_CHECK_EQUAL(user20_sp_after - user20_sp_before, v.stats[user20.name].author_reward);
}

BOOST_AUTO_TEST_SUITE_END()
