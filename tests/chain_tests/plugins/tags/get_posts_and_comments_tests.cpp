#include <scorum/tags/tags_api_objects.hpp>
#include <boost/test/unit_test.hpp>

#include "tags_common.hpp"

using namespace scorum;
using namespace scorum::tags::api;
using namespace scorum::app;
using namespace scorum::tags;

BOOST_FIXTURE_TEST_SUITE(get_posts_and_comments_tests, database_fixture::tags_fixture)

SCORUM_TEST_CASE(check_get_posts_and_comments_contract_negative)
{
    discussion_query q;
    q.limit = 101;
    BOOST_REQUIRE_THROW(_api.get_posts_and_comments(q), fc::assert_exception);

    q.limit = 100;
    q.start_author = "x";
    BOOST_REQUIRE_THROW(_api.get_posts_and_comments(q), fc::assert_exception);

    q.start_author.reset();
    q.start_permlink = "x";
    BOOST_REQUIRE_THROW(_api.get_posts_and_comments(q), fc::assert_exception);
}

SCORUM_TEST_CASE(check_get_posts_and_comments_contract_positive)
{
    discussion_query q;
    q.limit = 100;
    BOOST_REQUIRE_NO_THROW(_api.get_posts_and_comments(q));

    q.start_author = "alice";
    q.start_permlink = "p";
    BOOST_REQUIRE_NO_THROW(_api.get_posts_and_comments(q));
}

SCORUM_TEST_CASE(check_both_posts_and_comments_returned)
{
    discussion_query q;
    q.limit = 100;
    BOOST_REQUIRE_EQUAL(_api.get_posts_and_comments(q).size(), 0u);

    auto p = create_post(alice).in_block();
    p.create_comment(bob).in_block();

    BOOST_REQUIRE_EQUAL(_api.get_posts_and_comments(q).size(), 2u);
}

SCORUM_TEST_CASE(check_truncate_body)
{
    auto p = create_post(alice).set_body("1234567").in_block();
    auto c = p.create_comment(bob).set_body("abcdefgh").in_block();

    discussion_query q;
    q.limit = 100;
    q.truncate_body = 5;
    auto discussions = _api.get_posts_and_comments(q);

    BOOST_REQUIRE_EQUAL(discussions.size(), 2u);
#ifdef IS_LOW_MEM
    BOOST_CHECK_EQUAL(discussions[0].body, "");
    BOOST_CHECK_EQUAL(discussions[1].body, "");
    BOOST_CHECK_EQUAL(discussions[0].body_length, 0u);
    BOOST_CHECK_EQUAL(discussions[1].body_length, 0u);
#else
    BOOST_CHECK_EQUAL(discussions[0].body, "12345");
    BOOST_CHECK_EQUAL(discussions[1].body, "abcde");
    BOOST_CHECK_EQUAL(discussions[0].body_length, 7u);
    BOOST_CHECK_EQUAL(discussions[1].body_length, 8u);
#endif
}

SCORUM_TEST_CASE(check_limit)
{
    auto p = create_post(alice).in_block();
    auto c1 = p.create_comment(bob).in_block();
    auto c2 = c1.create_comment(sam).in_block();

    discussion_query q;
    q.limit = 2;
    auto discussions = _api.get_posts_and_comments(q);

    BOOST_REQUIRE_EQUAL(discussions.size(), 2u);
    BOOST_CHECK_EQUAL(discussions[0].permlink, p.permlink());
    BOOST_CHECK_EQUAL(discussions[1].permlink, c1.permlink());
}

SCORUM_TEST_CASE(check_same_author_comments)
{
    auto p = create_post(alice).in_block_with_delay();
    auto c1 = p.create_comment(alice).in_block_with_delay();
    c1.create_comment(alice).in_block_with_delay();

    discussion_query q;
    q.limit = 100;
    auto discussions = _api.get_posts_and_comments(q);

    BOOST_REQUIRE_EQUAL(discussions.size(), 3u);
}

SCORUM_TEST_CASE(check_pagination)
{
    /*
     * Heirarchy:
     *
     * alice/p1
     * --bob/c1
     * ----sam/c2
     * ----alice/c3
     * sam/p2
     * --alice/c4
     * --bob/c5
     *
     * 'by_permlink' index will be sorted as follows:
     * 0. alice/c3
     * 1. alice/c4
     * 2. alice/p1
     * 3. bob/c1
     * 4. bob/c5
     * 5. sam/c2
     * 6. sam/p2
     */
    auto p1 = create_post(alice).set_permlink("p1").in_block_with_delay();
    auto c1 = p1.create_comment(bob).set_permlink("c1").in_block_with_delay();
    auto c2 = c1.create_comment(sam).set_permlink("c2").in_block_with_delay();
    auto c3 = c1.create_comment(alice).set_permlink("c3").in_block_with_delay();
    auto p2 = create_post(sam).set_permlink("p2").in_block_with_delay();
    auto c4 = p2.create_comment(alice).set_permlink("c4").in_block_with_delay();
    auto c5 = p2.create_comment(bob).set_permlink("c5").in_block_with_delay();

    discussion_query q;
    q.limit = 4;
    {
        auto discussions = _api.get_posts_and_comments(q);

        BOOST_REQUIRE_EQUAL(discussions.size(), 4u);
        BOOST_CHECK_EQUAL(discussions[0].permlink, c3.permlink());
        BOOST_CHECK_EQUAL(discussions[1].permlink, c4.permlink());
        BOOST_CHECK_EQUAL(discussions[2].permlink, p1.permlink());
        BOOST_CHECK_EQUAL(discussions[3].permlink, c1.permlink());
    }
    {
        q.start_author = p1.author();
        q.start_permlink = p1.permlink();
        q.limit = 3u;
        auto discussions = _api.get_posts_and_comments(q);

        BOOST_REQUIRE_EQUAL(discussions.size(), 3u);
        BOOST_CHECK_EQUAL(discussions[0].permlink, p1.permlink());
        BOOST_CHECK_EQUAL(discussions[1].permlink, c1.permlink());
        BOOST_CHECK_EQUAL(discussions[2].permlink, c5.permlink());
    }
    {
        q.start_author = c5.author();
        q.start_permlink = c5.permlink();
        q.limit = 4u;
        auto discussions = _api.get_posts_and_comments(q);

        BOOST_REQUIRE_EQUAL(discussions.size(), 3u);
        BOOST_CHECK_EQUAL(discussions[0].permlink, c5.permlink());
        BOOST_CHECK_EQUAL(discussions[1].permlink, c2.permlink());
        BOOST_CHECK_EQUAL(discussions[2].permlink, p2.permlink());
    }
}

BOOST_AUTO_TEST_SUITE_END()
