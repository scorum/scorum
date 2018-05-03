#include <boost/test/unit_test.hpp>

#include "tags_common.hpp"

namespace tags_tests {

BOOST_FIXTURE_TEST_SUITE(get_comments, tags_fixture)

SCORUM_TEST_CASE(no_children)
{
    auto post = create_post(initdelegate, [](comment_operation& op) {
        op.title = "post 1";
        op.body = "post";
    });

    const auto discussions = _api.get_comments(post.author(), post.permlink());

    BOOST_REQUIRE_EQUAL(0u, discussions.size());
}

SCORUM_TEST_CASE(one_child)
{
    auto post = create_post(initdelegate, [](comment_operation& op) {
        op.title = "post 1";
        op.body = "post";
    });

    auto comment_level_1 = post.create_comment(initdelegate, [](comment_operation& op) {
        op.title = "level 1";
        op.body = "body";
    });

    const auto discussions = _api.get_comments(post.author(), post.permlink());

    BOOST_REQUIRE_EQUAL(1u, discussions.size());
}

SCORUM_TEST_CASE(two_children)
{
    auto post = create_post(initdelegate, [](comment_operation& op) {
        op.title = "post 1";
        op.body = "post";
    });

    auto comment_level_1 = post.create_comment(initdelegate, [](comment_operation& op) {
        op.title = "level 1";
        op.body = "body";
    });

    auto comment_level_2 = post.create_comment(initdelegate, [](comment_operation& op) {
        op.title = "level 2";
        op.body = "body";
    });

    const auto discussions = _api.get_comments(post.author(), post.permlink());

    BOOST_REQUIRE_EQUAL(2u, discussions.size());
}

SCORUM_TEST_CASE(no_post_children)
{
    auto post = create_post(initdelegate, [](comment_operation& op) {
        op.title = "post 1";
        op.body = "post";
    });

    auto post2 = create_post(initdelegate, [](comment_operation& op) {
        op.title = "post 2";
        op.body = "post";
        op.json_metadata = "{\"tags\" : [\"football\"]}";
    });

    auto comment_level_1 = post2.create_comment(initdelegate, [](comment_operation& op) {
        op.title = "level 1";
        op.body = "body";
    });

    const auto discussions = _api.get_comments(post.author(), post.permlink());

    BOOST_REQUIRE_EQUAL(0u, discussions.size());
}

SCORUM_TEST_CASE(check_order)
{
    /**
                   1
                 /   \
                2     4
              /   \   |
              3   6   5
                      |
                      7
         Expected order:
              2 3 6 4 5 7
    */

    const std::vector<std::string> expected_order = { "c2", "c3", "c6", "c4", "c5", "c7" };

    auto c1 = create_post(initdelegate, [](comment_operation& op) {
        op.title = "c1";
        op.body = "post";
    });

    auto c2 = c1.create_comment(initdelegate, [](comment_operation& op) {
        op.title = "c2";
        op.body = "body";
    });

    auto c3 = c2.create_comment(initdelegate, [](comment_operation& op) {
        op.title = "c3";
        op.body = "body";
    });

    auto c4 = c1.create_comment(initdelegate, [](comment_operation& op) {
        op.title = "c4";
        op.body = "body";
    });

    auto c5 = c4.create_comment(initdelegate, [](comment_operation& op) {
        op.title = "c5";
        op.body = "body";
    });

    auto c6 = c2.create_comment(initdelegate, [](comment_operation& op) {
        op.title = "c6";
        op.body = "body";
    });

    auto c7 = c5.create_comment(initdelegate, [](comment_operation& op) {
        op.title = "c7";
        op.body = "body";
    });

    const auto discussions = _api.get_comments(c1.author(), c1.permlink());

    BOOST_REQUIRE_EQUAL(6u, discussions.size());

    for (size_t i = 0; i < discussions.size(); ++i)
    {
        BOOST_CHECK_EQUAL(discussions[i].permlink, expected_order[i]);
    }
}

SCORUM_TEST_CASE(filter_with_depth)
{
    auto c1 = create_post(initdelegate, [](comment_operation& op) {
        op.title = "post";
        op.body = "post";
    });

    auto parent = c1;

    for (size_t i = 0; i < SCORUM_MAX_COMMENT_DEPTH; i++)
    {
        parent = parent.create_comment(initdelegate, [&](comment_operation& op) {
            op.title = "c" + std::to_string(i);
            op.body = "post";
        });
    }

    BOOST_CHECK_EQUAL((size_t)SCORUM_MAX_COMMENT_DEPTH,
                      _api.get_comments(c1.author(), c1.permlink(), SCORUM_MAX_COMMENT_DEPTH).size());

    BOOST_CHECK_EQUAL(3u, _api.get_comments(c1.author(), c1.permlink(), 3).size());
}

BOOST_AUTO_TEST_SUITE_END()
}
