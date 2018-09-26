#include <boost/test/unit_test.hpp>

#include "tags_common.hpp"

#include <scorum/chain/services/comment.hpp>
#include <scorum/chain/services/comment_statistic.hpp>

namespace get_post_comments_by_author_tests {

using namespace scorum;
using namespace scorum::chain;
using namespace scorum::tags;

struct get_post_comments_by_author_fixture : public database_fixture::tags_fixture
{
    get_post_comments_by_author_fixture()
        : comments(db.comment_service())
        , statistic(db.comment_statistic_sp_service())
    {
        actor(initdelegate).give_sp(alice, 1e9);
        actor(initdelegate).give_sp(bob, 1e9);
        actor(initdelegate).give_sp(sam, 1e9);
        actor(initdelegate).give_sp(dave, 1e9);

        generate_block();
    }

    struct discussion_query_wrapper : public api::discussion_query
    {
        discussion_query_wrapper(std::string start_author_,
                                 std::string start_permlink_ = "",
                                 uint32_t limit_ = MAX_DISCUSSIONS_LIST_SIZE,
                                 uint32_t truncate_body_ = 0)
        {
            start_author = start_author_;
            start_permlink = start_permlink_;
            limit = limit_;
            truncate_body = truncate_body_;
        }
    };

    comment_service_i& comments;
    comment_statistic_sp_service_i& statistic;

    struct test_case_1
    {
        comment_op alice_post;
        comment_op sam_post;
        comment_op sam_comment;
        comment_op dave_comment;
        comment_op bob_post;
    };

    // test case #1:
    //--------------
    //  'Alice' create post
    //  'Sam' create post
    //  'Sam' create comment for 'Alice' post
    //  'Dave' create comment for 'Sam' comment
    //  'Sam' and 'Dave' vote for 'Sam' comment
    //  'Alice' vote for 'Sam' post
    //  'Bob' create post
    //  'Alice' vote for 'Bob' post
    //  -conclusion-
    //      rewarded 'Alice' post, 'Sam' post&comment, 'Bob' post.
    //      'Dave' comment is not rewarded
    test_case_1 create_test_case_1()
    {
        auto p_1 = create_post(alice).set_json(default_metadata).in_block();
        auto p_2 = create_post(sam).set_json(default_metadata).in_block(SCORUM_MIN_ROOT_COMMENT_INTERVAL);

        auto c_level1 = p_1.create_comment(sam).in_block();
        auto c_level2 = c_level1.create_comment(dave).in_block();

        generate_blocks(db.head_block_time() + SCORUM_REVERSE_AUCTION_WINDOW_SECONDS);

        c_level1.vote(sam).in_block(SCORUM_MIN_VOTE_INTERVAL_SEC);
        c_level1.vote(dave).in_block(SCORUM_MIN_VOTE_INTERVAL_SEC);
        p_2.vote(alice).in_block(SCORUM_MIN_VOTE_INTERVAL_SEC);

        auto p_3 = create_post(bob).set_json(default_metadata).in_block();
        p_3.vote(alice).in_block(SCORUM_MIN_VOTE_INTERVAL_SEC);

        return { p_1, p_2, c_level1, c_level2, p_3 };
    }

