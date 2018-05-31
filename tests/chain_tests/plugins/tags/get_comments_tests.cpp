#include <boost/test/unit_test.hpp>

#include "tags_common.hpp"

BOOST_FIXTURE_TEST_SUITE(get_comments, database_fixture::tags_fixture)

SCORUM_TEST_CASE(no_children)
{
    auto post = create_post(initdelegate).in_block_with_delay();

    const auto discussions = _api.get_comments(post.author(), post.permlink());

    BOOST_REQUIRE_EQUAL(0u, discussions.size());
}

SCORUM_TEST_CASE(one_child)
{
    auto post = create_post(initdelegate).in_block_with_delay();
    post.create_comment(initdelegate).in_block_with_delay();

    const auto discussions = _api.get_comments(post.author(), post.permlink());

    BOOST_REQUIRE_EQUAL(1u, discussions.size());
}

SCORUM_TEST_CASE(two_children)
{
    auto post = create_post(initdelegate).in_block_with_delay();
    post.create_comment(initdelegate).in_block_with_delay();
    post.create_comment(initdelegate).in_block_with_delay();

    const auto discussions = _api.get_comments(post.author(), post.permlink());

    BOOST_REQUIRE_EQUAL(2u, discussions.size());
}

SCORUM_TEST_CASE(no_post_children)
{
    auto post = create_post(initdelegate).in_block_with_delay();
    generate_block();
    auto post2 = create_post(initdelegate).set_json(R"({"tags" : ["football"]})").in_block_with_delay();
    post2.create_comment(initdelegate).in_block_with_delay();

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

    auto c1 = create_post(initdelegate).in_block_with_delay();
    auto c2 = c1.create_comment(initdelegate).in_block_with_delay();
    auto c3 = c2.create_comment(initdelegate).in_block_with_delay();
    auto c4 = c1.create_comment(initdelegate).in_block_with_delay();
    auto c5 = c4.create_comment(initdelegate).in_block_with_delay();
    auto c6 = c2.create_comment(initdelegate).in_block_with_delay();
    auto c7 = c5.create_comment(initdelegate).in_block_with_delay();

    const auto discussions = _api.get_comments(c1.author(), c1.permlink());

    const std::vector<std::reference_wrapper<comment_op>> expected_order = { c2, c3, c6, c4, c5, c7 };

    BOOST_REQUIRE_EQUAL(6u, discussions.size());

    for (size_t i = 0; i < discussions.size(); ++i)
    {
        BOOST_CHECK_EQUAL(discussions[i].permlink, expected_order[i].get().permlink());
    }
}

SCORUM_TEST_CASE(filter_with_depth)
{
    auto c1 = create_post(initdelegate).in_block_with_delay();
    auto parent = c1;

    for (size_t i = 0; i < SCORUM_MAX_COMMENT_DEPTH; i++)
    {
        parent = parent.create_comment(initdelegate).in_block_with_delay();
    }

    BOOST_CHECK_EQUAL((size_t)SCORUM_MAX_COMMENT_DEPTH,
                      _api.get_comments(c1.author(), c1.permlink(), SCORUM_MAX_COMMENT_DEPTH).size());

    BOOST_CHECK_EQUAL(3u, _api.get_comments(c1.author(), c1.permlink(), 3).size());
}

BOOST_AUTO_TEST_SUITE_END()
