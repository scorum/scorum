#ifndef IS_LOW_MEM

#include "tags_common.hpp"
#include <scorum/tags/tags_api_objects.hpp>
#include <scorum/tags/tags_api.hpp>
#include <scorum/common_api/config.hpp>
#include <boost/test/unit_test.hpp>

using namespace scorum;
using namespace scorum::tags::api;
using namespace scorum::app;
using namespace scorum::tags;

namespace database_fixture {

struct get_discussions_by_query_fixture : public tags_fixture
{
    using discussion = scorum::tags::api::discussion;

    get_discussions_by_query_fixture()
    {
        actor(initdelegate).give_sp(alice, 1e9);
        actor(initdelegate).give_sp(bob, 1e9);
        actor(initdelegate).give_sp(sam, 1e9);
        actor(initdelegate).give_sp(dave, 1e9);
    }
};
}

BOOST_FIXTURE_TEST_SUITE(get_discussions_by_trending_tests, database_fixture::get_discussions_by_query_fixture)

SCORUM_TEST_CASE(no_votes_should_return_nothing)
{
    create_post(alice).set_json(R"({"domains": ["com"], "categories": ["cat"], "tags":["A","B","C"]})").in_block();

    discussion_query q;
    q.limit = 100;
    q.tags_logical_and = true;
    q.tags = { "B", "C" };
    std::vector<discussion> discussions = _api.get_discussions_by_trending(q);

    BOOST_REQUIRE_EQUAL(discussions.size(), 0u);
}

SCORUM_TEST_CASE(no_requested_tag_should_return_nothing)
{
    auto p1 = create_post(alice)
                  .set_json(R"({"domains": ["com"], "categories": ["cat"], "tags":["A","B","C"]})")
                  .in_block();
    p1.vote(alice).in_block();

    discussion_query q;
    q.limit = 100;
    q.tags_logical_and = true;
    q.tags = { "D" };
    std::vector<discussion> discussions = _api.get_discussions_by_trending(q);

    BOOST_REQUIRE_EQUAL(discussions.size(), 0u);
}

SCORUM_TEST_CASE(no_category_and_domain_should_return_post)
{
    auto p1 = create_post(alice).set_json(R"({"tags":["A"]})").in_block();
    p1.vote(alice).in_block().in_block();

    discussion_query q;
    q.limit = 100;
    q.tags_logical_and = true;
    q.tags = { "A" };

    BOOST_REQUIRE_EQUAL(_api.get_discussions_by_trending(q).size(), 1u);
}

SCORUM_TEST_CASE(no_json_metadata_should_return_post)
{
    auto p1 = create_post(alice).in_block();
    p1.vote(alice).in_block().in_block();

    discussion_query q;
    q.limit = 100;

    BOOST_REQUIRE_EQUAL(_api.get_discussions_by_trending(q).size(), 1u);
}

SCORUM_TEST_CASE(should_return_post_by_converting_all_to_lowercase)
{
    auto p1 = create_post(alice).set_json(R"({"categories":["A"], "domains": ["b"]})").in_block();
    p1.vote(alice).in_block().in_block();

    discussion_query q;
    q.limit = 100;
    q.tags_logical_and = true;
    q.tags = { "a", "B" };

    BOOST_REQUIRE_EQUAL(_api.get_discussions_by_trending(q).size(), 1u);
}

SCORUM_TEST_CASE(should_return_voted_tags_intersection)
{
    auto p1 = create_post(alice)
                  .set_json(R"({"domains": ["com"], "categories": ["cat"], "tags":["A","B","C"]})")
                  .in_block();
    auto p2
        = create_post(bob).set_json(R"({"domains": ["com"], "categories": ["cat"], "tags":["C","D","E"]})").in_block();
    auto p3 = create_post(sam)
                  .set_json(R"({"domains": ["com"], "categories": ["cat"], "tags":["B","C","D","E"]})")
                  .in_block();

    p1.vote(sam).in_block();

    p2.vote(sam).in_block();
    p2.vote(bob).in_block();

    discussion_query q;
    q.limit = 100;
    q.tags_logical_and = true;
    q.tags = { "B", "C" };
    {
        std::vector<discussion> discussions = _api.get_discussions_by_trending(q);

        BOOST_REQUIRE_EQUAL(discussions.size(), 1u);
        BOOST_REQUIRE_EQUAL(discussions[0].permlink, p1.permlink());
    }

    p3.vote(sam).in_block();
    p3.vote(bob).in_block();
    p3.vote(alice).in_block();

    {
        std::vector<discussion> discussions = _api.get_discussions_by_trending(q);

        BOOST_REQUIRE_EQUAL(discussions.size(), 2u);
        BOOST_REQUIRE_EQUAL(discussions[0].permlink, p3.permlink());
        BOOST_REQUIRE_EQUAL(discussions[1].permlink, p1.permlink());
    }
}