    std::string default_metadata
        = R"({"domains": ["chain_tests"], "categories": ["tags_get_post_comments_by_author_tests"], "tags": ["test"]})";
};

BOOST_FIXTURE_TEST_SUITE(tags_get_post_comments_by_author_tests, get_post_comments_by_author_fixture)

SCORUM_TEST_CASE(get_post_comments_by_author_negative_check)
{
    BOOST_CHECK_THROW(
        _api.get_paid_posts_comments_by_author(discussion_query_wrapper{ "alice", "1", MAX_DISCUSSIONS_LIST_SIZE * 2 }),
        fc::assert_exception);
    BOOST_CHECK_THROW(
        _api.get_paid_posts_comments_by_author(discussion_query_wrapper{ "", "1", MAX_DISCUSSIONS_LIST_SIZE }),
        fc::assert_exception);
}

#ifdef IS_LOW_MEM
SCORUM_TEST_CASE(get_post_comments_by_author_empty_for_low_mem_mode_check)
{
    auto test_case = create_test_case_1();

    generate_blocks(db.head_block_time() + SCORUM_CASHOUT_WINDOW_SECONDS);

    auto result = _api.get_paid_posts_comments_by_author(discussion_query_wrapper{ "bob" });

    BOOST_REQUIRE_EQUAL(
        comments.get(test_case.bob_post.author(), test_case.bob_post.permlink()).cashout_time.to_iso_string(),
        fc::time_point_sec::maximum().to_iso_string());

    BOOST_CHECK(result.empty());
}
#else // IS_LOW_MEM
SCORUM_TEST_CASE(get_post_comments_by_author_not_empty_if_cashout_reached_check)
{
    auto test_case_step1 = create_test_case_1();

    auto start = db.head_block_time();

    generate_blocks(start + SCORUM_MIN_ROOT_COMMENT_INTERVAL);

    auto test_case_step2 = create_test_case_1();

    auto result = _api.get_paid_posts_comments_by_author(discussion_query_wrapper{ "bob" });

    BOOST_REQUIRE(result.empty());

    SCORUM_MESSAGE("-- Check first post of 'Bob' cashout");

    generate_blocks(start + SCORUM_CASHOUT_WINDOW_SECONDS);

    const auto& bob_post_1 = comments.get(test_case_step1.bob_post.author(), test_case_step1.bob_post.permlink());
    const auto& bob_post_2 = comments.get(test_case_step2.bob_post.author(), test_case_step2.bob_post.permlink());
    const auto& bob_stat_step1 = statistic.get(bob_post_1.id);
    const auto& bob_stat_step2 = statistic.get(bob_post_2.id);

    BOOST_REQUIRE_EQUAL(bob_post_1.cashout_time.to_iso_string(), fc::time_point_sec::maximum().to_iso_string());
    BOOST_REQUIRE_NE(bob_post_2.cashout_time.to_iso_string(), fc::time_point_sec::maximum().to_iso_string());

    BOOST_REQUIRE_GT(bob_stat_step1.total_payout_value, ASSET_NULL_SP);
    BOOST_REQUIRE_GT(bob_stat_step1.author_payout_value, ASSET_NULL_SP);

    BOOST_REQUIRE_EQUAL(bob_stat_step2.total_payout_value, ASSET_NULL_SP);
    BOOST_REQUIRE_EQUAL(bob_stat_step2.author_payout_value, ASSET_NULL_SP);

    result = _api.get_paid_posts_comments_by_author(discussion_query_wrapper{ "bob" });

    BOOST_REQUIRE_EQUAL(result.size(), 1u);
    BOOST_CHECK_EQUAL(result[0].author, test_case_step1.bob_post.author());
    BOOST_CHECK_EQUAL(result[0].permlink, test_case_step1.bob_post.permlink());

    SCORUM_MESSAGE("-- Check second post of 'Bob' cashout");

    generate_blocks(db.head_block_time() + SCORUM_CASHOUT_WINDOW_SECONDS);

    BOOST_REQUIRE_EQUAL(bob_post_2.cashout_time.to_iso_string(), fc::time_point_sec::maximum().to_iso_string());

    result = _api.get_paid_posts_comments_by_author(discussion_query_wrapper{ "bob" });

    BOOST_REQUIRE_EQUAL(result.size(), 2u);
    BOOST_CHECK_EQUAL(result[0].author, test_case_step2.bob_post.author());
    BOOST_CHECK_EQUAL(result[0].permlink, test_case_step2.bob_post.permlink());
    BOOST_CHECK_EQUAL(result[1].author, test_case_step1.bob_post.author());
    BOOST_CHECK_EQUAL(result[1].permlink, test_case_step1.bob_post.permlink());

    SCORUM_MESSAGE("-- Check 'Bob's discussions are sorted");

    BOOST_CHECK(std::is_sorted(begin(result), end(result), [](const api::discussion& lhs, const api::discussion& rhs) {
        return lhs.last_payout > rhs.last_payout;
    }));
}

SCORUM_TEST_CASE(get_post_comments_by_author_should_return_rewarded_post)
{
    auto test_case = create_test_case_1();

    auto result = _api.get_paid_posts_comments_by_author(discussion_query_wrapper{ "bob" });

    BOOST_REQUIRE(result.empty());

    generate_blocks(db.head_block_time() + SCORUM_CASHOUT_WINDOW_SECONDS);

    const auto& bob_post = comments.get(test_case.bob_post.author(), test_case.bob_post.permlink());
    const auto& bob_stat = statistic.get(bob_post.id);

    BOOST_REQUIRE_EQUAL(bob_post.cashout_time.to_iso_string(), fc::time_point_sec::maximum().to_iso_string());

    BOOST_REQUIRE_GT(bob_stat.total_payout_value, ASSET_NULL_SP);
    BOOST_REQUIRE_GT(bob_stat.author_payout_value, ASSET_NULL_SP);

    result = _api.get_paid_posts_comments_by_author(discussion_query_wrapper{ "bob" });

    BOOST_REQUIRE_EQUAL(result.size(), 1u);
    BOOST_CHECK_EQUAL(result[0].author, test_case.bob_post.author());
    BOOST_CHECK_EQUAL(result[0].permlink, test_case.bob_post.permlink());
}

SCORUM_TEST_CASE(get_post_comments_by_author_should_return_rewarded_post_and_comment)
{
    auto test_case = create_test_case_1();

    auto result = _api.get_paid_posts_comments_by_author(discussion_query_wrapper{ "sam" });

    BOOST_REQUIRE(result.empty());

    generate_blocks(db.head_block_time() + SCORUM_CASHOUT_WINDOW_SECONDS);

    const auto& sam_comment = comments.get(test_case.sam_comment.author(), test_case.sam_comment.permlink());
    const auto& sam_post = comments.get(test_case.sam_post.author(), test_case.sam_post.permlink());
    const auto& sam_comment_stat = statistic.get(sam_comment.id);
    const auto& sam_post_stat = statistic.get(sam_post.id);

    BOOST_REQUIRE_EQUAL(sam_comment.cashout_time.to_iso_string(), fc::time_point_sec::maximum().to_iso_string());
    BOOST_REQUIRE_EQUAL(sam_post.cashout_time.to_iso_string(), fc::time_point_sec::maximum().to_iso_string());

    BOOST_REQUIRE_GT(sam_comment_stat.total_payout_value, ASSET_NULL_SP);
    BOOST_REQUIRE_GT(sam_post_stat.total_payout_value, ASSET_NULL_SP);

    result = _api.get_paid_posts_comments_by_author(discussion_query_wrapper{ "sam" });

    BOOST_REQUIRE_EQUAL(result.size(), 2);
    BOOST_CHECK_EQUAL(result[0].author, test_case.sam_post.author());
    BOOST_CHECK_EQUAL(result[0].permlink, test_case.sam_post.permlink());
    BOOST_CHECK_EQUAL(result[1].author, test_case.sam_comment.author());
    BOOST_CHECK_EQUAL(result[1].permlink, test_case.sam_comment.permlink());
}

SCORUM_TEST_CASE(check_ordering_by_last_payout_legacy_reward_distribution)
{
    set_hardfork(SCORUM_HARDFORK_0_2);

    auto p_1 = create_post(alice).set_json(default_metadata).in_block(SCORUM_MIN_ROOT_COMMENT_INTERVAL);
    generate_block();
    auto p_2 = create_post(alice).set_json(default_metadata).in_block(SCORUM_MIN_ROOT_COMMENT_INTERVAL);

    p_1.vote(dave).in_block(SCORUM_MIN_VOTE_INTERVAL_SEC);
    generate_block();
    p_2.vote(dave).in_block(SCORUM_MIN_VOTE_INTERVAL_SEC);

    // p_1 & p_2 cashout
    generate_blocks(db.head_block_time() + SCORUM_CASHOUT_WINDOW_SECONDS);

    auto result = _api.get_paid_posts_comments_by_author(discussion_query_wrapper{ "alice" });
    BOOST_REQUIRE_EQUAL(result.size(), 2);
    BOOST_CHECK_EQUAL(result[0].permlink, p_1.permlink());
    BOOST_CHECK_EQUAL(result[1].permlink, p_2.permlink());

    auto c_1 = p_2.create_comment(dave).in_block();
    c_1.vote(dave).in_block(SCORUM_MIN_VOTE_INTERVAL_SEC);

    // c_1 cashout (p_2 will also rewarded)
    generate_blocks(db.head_block_time() + SCORUM_CASHOUT_WINDOW_SECONDS);

    result = _api.get_paid_posts_comments_by_author(discussion_query_wrapper{ "alice" });
    BOOST_REQUIRE_EQUAL(result.size(), 2);
    BOOST_CHECK_EQUAL(result[0].permlink, p_2.permlink());
    BOOST_CHECK_EQUAL(result[1].permlink, p_1.permlink());
}

SCORUM_TEST_CASE(get_post_comments_by_author_pagination_for_rewarded_check)
{
    std::vector<test_case_1> test_cases;

    const auto page_size = 3u;

    for (auto ci = 0u; ci < page_size * 2; ++ci)
    {
        test_cases.emplace_back(create_test_case_1());

        generate_blocks(db.head_block_time() + SCORUM_MIN_ROOT_COMMENT_INTERVAL);
    }

    generate_blocks(db.head_block_time() + SCORUM_CASHOUT_WINDOW_SECONDS);

    auto result_page1 = _api.get_paid_posts_comments_by_author(discussion_query_wrapper{ "bob", "", page_size });

    wdump((result_page1));

    BOOST_REQUIRE_EQUAL(result_page1.size(), page_size);

    BOOST_CHECK(std::is_sorted(
        begin(result_page1), end(result_page1),
        [](const api::discussion& lhs, const api::discussion& rhs) { return lhs.last_payout > rhs.last_payout; }));

    auto result_page2 = _api.get_paid_posts_comments_by_author(
        discussion_query_wrapper{ "bob", result_page1.rbegin()->permlink, page_size + 1 });
    // next page includes last record from previous page. It is Ok for front-end
    result_page2.erase(result_page2.begin());

    wdump((result_page2));

    BOOST_REQUIRE_EQUAL(result_page2.size(), page_size);

    BOOST_CHECK(std::is_sorted(
        begin(result_page2), end(result_page2),
        [](const api::discussion& lhs, const api::discussion& rhs) { return lhs.last_payout > rhs.last_payout; }));

    decltype(result_page1) result;

    std::copy(begin(result_page1), end(result_page1), std::back_inserter(result));
    std::copy(begin(result_page2), end(result_page2), std::back_inserter(result));

    wdump((result));

    BOOST_REQUIRE_EQUAL(result.size(), page_size * 2);

    BOOST_CHECK(std::is_sorted(begin(result), end(result), [](const api::discussion& lhs, const api::discussion& rhs) {
        return lhs.last_payout > rhs.last_payout;
    }));
}
#endif //! IS_LOW_MEM
BOOST_AUTO_TEST_SUITE_END()
}
