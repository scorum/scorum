#include <boost/test/unit_test.hpp>

#include "tags_common.hpp"

#include <scorum/chain/services/comment.hpp>
#include <scorum/chain/services/comment_statistic.hpp>

namespace get_parents_tests {

using namespace scorum;
using namespace scorum::chain;
using namespace scorum::tags;

struct get_parents_fixture : public database_fixture::tags_fixture
{
    get_parents_fixture()
        : comments(db.comment_service())
        , statistic(db.comment_statistic_sp_service())
    {
        actor(initdelegate).give_sp(alice, 1e5);
        actor(initdelegate).give_sp(bob, 1e5);
        actor(initdelegate).give_sp(sam, 1e5);
        actor(initdelegate).give_sp(dave, 1e5);
    }

    struct content_query_wrapper : public api::content_query
    {
        content_query_wrapper(std::string author_, std::string permlink_)
        {
            author = author_;
            permlink = permlink_;
        }
    };

    comment_service_i& comments;
    comment_statistic_sp_service_i& statistic;

    struct test_case_1
    {
        comment_op alice_post;
        comment_op sam_comment;
        comment_op dave_comment;
        comment_op bob_post;
    };

    // test case #1:
    //--------------
    //  'Alice' create post
    //  'Sam' create comment for 'Alice' post
    //  'Dave' create comment for 'Sam' comment
    //  'Bob' create post
    test_case_1 create_test_case_1()
    {
        auto p_1 = create_post(alice).set_json(default_metadata).in_block();

        auto c_level1 = p_1.create_comment(sam).in_block();
        auto c_level2 = c_level1.create_comment(dave).in_block();

        auto p_2 = create_post(bob).set_json(default_metadata).in_block();

        return { p_1, c_level1, c_level2, p_2 };
    }

    std::string default_metadata
        = R"({"domains": ["chain_tests"], "categories": ["tags_get_parents_tests"], "tags": ["test"]})";
};

BOOST_FIXTURE_TEST_SUITE(tags_get_parents_tests, get_parents_fixture)

SCORUM_TEST_CASE(get_parents_negative_check)
{
    BOOST_CHECK_THROW(_api.get_parents(content_query_wrapper{ "", "1" }), fc::assert_exception);
    BOOST_CHECK_THROW(_api.get_parents(content_query_wrapper{ "alice", "" }), fc::assert_exception);
}

SCORUM_TEST_CASE(get_parents_positive_check)
{
    auto test_case = create_test_case_1();

    auto result
        = _api.get_parents(content_query_wrapper{ test_case.dave_comment.author(), test_case.dave_comment.permlink() });

    BOOST_REQUIRE_EQUAL(result.size(), 2u);

    BOOST_CHECK(std::find_if(std::begin(result), std::end(result),
                             [&](const api::discussion& d) {
                                 return d.author == test_case.sam_comment.author()
                                     && d.permlink == test_case.sam_comment.permlink();
                             })
                != result.end());

    BOOST_CHECK(std::find_if(std::begin(result), std::end(result),
                             [&](const api::discussion& d) {
                                 return d.author == test_case.alice_post.author()
                                     && d.permlink == test_case.alice_post.permlink();
                             })
                != result.end());

    result
        = _api.get_parents(content_query_wrapper{ test_case.sam_comment.author(), test_case.sam_comment.permlink() });

    BOOST_REQUIRE_EQUAL(result.size(), 1u);

    BOOST_CHECK(std::find_if(std::begin(result), std::end(result),
                             [&](const api::discussion& d) {
                                 return d.author == test_case.alice_post.author()
                                     && d.permlink == test_case.alice_post.permlink();
                             })
                != result.end());

    result = _api.get_parents(content_query_wrapper{ test_case.alice_post.author(), test_case.alice_post.permlink() });

    BOOST_REQUIRE_EQUAL(result.size(), 0u);

    result = _api.get_parents(content_query_wrapper{ test_case.bob_post.author(), test_case.bob_post.permlink() });

    BOOST_REQUIRE_EQUAL(result.size(), 0u);
}

BOOST_AUTO_TEST_SUITE_END()
}