SCORUM_TEST_CASE(should_return_tags_intersection_contains_same_tags_in_diff_case)
{
    auto p1 = create_post(alice)
                  .set_json(R"({"domains": ["com"], "categories": ["cat"], "tags":["РУС1","рус1","Рус2","рус2"]})")
                  .in_block();
    auto p2 = create_post(bob)
                  .set_json(R"({"domains": ["com"], "categories": ["cat"], "tags":["Рус1","РУС1"]})")
                  .in_block();

    p1.vote(sam).in_block();

    p2.vote(sam).in_block();
    p2.vote(bob).in_block();

    discussion_query q;
    q.limit = 100;
    q.tags_logical_and = true;
    q.tags = { "рус1", "рУс1" };

    BOOST_REQUIRE_EQUAL(_api.get_discussions_by_trending(q).size(), 2u);

    q.tags = { "рус2", "рУс2" };
    BOOST_REQUIRE_EQUAL(_api.get_discussions_by_trending(q).size(), 1u);
}

SCORUM_TEST_CASE(should_return_voted_tags_union)
{
    auto p1 = create_post(alice)
                  .set_json(R"({"domains": ["com"], "categories": ["cat"], "tags":["A","B","C"]})")
                  .in_block();
    auto p2
        = create_post(bob).set_json(R"({"domains": ["com"], "categories": ["cat"], "tags":["C","D","E"]})").in_block();
    // this post (p3) will be skipped (despite it has max trending) cuz it doesn't have neither "B" or "D" tag
    auto p3 = create_post(sam).set_json(R"({"domains": ["com"], "categories": ["cat"], "tags":["C","E"]})").in_block();

    p1.vote(sam).in_block();

    p2.vote(sam).in_block();
    p2.vote(bob).in_block();

    p3.vote(sam).in_block();
    p3.vote(bob).in_block();
    p3.vote(alice).in_block();

    discussion_query q;
    q.limit = 100;
    q.tags_logical_and = false;
    q.tags = { "A", "D" };

    std::vector<discussion> discussions = _api.get_discussions_by_trending(q);

    BOOST_REQUIRE_EQUAL(discussions.size(), 2u);
    BOOST_REQUIRE_EQUAL(discussions[0].permlink, p2.permlink());
    BOOST_REQUIRE_EQUAL(discussions[1].permlink, p1.permlink());
}

SCORUM_TEST_CASE(check_pagination)
{
    auto p1 = create_post(alice)
                  .set_json(R"({"domains": ["com"], "categories": ["cat"], "tags":["A","B","C"]})")
                  .in_block_with_delay();
    // this post (p2) will be skipped cuz it doesn't have "C" tag
    auto p2 = create_post(bob)
                  .set_json(R"({"domains": ["com"], "categories": ["cat"], "tags":["D","E"]})")
                  .in_block_with_delay();
    auto p3 = create_post(alice)
                  .set_json(R"({"domains": ["com"], "categories": ["cat"], "tags":["B","C","D","E"]})")
                  .in_block();
    auto p4
        = create_post(bob).set_json(R"({"domains": ["com"], "categories": ["cat"], "tags":["C","B","E"]})").in_block();

    p1.vote(sam).in_block();

    p2.vote(sam).in_block();
    p2.vote(bob).in_block();

    p3.vote(sam).in_block();
    p3.vote(bob).in_block();
    p3.vote(alice).in_block();

    p4.vote(bob).in_block();
    p4.vote(sam).in_block();

    /*
     * [post3; post4]
     *             _______ start_autor/start_permlink for the 2nd call
     *           /
     *        [post4; post1]
     */
    discussion_query q;
    q.limit = 2;
    q.tags_logical_and = true;
    q.tags = { "C", "B" };
    {
        std::vector<discussion> discussions = _api.get_discussions_by_trending(q);

        BOOST_REQUIRE_EQUAL(discussions.size(), 2u);
        BOOST_REQUIRE_EQUAL(discussions[0].permlink, p3.permlink());
        BOOST_REQUIRE_EQUAL(discussions[1].permlink, p4.permlink());

        q.start_author = discussions[1].author;
        q.start_permlink = discussions[1].permlink;
    }

    {
        std::vector<discussion> discussions = _api.get_discussions_by_trending(q);

        BOOST_REQUIRE_EQUAL(discussions.size(), 2u);
        BOOST_REQUIRE_EQUAL(discussions[0].permlink, p4.permlink());
        BOOST_REQUIRE_EQUAL(discussions[1].permlink, p1.permlink());
    }
}

