#include <boost/test/unit_test.hpp>

#include "database_blog_integration.hpp"

#include "actor.hpp"

#include <scorum/chain/services/comment.hpp>
#include <scorum/chain/services/comment_statistic.hpp>

#include <scorum/tags/tags_objects.hpp>
#include <scorum/tags/tags_api.hpp>
#include <scorum/tags/tags_plugin.hpp>

#include <boost/fusion/include/map.hpp>
#include <boost/fusion/include/at_key.hpp>

namespace rewards_blogging_statistic_tests {

using namespace scorum::chain;
using namespace scorum::protocol;
using namespace scorum::app;
using namespace scorum::tags;

using namespace database_fixture;

struct rewards_blogging_statistic_fixture : public database_blog_integration_fixture
{
    rewards_blogging_statistic_fixture()
        : alice("alice")
        , bob("bob")
        , sam("sam")
        , dave("dave")
        , andrew("andrew")
        , comments(db.comment_service())
        , statistic(db.comment_statistic_sp_service())
        , _api_ctx(app, TAGS_API_NAME, std::make_shared<api_session_data>())
        , tag_api(_api_ctx)
    {
        init_plugin<scorum::tags::tags_plugin>();

        open_database();

        create_actor(alice);
        create_actor(bob);
        create_actor(sam);
        create_actor(dave);
        create_actor(andrew);
    }

    void create_actor(const Actor& a)
    {
        actor(initdelegate).create_account(a);
        actor(initdelegate).give_sp(a, 1e5);
    }

    struct test_case_1
    {
        comment_op alice_post;
        comment_op sam_comment;
    };

    // test case #1:
    //--------------
    //  'Alice' create post with benificiar 'Bob' (30%)
    //  'Sam' create comment for 'Alice' post
    //  'Dave' vote for 'Alice' post
    //  'Andrew' vote for 'Sam' comment
    test_case_1 create_test_case_1()
    {
        auto p
            = create_post(alice)
                  .set_beneficiar(bob, 30)
                  .set_json(
                      R"({"domains": ["chain_tests"], "categories": ["rewards_blogging_statistic_tests"], "tags": ["test"]})")
                  .in_block();

        auto c = p.create_comment(sam).in_block();

        generate_blocks(db.head_block_time() + SCORUM_REVERSE_AUCTION_WINDOW_SECONDS);

        p.vote(dave).in_block();

        c.vote(andrew).in_block();

        generate_blocks(db.head_block_time() + SCORUM_CASHOUT_WINDOW_SECONDS);

        return { p, c };
    }

    Actor alice;
    Actor bob;
    Actor sam;
    Actor dave;
    Actor andrew;

    comment_service_i& comments;
    comment_statistic_sp_service_i& statistic;

