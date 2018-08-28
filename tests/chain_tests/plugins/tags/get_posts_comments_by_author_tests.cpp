#include <boost/test/unit_test.hpp>

#include "tags_common.hpp"

namespace get_post_comments_by_author_tests {

using namespace scorum;
using namespace scorum::tags;

struct get_post_comments_by_author_fixture : public database_fixture::tags_fixture
{
    get_post_comments_by_author_fixture() = default;

    struct discussion_query_wrapper : public api::discussion_query
    {
        discussion_query_wrapper(std::string start_author_,
                                 std::string start_permlink_ = "",
                                 uint32_t limit_ = 0,
                                 uint32_t truncate_body_ = 0)
        {
            start_author = start_author_;
            start_permlink = start_permlink_;
            limit = limit_;
            truncate_body = truncate_body_;
        }
    };

    struct test_case_1
    {
        comment_op alice_post;
        comment_op sam_comment_l1;
        comment_op dave_comment_l2;
        comment_op bob_post;
    };

    // test case #1:
    //--------------
    //  'Alice' create post
    //  'Sam' create comment for 'Alice' post
    //  'Dave' create comment for 'Sam' comment
    //  'Sam' and 'Dave' vote for 'Sam' comment
    //  'Bob' create post
    //  'Alice' vote for 'Bob' post
    //  -conclusion-
    //      rewarded 'Alice' post, 'Sam' comment, 'Bob' post.
    //      'Dave' comment is not rewarded
    test_case_1 create_test_case_1()
    {
        auto p_1 = create_post(alice).set_json(default_metadata).in_block();

        auto c_level1 = p_1.create_comment(sam).in_block();
        auto c_level2 = c_level1.create_comment(dave).in_block();

        generate_blocks(db.head_block_time() + SCORUM_REVERSE_AUCTION_WINDOW_SECONDS);

        c_level1.vote(sam).in_block(SCORUM_MIN_VOTE_INTERVAL_SEC);
        c_level2.vote(dave).in_block(SCORUM_MIN_VOTE_INTERVAL_SEC);

        auto p_2 = create_post(bob).set_json(default_metadata).in_block();
        p_2.vote(alice).in_block(SCORUM_MIN_VOTE_INTERVAL_SEC);

        return { p_1, c_level1, c_level2, p_2 };
    }

    std::string default_metadata
        = R"({"domains": ["chain_tests"], "categories": ["tags_get_post_comments_by_author_tests"], "tags": ["test"]})";
};

BOOST_FIXTURE_TEST_SUITE(tags_get_post_comments_by_author_tests, get_post_comments_by_author_fixture)

SCORUM_TEST_CASE(get_post_comments_by_author_nevative_check)
{
    BOOST_CHECK_THROW(
        _api.get_posts_comments_by_author(discussion_query_wrapper{ "alice", "1", MAX_DISCUSSIONS_LIST_SIZE * 2 }),
        fc::assert_exception);
    BOOST_CHECK_THROW(_api.get_posts_comments_by_author(discussion_query_wrapper{ "", "1", MAX_DISCUSSIONS_LIST_SIZE }),
                      fc::assert_exception);
}

SCORUM_TEST_CASE(get_post_comments_by_author_not_empty_if_cashout_reached_check)
{
    auto test_case = create_test_case_1();

    auto result = _api.get_posts_comments_by_author(discussion_query_wrapper{ "alice" });

    BOOST_REQUIRE(result.empty());

    generate_blocks(db.head_block_time() + SCORUM_CASHOUT_WINDOW_SECONDS);

    result = _api.get_posts_comments_by_author(discussion_query_wrapper{ "alice" });

    BOOST_REQUIRE_EQUAL(result.size(), 1u);

    BOOST_CHECK(std::find_if(std::begin(result), std::end(result),
                             [&](const api::discussion& d) {
                                 return d.author == test_case.alice_post.author()
                                     && d.permlink == test_case.alice_post.permlink();
                             })
                != result.end());
}

SCORUM_TEST_CASE(get_post_comments_by_author_not_empty_if_rewarded_check)
{
    // TODO
}

SCORUM_TEST_CASE(get_post_comments_by_author_pagination_for_rewarded_check)
{
    // TODO
}
BOOST_AUTO_TEST_SUITE_END()
}