SCORUM_TEST_CASE(check_only_first_8_tags_are_analized)
{
    // G-K are ignored (see ::scorum::TAGS_TO_ANALIZE_COUNT)
    auto p1
        = create_post(alice)
              .set_json(
                  R"({"domains": ["com"], "categories": ["cat"], "tags":["A","B","C","D","E","F","G","H","I","J","K"]})")
              .in_block();
    auto p2 = create_post(bob).set_json(R"({"domains": ["com"], "categories": ["cat"], "tags":["F","G"]})").in_block();

    p1.vote(sam).in_block();

    p2.vote(sam).in_block();
    p2.vote(bob).in_block();

    discussion_query q;
    q.limit = 100;
    q.tags_logical_and = true;
    q.tags = { "G" };
    {
        std::vector<discussion> discussions = _api.get_discussions_by_trending(q);

        BOOST_REQUIRE_EQUAL(discussions.size(), 1u);
        BOOST_REQUIRE_EQUAL(discussions[0].permlink, p2.permlink());
    }
    {
        q.tags = { "F" };
        std::vector<discussion> discussions = _api.get_discussions_by_trending(q);

        BOOST_REQUIRE_EQUAL(discussions.size(), 2u);
    }
}

SCORUM_TEST_CASE(check_truncate_body)
{
    auto p1 = create_post(bob)
                  .set_body("1234567890")
                  .set_json(R"({"domains": ["com"], "categories": ["cat"], "tags":["I"]})")
                  .in_block();

    p1.vote(sam).in_block();

    discussion_query q;
    q.limit = 100;
    q.tags_logical_and = true;
    q.tags = { "I" };
    q.truncate_body = 5;
    std::vector<discussion> discussions = _api.get_discussions_by_trending(q);

    BOOST_REQUIRE_EQUAL(discussions.size(), 1u);
    BOOST_REQUIRE_EQUAL(discussions[0].body.size(), q.truncate_body);
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_FIXTURE_TEST_SUITE(get_discussions_by_created_tests, database_fixture::get_discussions_by_query_fixture)

SCORUM_TEST_CASE(no_votes_should_return_union)
{
    auto p1 = create_post(alice).set_json(R"({"domains": ["com"], "categories": ["cat"], "tags":["A"]})").in_block();
    auto p2 = create_post(bob).set_json(R"({"domains": ["com"], "categories": ["cat"], "tags":["B"]})").in_block();

    discussion_query q;
    q.limit = 100;
    q.tags_logical_and = false;
    q.tags = { "A", "B", "C" };
    std::vector<discussion> discussions = _api.get_discussions_by_created(q);

    BOOST_REQUIRE_EQUAL(discussions.size(), 2u);
    BOOST_REQUIRE_EQUAL(discussions[0].permlink, p2.permlink());
    BOOST_REQUIRE_EQUAL(discussions[1].permlink, p1.permlink());
}

SCORUM_TEST_CASE(should_return_two_posts_from_same_block_ordered_by_id_desc)
{
    auto p1 = create_post(alice).set_json(R"({"domains": ["com"], "categories": ["cat"], "tags":["B"]})").push();
    auto p2 = create_post(bob).set_json(R"({"domains": ["com"], "categories": ["cat"], "tags":["A"]})").in_block();

    discussion_query q;
    q.limit = 100;
    q.tags_logical_and = false;
    q.tags = { "A", "B" };
    std::vector<discussion> discussions = _api.get_discussions_by_created(q);

    BOOST_REQUIRE_EQUAL(discussions.size(), 2u);
    BOOST_REQUIRE_EQUAL(discussions[0].permlink, p2.permlink());
    BOOST_REQUIRE_EQUAL(discussions[1].permlink, p1.permlink());
}

SCORUM_TEST_CASE(check_tag_should_be_truncated_to_24symbols)
{
    auto json
        = R"({"domains": ["d"], "categories": ["c"], "tags":["手手手手手田田田田田手手手手手田田田田田手手手手手"]})";
    auto p1 = create_post(alice).set_json(json).in_block();

    discussion_query q;
    q.limit = 100;
    q.tags_logical_and = false;

    q.tags = { "手手手手手田田田田田手手手手手田田田田田手手手手手田田田田田" }; // 30 symbols
    std::vector<discussion> discussions = _api.get_discussions_by_created(q);
    BOOST_REQUIRE_EQUAL(discussions.size(), 1u);
    BOOST_CHECK_EQUAL(discussions[0].json_metadata, json);

    q.tags = { "手手手手手田田田田田手手手手手田田田田田手手手手" }; // 24 symbols
    BOOST_CHECK_EQUAL(_api.get_discussions_by_created(q).size(), 1u);

    q.tags = { "手手手手手田田田田田手手手手手田田田田田手手手" }; // 23 symbols
    BOOST_CHECK_EQUAL(_api.get_discussions_by_created(q).size(), 0u);
}

SCORUM_TEST_CASE(check_domain_should_be_truncated_to_24symbols)
{
    // mixed russian and english which means that size of strings in bytes is not a multiple of theirs length in symbols
    auto json = R"({"domains": ["domaiдоменdomaidomaiдомен"], "categories": ["c"], "tags":["t"]})";
    auto p1 = create_post(alice).set_json(json).in_block();

    discussion_query q;
    q.limit = 100;
    q.tags_logical_and = false;

    q.tags = { "domaiдоменdomaidomaiдомен" }; // 25 symbols
    BOOST_CHECK_EQUAL(_api.get_discussions_by_created(q).size(), 1u);

    q.tags = { "domaiдоменdomaidomaiдоме" }; // 24 symbols
    BOOST_CHECK_EQUAL(_api.get_discussions_by_created(q).size(), 1u);

    q.tags = { "domaiдоменdomaidomaiдом" }; // 23 symbols
    BOOST_CHECK_EQUAL(_api.get_discussions_by_created(q).size(), 0u);
}

SCORUM_TEST_CASE(check_category_should_be_truncated_to_24symbols)
{
    auto json = R"({"domains": ["d"], "categories": ["categcategcategcategcateg"], "tags":["t"]})";
    auto p1 = create_post(alice).set_json(json).in_block();

    discussion_query q;
    q.limit = 100;
    q.tags_logical_and = false;

    q.tags = { "categcategcategcategcateg" }; // 25 symbols
    BOOST_CHECK_EQUAL(_api.get_discussions_by_created(q).size(), 1u);

    q.tags = { "categcategcategcategcate" }; // 24 symbols
    BOOST_CHECK_EQUAL(_api.get_discussions_by_created(q).size(), 1u);

    q.tags = { "categcategcategcategcat" }; // 23 symbols
    BOOST_CHECK_EQUAL(_api.get_discussions_by_created(q).size(), 0u);
}

SCORUM_TEST_CASE(check_comments_should_not_be_returned)
{
    auto p1
        = create_post(alice).set_json(R"({"domains": ["com"], "categories": ["cat"], "tags":["A", "D"]})").in_block();
    // comments creation shouldn't be monitored by tags_plugin
    auto c1
        = p1.create_comment(bob).set_json(R"({"domains": ["com"], "categories": ["cat"], "tags":["A"]})").in_block();
    auto p2 = create_post(sam)
                  .set_json(R"({"domains": ["com"], "categories": ["cat"], "tags":["B","C"]})")
                  .in_block_with_delay();

    // comments changing shoudn't be monitored by tags_plugin
    p1.create_comment(bob)
        .set_permlink(c1.permlink())
        .set_body("new-body")
        .set_json(R"({"domains": ["com"], "categories": ["cat"], "tags":["A"]})")
        .in_block();
    // comments voting shoudn't be monitored by tags_plugin
    c1.vote(alice).in_block();
    // comments payouts shoudn't be monitored by tags_plugin
    generate_blocks(db.head_block_time() + SCORUM_CASHOUT_WINDOW_SECONDS);

    discussion_query q;
    q.limit = 100;
    q.tags_logical_and = false;
    q.tags = { "A", "B", "C", "D" };
    std::vector<discussion> discussions = _api.get_discussions_by_created(q);

    BOOST_REQUIRE_EQUAL(discussions.size(), 2u);
    BOOST_REQUIRE_EQUAL(discussions[0].permlink, p2.permlink());
    BOOST_REQUIRE_EQUAL(discussions[1].permlink, p1.permlink());
}

SCORUM_TEST_CASE(check_discussions_after_post_deleting)
{
    auto p1 = create_post(alice)
                  .set_json(R"({"domains": ["com"], "categories": ["cat"], "tags":["A","B","C"]})")
                  .in_block_with_delay();
    auto p2
        = create_post(bob).set_json(R"({"domains": ["com"], "categories": ["cat"], "tags":["B","C","D"]})").in_block();
    auto p3 = create_post(alice)
                  .set_json(R"({"domains": ["com"], "categories": ["cat"], "tags":["B","C","D","E"]})")
                  .in_block();

    discussion_query q;
    q.limit = 100;
    q.tags_logical_and = true;
    q.tags = { "B", "C" };
    {
        std::vector<discussion> discussions = _api.get_discussions_by_created(q);

        BOOST_REQUIRE_EQUAL(discussions.size(), 3u);
    }

    delete_comment_operation op;
    op.author = p2.author();
    op.permlink = p2.permlink();
    push_operation(op, alice.private_key);

    {
        std::vector<discussion> discussions = _api.get_discussions_by_created(q);

        BOOST_REQUIRE_EQUAL(discussions.size(), 2u);
        BOOST_REQUIRE_EQUAL(discussions[0].permlink, p3.permlink());
        BOOST_REQUIRE_EQUAL(discussions[1].permlink, p1.permlink());
    }
}

SCORUM_TEST_CASE(check_active_votes_if_comment_was_voted_with_negative_weight)
{
    auto p1 = create_post(alice).set_json(R"({"domains": ["com"], "categories": ["cat"], "tags":["A"]})").in_block();

    p1.vote(sam, -100).in_block();

    discussion_query q;
    q.limit = 100;
    q.tags_logical_and = true;

    auto discussions = _api.get_discussions_by_created(q);
    BOOST_REQUIRE_EQUAL(discussions.size(), 1u);
    BOOST_REQUIRE_EQUAL(discussions[0].active_votes.size(), 1u);
    BOOST_REQUIRE_EQUAL(discussions[0].active_votes[0].percent, -100 * SCORUM_1_PERCENT);
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_FIXTURE_TEST_SUITE(get_discussions_by_hot_tests, database_fixture::get_discussions_by_query_fixture)

SCORUM_TEST_CASE(should_return_voted_tags_union)
{
    auto p1 = create_post(alice)
                  .set_json(R"({"domains": ["com"], "categories": ["cat"], "tags":["A","B","C"]})")
                  .in_block_with_delay();
    auto p2
        = create_post(bob).set_json(R"({"domains": ["com"], "categories": ["cat"], "tags":["C","D","E"]})").in_block();
    auto p3 = create_post(alice)
                  .set_json(R"({"domains": ["com"], "categories": ["cat"], "tags":["B","C","D","E"]})")
                  .in_block();

    p1.vote(sam).in_block();

    p2.vote(sam).in_block();
    p2.vote(bob).in_block();

    discussion_query q;
    q.limit = 100;
    q.tags_logical_and = true;
    q.tags = { "B", "C" };
    {
        std::vector<discussion> discussions = _api.get_discussions_by_hot(q);

        BOOST_REQUIRE_EQUAL(discussions.size(), 1u);
        BOOST_REQUIRE_EQUAL(discussions[0].permlink, p1.permlink());
    }

    p3.vote(sam).in_block();
    p3.vote(bob).in_block();
    p3.vote(alice).in_block();

    {
        std::vector<discussion> discussions = _api.get_discussions_by_trending(q);

        BOOST_REQUIRE_EQUAL(discussions.size(), 2u);
        BOOST_REQUIRE_EQUAL(discussions[0].permlink, p3.permlink());
        BOOST_REQUIRE_EQUAL(discussions[1].permlink, p1.permlink());
    }
}

SCORUM_TEST_CASE(should_return_all_posts_both_with_and_without_tags)
{
    auto p1 = create_post(alice)
                  .set_json(R"({"domains": ["com"], "categories": ["cat"], "tags":["A"]})")
                  .in_block_with_delay();
    auto p2 = create_post(bob).set_json(R"({"domains": ["com"], "categories": ["cat"]})").in_block();
    auto p3 = create_post(alice).set_json(R"({"domains": ["com"], "categories": ["cat"], "tags":["B"]})").in_block();

    p1.vote(sam).in_block();

    p2.vote(sam).in_block();
    p2.vote(bob).in_block();

    p3.vote(sam).in_block();
    p3.vote(bob).in_block();
    p3.vote(alice).in_block();

    discussion_query q;
    q.limit = 100;
    q.tags_logical_and = true;

    BOOST_REQUIRE_EQUAL(_api.get_discussions_by_hot(q).size(), 3u);
}

SCORUM_TEST_CASE(should_return_posts_even_after_cashout)
{
    auto p1 = create_post(alice).set_json(R"({"domains": ["com"], "categories": ["cat"]})").in_block();

    p1.vote(sam).push();

    generate_blocks(db.head_block_time() + SCORUM_CASHOUT_WINDOW_SECONDS);

    discussion_query q;
    q.limit = 100;
    q.tags_logical_and = true;

    BOOST_REQUIRE_EQUAL(_api.get_discussions_by_hot(q).size(), 1u);
}

BOOST_AUTO_TEST_SUITE_END()

#endif