    api_context _api_ctx;
    scorum::tags::tags_api tag_api;
};

BOOST_FIXTURE_TEST_SUITE(rewards_blogging_statistic_tests, rewards_blogging_statistic_fixture)

SCORUM_TEST_CASE(comment_object_statistic_check)
{
    auto test_case = create_test_case_1();

    const auto& alice_stat
        = statistic.get(comments.get(test_case.alice_post.author(), test_case.alice_post.permlink()).id);
    const auto& sam_stat
        = statistic.get(comments.get(test_case.sam_comment.author(), test_case.sam_comment.permlink()).id);

    SCORUM_MESSAGE("-- Check total payout");

    BOOST_CHECK_EQUAL(alice_stat.total_payout_value,
                      alice_stat.author_payout_value + alice_stat.curator_payout_value
                          + alice_stat.from_children_payout_value + alice_stat.beneficiary_payout_value);
    BOOST_CHECK_EQUAL(sam_stat.total_payout_value,
                      sam_stat.author_payout_value + sam_stat.curator_payout_value + sam_stat.from_children_payout_value
                          + sam_stat.beneficiary_payout_value);

    SCORUM_MESSAGE("-- Check parents payout");

    BOOST_CHECK_EQUAL(alice_stat.from_children_payout_value, sam_stat.to_parent_payout_value);
    BOOST_CHECK_EQUAL(alice_stat.to_parent_payout_value, ASSET_NULL_SP);
    BOOST_CHECK_EQUAL(sam_stat.from_children_payout_value, ASSET_NULL_SP);

    SCORUM_MESSAGE("-- Check beneficiary payout");

    BOOST_CHECK_NE(alice_stat.beneficiary_payout_value, ASSET_NULL_SP);
    BOOST_CHECK_EQUAL(sam_stat.beneficiary_payout_value, ASSET_NULL_SP);

    SCORUM_MESSAGE("-- Check curation payout");

    BOOST_CHECK_NE(alice_stat.curator_payout_value, ASSET_NULL_SP);
    BOOST_CHECK_NE(sam_stat.curator_payout_value, ASSET_NULL_SP);
}

// There tests check statistic for SP reward only (algorithm for SCR will be same)
//
struct comment_reward_operation_stats
{
    asset fund_reward = ASSET_NULL_SP;
    asset total_payout = ASSET_NULL_SP;
    asset author_payout = ASSET_NULL_SP;
    asset curators_payout = ASSET_NULL_SP;
    asset from_children_payout = ASSET_NULL_SP;
    asset to_parent_payout = ASSET_NULL_SP;
    asset beneficiaries_payout = ASSET_NULL_SP;
};

struct author_reward_operation_stats
{
    asset reward = ASSET_NULL_SP;
};

struct curation_reward_operation_stats
{
    asset reward = ASSET_NULL_SP;
};

struct comment_benefactor_reward_operation_stats
{
    asset reward = ASSET_NULL_SP;
};

SCORUM_TEST_CASE(virtual_operation_statistic_check)
{
    namespace bf = boost::fusion;
    // clang-format off
    bf::map<
            bf::pair<comment_reward_operation_stats,
                     std::map<std::string, comment_reward_operation_stats>>,
            bf::pair<author_reward_operation_stats,
                     std::map<std::string, author_reward_operation_stats>>,
            bf::pair<curation_reward_operation_stats,
                     std::map<std::string, curation_reward_operation_stats>>,
            bf::pair<comment_benefactor_reward_operation_stats,
                     std::map<std::string, comment_benefactor_reward_operation_stats>>
            > stats;

    db.post_apply_operation.connect([&](const operation_notification& note) {
        note.op.weak_visit(
            [&](const comment_reward_operation& op)
            {
                auto &stat = bf::at_key<comment_reward_operation_stats>(stats)[op.author];
                stat.fund_reward += op.fund_reward;
                stat.total_payout += op.total_payout;
                stat.author_payout += op.author_payout;
                stat.curators_payout += op.curators_payout;
                stat.from_children_payout += op.from_children_payout;
                stat.to_parent_payout += op.to_parent_payout;
                stat.beneficiaries_payout += op.beneficiaries_payout;
            },
            [&](const author_reward_operation& op)
            {
                auto &stat = bf::at_key<author_reward_operation_stats>(stats)[op.author];
                stat.reward += op.reward;
            },
            [&](const curation_reward_operation& op)
            {
                auto &stat = bf::at_key<curation_reward_operation_stats>(stats)[op.comment_author];
                stat.reward += op.reward;
            },
            [&](const comment_benefactor_reward_operation& op)
            {
                auto &stat = bf::at_key<comment_benefactor_reward_operation_stats>(stats)[op.author];
                stat.reward += op.reward;
            });
    });
    // clang-format on

    auto test_case = create_test_case_1();

    const auto& alice_stat
        = statistic.get(comments.get(test_case.alice_post.author(), test_case.alice_post.permlink()).id);
    const auto& sam_stat
        = statistic.get(comments.get(test_case.sam_comment.author(), test_case.sam_comment.permlink()).id);

    SCORUM_MESSAGE("-- Check comment_reward_operation");
    {
        const auto& alice_v_stat = bf::at_key<comment_reward_operation_stats>(stats)[alice];

        wdump((alice_v_stat));

        const auto& sam_v_stat = bf::at_key<comment_reward_operation_stats>(stats)[sam];

        wdump((sam_v_stat));

        BOOST_CHECK_EQUAL(alice_v_stat.total_payout, alice_stat.total_payout_value);
        BOOST_CHECK_EQUAL(alice_v_stat.author_payout, alice_stat.author_payout_value);
        BOOST_CHECK_EQUAL(alice_v_stat.curators_payout, alice_stat.curator_payout_value);
        BOOST_CHECK_EQUAL(alice_v_stat.beneficiaries_payout, alice_stat.beneficiary_payout_value);
        BOOST_CHECK_EQUAL(alice_v_stat.fund_reward, alice_stat.fund_reward_value);
        BOOST_CHECK_EQUAL(alice_v_stat.from_children_payout, alice_stat.from_children_payout_value);
        BOOST_CHECK_EQUAL(alice_v_stat.to_parent_payout, alice_stat.to_parent_payout_value);

        BOOST_CHECK_EQUAL(sam_v_stat.total_payout, sam_stat.total_payout_value);
        BOOST_CHECK_EQUAL(sam_v_stat.author_payout, sam_stat.author_payout_value);
        BOOST_CHECK_EQUAL(sam_v_stat.curators_payout, sam_stat.curator_payout_value);
        BOOST_CHECK_EQUAL(sam_v_stat.beneficiaries_payout, sam_stat.beneficiary_payout_value);
        BOOST_CHECK_EQUAL(sam_v_stat.fund_reward, sam_stat.fund_reward_value);
        BOOST_CHECK_EQUAL(sam_v_stat.from_children_payout, sam_stat.from_children_payout_value);
        BOOST_CHECK_EQUAL(sam_v_stat.to_parent_payout, sam_stat.to_parent_payout_value);
    }

    SCORUM_MESSAGE("-- Check author_reward_operation");
    {
        const auto& alice_v_stat = bf::at_key<author_reward_operation_stats>(stats)[alice];

        wdump((alice_v_stat));

        const auto& sam_v_stat = bf::at_key<author_reward_operation_stats>(stats)[sam];

        wdump((sam_v_stat));

        BOOST_CHECK_EQUAL(alice_v_stat.reward, alice_stat.author_payout_value + alice_stat.from_children_payout_value);
        BOOST_CHECK_EQUAL(sam_v_stat.reward, sam_stat.author_payout_value + sam_stat.from_children_payout_value);
    }

    SCORUM_MESSAGE("-- Check curation_reward_operation");
    {
        const auto& alice_v_stat = bf::at_key<curation_reward_operation_stats>(stats)[alice];

        wdump((alice_v_stat));

        const auto& sam_v_stat = bf::at_key<curation_reward_operation_stats>(stats)[sam];

        wdump((sam_v_stat));

        BOOST_CHECK_EQUAL(alice_v_stat.reward, alice_stat.curator_payout_value);
        BOOST_CHECK_EQUAL(sam_v_stat.reward, sam_stat.curator_payout_value);
    }

    SCORUM_MESSAGE("-- Check comment_benefactor_reward_operation");
    {
        const auto& alice_v_stat = bf::at_key<comment_benefactor_reward_operation_stats>(stats)[alice];

        wdump((alice_v_stat));

        const auto& sam_v_stat = bf::at_key<comment_benefactor_reward_operation_stats>(stats)[sam];

        wdump((sam_v_stat));

        BOOST_CHECK_EQUAL(alice_v_stat.reward, alice_stat.beneficiary_payout_value);
        BOOST_CHECK_EQUAL(sam_v_stat.reward, sam_stat.beneficiary_payout_value);
    }
}

SCORUM_TEST_CASE(tag_discussion_statistic_check)
{
    auto test_case = create_test_case_1();

    const auto& alice_stat
        = statistic.get(comments.get(test_case.alice_post.author(), test_case.alice_post.permlink()).id);
    const auto& sam_stat
        = statistic.get(comments.get(test_case.sam_comment.author(), test_case.sam_comment.permlink()).id);

    auto alice_discussion = tag_api.get_content(test_case.alice_post.author(), test_case.alice_post.permlink());
    auto comments = tag_api.get_comments(test_case.alice_post.author(), test_case.alice_post.permlink());
    BOOST_REQUIRE(!comments.empty());
    auto sam_discussion = comments[0];

    wdump((alice_discussion));

    BOOST_CHECK_EQUAL(alice_discussion.total_payout_sp_value, alice_stat.total_payout_value);
    BOOST_CHECK_EQUAL(alice_discussion.author_payout_sp_value, alice_stat.author_payout_value);
    BOOST_CHECK_EQUAL(alice_discussion.curator_payout_sp_value, alice_stat.curator_payout_value);
    BOOST_CHECK_EQUAL(alice_discussion.beneficiary_payout_sp_value, alice_stat.beneficiary_payout_value);
    BOOST_CHECK_EQUAL(alice_discussion.from_children_payout_sp_value, alice_stat.from_children_payout_value);
    BOOST_CHECK_EQUAL(alice_discussion.to_parent_payout_sp_value, alice_stat.to_parent_payout_value);

    wdump((sam_discussion));

    BOOST_CHECK_EQUAL(sam_discussion.total_payout_sp_value, sam_stat.total_payout_value);
    BOOST_CHECK_EQUAL(sam_discussion.author_payout_sp_value, sam_stat.author_payout_value);
    BOOST_CHECK_EQUAL(sam_discussion.curator_payout_sp_value, sam_stat.curator_payout_value);
    BOOST_CHECK_EQUAL(sam_discussion.beneficiary_payout_sp_value, sam_stat.beneficiary_payout_value);
    BOOST_CHECK_EQUAL(sam_discussion.from_children_payout_sp_value, sam_stat.from_children_payout_value);
    BOOST_CHECK_EQUAL(sam_discussion.to_parent_payout_sp_value, sam_stat.to_parent_payout_value);
}

#ifndef IS_LOW_MEM
SCORUM_TEST_CASE(tag_total_statistic_check)
{
    auto test_case = create_test_case_1();

    const auto& alice_stat
        = statistic.get(comments.get(test_case.alice_post.author(), test_case.alice_post.permlink()).id);

    auto posts = tag_api.get_trending_tags("test", 2);
    BOOST_REQUIRE(!posts.empty());
    auto alice_tag_stat = posts[0];

    BOOST_CHECK_EQUAL(alice_tag_stat.total_payouts_sp, alice_stat.total_payout_value);
}
#endif //! IS_LOW_MEM
BOOST_AUTO_TEST_SUITE_END()
}

// reflecton for wdump
FC_REFLECT(rewards_blogging_statistic_tests::rewards_blogging_statistic_tests::comment_reward_operation_stats,
           (fund_reward)(total_payout)(author_payout)(curators_payout)(from_children_payout)(to_parent_payout)(
               beneficiaries_payout))
FC_REFLECT(rewards_blogging_statistic_tests::rewards_blogging_statistic_tests::author_reward_operation_stats, (reward))
FC_REFLECT(rewards_blogging_statistic_tests::rewards_blogging_statistic_tests::curation_reward_operation_stats,
           (reward))
FC_REFLECT(
    rewards_blogging_statistic_tests::rewards_blogging_statistic_tests::comment_benefactor_reward_operation_stats,
    (reward))
