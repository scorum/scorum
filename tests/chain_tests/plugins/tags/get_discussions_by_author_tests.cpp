#ifndef IS_LOW_MEM
#include "tags_common.hpp"
#include <scorum/tags/tags_api_objects.hpp>
#include <scorum/common_api/config.hpp>
#include <boost/test/unit_test.hpp>

using namespace scorum;
using namespace scorum::tags;

BOOST_FIXTURE_TEST_SUITE(get_discussions_by_author_tests, database_fixture::tags_fixture)

SCORUM_TEST_CASE(limit_overflow_should_assert)
{
    api::discussion_query q;
    q.start_author = "alice";
    q.start_permlink = "";
    q.limit = MAX_DISCUSSIONS_LIST_SIZE;

    BOOST_REQUIRE_NO_THROW(_api.get_discussions_by_author(q));

    q.limit += 1;

    BOOST_REQUIRE_THROW(_api.get_discussions_by_author(q), fc::assert_exception);
}

SCORUM_TEST_CASE(check_filtered_by_author_name)
{
    /*
     *  bob
     *      alice
     *  alice
     *      sam
     *  bob
     *  alice
     *      alice
     *
    */

    auto b1 = create_post(bob).in_block_with_delay();
    b1.create_comment(alice).in_block_with_delay();

    auto a1 = create_post(alice).in_block_with_delay();
    a1.create_comment(sam).in_block_with_delay();

    create_post(bob);

    auto a2 = create_post(alice).in_block_with_delay();
    a2.create_comment(alice).in_block_with_delay();

    api::discussion_query q;
    q.start_author = alice.name;
    q.start_permlink = "";
    q.limit = MAX_DISCUSSIONS_LIST_SIZE;
    auto discussions = _api.get_discussions_by_author(q);

    BOOST_REQUIRE_EQUAL(discussions.size(), 2u);

    BOOST_REQUIRE_EQUAL(discussions[0].permlink, a2.permlink());
    BOOST_REQUIRE_EQUAL(discussions[1].permlink, a1.permlink());
}

SCORUM_TEST_CASE(check_filtered_by_permlink)
{
    /*
     *  alice
     *      bob
     *  bob
     *  alice
     *      sam
     *  alice
     *      alice
     *
    */

    auto a1 = create_post(alice).in_block_with_delay();
    a1.create_comment(bob).in_block_with_delay();

    create_post(bob).in_block_with_delay();

    auto a2 = create_post(alice).in_block_with_delay();
    a2.create_comment(sam).in_block_with_delay();

    auto a3 = create_post(alice).in_block_with_delay();
    a3.create_comment(alice).in_block_with_delay();

    api::discussion_query q;
    q.start_author = alice.name;
    q.start_permlink = a2.permlink();
    q.limit = MAX_DISCUSSIONS_LIST_SIZE;
    auto discussions = _api.get_discussions_by_author(q);

    BOOST_REQUIRE_EQUAL(discussions.size(), 2u);

    BOOST_REQUIRE_EQUAL(discussions[0].permlink, a2.permlink());
    BOOST_REQUIRE_EQUAL(discussions[1].permlink, a1.permlink());
}

SCORUM_TEST_CASE(check_filtered_by_limit)
{
    /*
     *  alice
     *      bob
     *  bob
     *  alice
     *      sam
     *  alice
     *      alice
     *
    */

    auto a1 = create_post(alice).in_block_with_delay();
    a1.create_comment(bob).in_block_with_delay();

    create_post(bob).in_block_with_delay();

    auto a2 = create_post(alice).in_block_with_delay();
    a2.create_comment(sam).in_block_with_delay();

    auto a3 = create_post(alice).in_block_with_delay();
    a3.create_comment(alice).in_block_with_delay();

    api::discussion_query q;
    q.start_author = alice.name;
    q.start_permlink = "";
    q.limit = 2;
    auto discussions = _api.get_discussions_by_author(q);

    BOOST_REQUIRE_EQUAL(discussions.size(), 2u);

    BOOST_REQUIRE_EQUAL(discussions[0].permlink, a3.permlink());
    BOOST_REQUIRE_EQUAL(discussions[1].permlink, a2.permlink());
}

SCORUM_TEST_CASE(check_filtered_by_permlink_and_limit)
{
    /*
     *  alice
     *      bob
     *  alice
     *      bob
     *  alice
     *      sam
     *  alice
     *      alice
     *
    */

    auto a1 = create_post(alice).in_block_with_delay();
    a1.create_comment(bob).in_block_with_delay();

    auto a2 = create_post(alice).in_block_with_delay();
    a2.create_comment(bob).in_block_with_delay();

    auto a3 = create_post(alice).in_block_with_delay();
    a3.create_comment(sam).in_block_with_delay();

    auto a4 = create_post(alice).in_block_with_delay();
    a4.create_comment(alice).in_block_with_delay();

    api::discussion_query q;
    q.start_author = alice.name;
    q.start_permlink = a3.permlink();
    q.limit = 2;
    auto discussions = _api.get_discussions_by_author(q);

    BOOST_REQUIRE_EQUAL(discussions.size(), 2u);

    BOOST_REQUIRE_EQUAL(discussions[0].permlink, a3.permlink());
    BOOST_REQUIRE_EQUAL(discussions[1].permlink, a2.permlink());
}

SCORUM_TEST_CASE(check_returned_by_created_in_desc_order)
{
    auto a1 = create_post(alice).in_block_with_delay();
    create_post(bob).in_block_with_delay();
    auto a2 = create_post(alice).in_block_with_delay();

    api::discussion_query q;
    q.start_author = alice.name;
    q.start_permlink = "";
    q.limit = MAX_DISCUSSIONS_LIST_SIZE;

    {
        auto discussions = _api.get_discussions_by_author(q);

        BOOST_REQUIRE_EQUAL(discussions.size(), 2u);
        BOOST_CHECK_EQUAL(discussions[0].permlink, a2.permlink());
        BOOST_CHECK_EQUAL(discussions[1].permlink, a1.permlink());
    }

    create_post(alice).set_permlink(a1.permlink()).set_body("new-body").in_block_with_delay();

    {
        auto discussions = _api.get_discussions_by_author(q);

        BOOST_REQUIRE_EQUAL(discussions.size(), 2u);
        BOOST_CHECK_EQUAL(discussions[0].permlink, a2.permlink());
        BOOST_CHECK_EQUAL(discussions[1].permlink, a1.permlink());
    }
}

SCORUM_TEST_CASE(check_return_nothing)
{
    /*
     *  alice
     *      bob
     *      sam
     *          alice
     *  alice
     *      sam
     *  bob
    */

    auto a1 = create_post(alice).in_block_with_delay();
    a1.create_comment(bob).in_block_with_delay();
    auto s1 = a1.create_comment(sam).in_block_with_delay();
    s1.create_comment(alice).in_block_with_delay();

    auto a2 = create_post(alice).in_block_with_delay();
    a2.create_comment(sam).in_block_with_delay();

    create_post(bob).in_block_with_delay();

    {
        api::discussion_query q;
        q.start_author = sam.name;
        q.start_permlink = "";
        q.limit = 10;
        auto discussions = _api.get_discussions_by_author(q);

        BOOST_REQUIRE_EQUAL(discussions.size(), 0u);
    }
}

BOOST_AUTO_TEST_SUITE_END()

#endif
