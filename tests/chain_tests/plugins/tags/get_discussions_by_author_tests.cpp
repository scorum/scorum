#ifndef IS_LOW_MEM
#include "get_discussions_by_common.hpp"
#include <scorum/tags/tags_api_objects.hpp>
#include <scorum/common_api/config.hpp>
#include <boost/test/unit_test.hpp>

namespace tags_tests {

using namespace scorum;

BOOST_FIXTURE_TEST_SUITE(get_discussions_by_author_before_date_tests, get_discussions_by_common)

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
     *  alice
     *      bob
     *      sam
     *          alice
     *  bob
     *      alice
     *  alice
     *      sam
     *  bob
     *  alice
     *      alice
     *
    */

    auto a1 = post(alice, "alice1");
    auto a1_b = comment(a1, bob, "alice1-bob");
    auto a1_s = comment(a1, sam, "alice1-sam");
    auto a1_s_a = comment(a1_s, alice, "alice1-sam-alice");

    auto b1 = post(bob, "bob1");
    auto b1_a = comment(b1, alice, "bob1-alice");

    auto a2 = post(alice, "alice2");
    auto a2_s = comment(a2, sam, "alice2-sam");

    auto b2 = post(bob, "bob2");

    auto a3 = post(alice, "alice3");
    auto a3_a = comment(a3, alice, "alice3-alice");

    ignore_unused_variable_warning({ a1_b, a1_s_a, b1_a, a2_s, b2, a3_a });

    api::discussion_query q;
    q.start_author = alice.name;
    q.start_permlink = "";
    q.limit = MAX_DISCUSSIONS_LIST_SIZE;
    auto discussions = _api.get_discussions_by_author(q);

    BOOST_REQUIRE_EQUAL(discussions.size(), 3u);

    BOOST_REQUIRE_EQUAL(discussions[0].permlink, a1.permlink());
    BOOST_REQUIRE_EQUAL(discussions[1].permlink, a2.permlink());
    BOOST_REQUIRE_EQUAL(discussions[2].permlink, a3.permlink());
}

SCORUM_TEST_CASE(check_filtered_by_permlink)
{
    /*
     *  alice
     *      bob
     *      sam
     *          alice
     *  bob
     *      alice
     *  alice
     *      sam
     *  bob
     *  alice
     *      alice
     *  alice
     *
    */

    auto a1 = post(alice, "alice1");
    auto a1_b = comment(a1, bob, "alice1-bob");
    auto a1_s = comment(a1, sam, "alice1-sam");
    auto a1_s_a = comment(a1_s, alice, "alice1-sam-alice");

    auto b1 = post(bob, "bob1");
    auto b1_a = comment(b1, alice, "bob1-alice");

    auto a2 = post(alice, "alice2");
    auto a2_s = comment(a2, sam, "alice2-sam");

    auto b2 = post(bob, "bob2");

    auto a3 = post(alice, "alice3");
    auto a3_a = comment(a3, alice, "alice3-alice");

    auto a4 = post(alice, "alice4");

    ignore_unused_variable_warning({ a1_b, a1_s_a, b1_a, a2_s, b2, a3_a, a4 });

    api::discussion_query q;
    q.start_author = alice.name;
    q.start_permlink = a2.permlink();
    q.limit = MAX_DISCUSSIONS_LIST_SIZE;
    auto discussions = _api.get_discussions_by_author(q);

    BOOST_REQUIRE_EQUAL(discussions.size(), 3u);

    BOOST_REQUIRE_EQUAL(discussions[0].permlink, a2.permlink());
    BOOST_REQUIRE_EQUAL(discussions[1].permlink, a3.permlink());
    BOOST_REQUIRE_EQUAL(discussions[2].permlink, a4.permlink());
}

SCORUM_TEST_CASE(check_filtered_by_limit)
{
    /*
     *  alice
     *      bob
     *      sam
     *          alice
     *  bob
     *      alice
     *  alice
     *      sam
     *  bob
     *  alice
     *      alice
    */

    auto a1 = post(alice, "alice1");
    auto a1_b = comment(a1, bob, "alice1-bob");
    auto a1_s = comment(a1, sam, "alice1-sam");
    auto a1_s_a = comment(a1_s, alice, "alice1-sam-alice");

    auto b1 = post(bob, "bob1");
    auto b1_a = comment(b1, alice, "bob1-alice");

    auto a2 = post(alice, "alice2");
    auto a2_s = comment(a2, sam, "alice2-sam");

    auto b2 = post(bob, "bob2");

    auto a3 = post(alice, "alice3");
    auto a3_a = comment(a3, alice, "alice3-alice");

    ignore_unused_variable_warning({ a1_b, a1_s_a, b1_a, a2_s, b2, a3_a });

    {
        api::discussion_query q;
        q.start_author = alice.name;
        q.start_permlink = "";
        q.limit = 2;
        auto discussions = _api.get_discussions_by_author(q);

        BOOST_REQUIRE_EQUAL(discussions.size(), 2u);

        BOOST_REQUIRE_EQUAL(discussions[0].permlink, a1.permlink());
        BOOST_REQUIRE_EQUAL(discussions[1].permlink, a2.permlink());
    }

    {
        api::discussion_query q;
        q.start_author = alice.name;
        q.start_permlink = "";
        q.limit = 5;
        auto discussions = _api.get_discussions_by_author(q);

        BOOST_REQUIRE_EQUAL(discussions.size(), 3u);

        BOOST_REQUIRE_EQUAL(discussions[0].permlink, a1.permlink());
        BOOST_REQUIRE_EQUAL(discussions[1].permlink, a2.permlink());
        BOOST_REQUIRE_EQUAL(discussions[2].permlink, a3.permlink());
    }

    {
        api::discussion_query q;
        q.start_author = alice.name;
        q.start_permlink = a2.permlink();
        q.limit = 5;
        auto discussions = _api.get_discussions_by_author(q);

        BOOST_REQUIRE_EQUAL(discussions.size(), 2u);

        BOOST_REQUIRE_EQUAL(discussions[0].permlink, a2.permlink());
        BOOST_REQUIRE_EQUAL(discussions[1].permlink, a3.permlink());
    }
}

SCORUM_TEST_CASE(check_filtered_by_permlink_and_limit)
{
    /*
     *  alice
     *      bob
     *      sam
     *          alice
     *  bob
     *      alice
     *  alice
     *      sam
     *  bob
     *  alice
     *      alice
    */

    auto a1 = post(alice, "alice1");
    auto a1_b = comment(a1, bob, "alice1-bob");
    auto a1_s = comment(a1, sam, "alice1-sam");
    auto a1_s_a = comment(a1_s, alice, "alice1-sam-alice");

    auto b1 = post(bob, "bob1");
    auto b1_a = comment(b1, alice, "bob1-alice");

    auto a2 = post(alice, "alice2");
    auto a2_s = comment(a2, sam, "alice2-sam");

    auto b2 = post(bob, "bob2");

    auto a3 = post(alice, "alice3");
    auto a3_a = comment(a3, alice, "alice3-alice");

    ignore_unused_variable_warning({ a1_b, a1_s_a, b1_a, a2_s, b2, a3_a });

    {
        api::discussion_query q;
        q.start_author = alice.name;
        q.start_permlink = a2.permlink();
        q.limit = 1;
        auto discussions = _api.get_discussions_by_author(q);

        BOOST_REQUIRE_EQUAL(discussions.size(), 1u);

        BOOST_REQUIRE_EQUAL(discussions[0].permlink, a2.permlink());
    }
    {
        api::discussion_query q;
        q.start_author = alice.name;
        q.start_permlink = a3.permlink();
        q.limit = 5;
        auto discussions = _api.get_discussions_by_author(q);

        BOOST_REQUIRE_EQUAL(discussions.size(), 1u);

        BOOST_REQUIRE_EQUAL(discussions[0].permlink, a3.permlink());
    }
}

SCORUM_TEST_CASE(check_return_nothing)
{
    /*
     *  alice
     *      bob
     *      sam
     *          alice
     *  bob
     *      alice
     *  alice
     *      sam
     *  bob
     *  alice
     *      alice
    */

    auto a1 = post(alice, "alice1");
    auto a1_b = comment(a1, bob, "alice1-bob");
    auto a1_s = comment(a1, sam, "alice1-sam");
    auto a1_s_a = comment(a1_s, alice, "alice1-sam-alice");

    auto b1 = post(bob, "bob1");
    auto b1_a = comment(b1, alice, "bob1-alice");

    auto a2 = post(alice, "alice2");
    auto a2_s = comment(a2, sam, "alice2-sam");

    auto b2 = post(bob, "bob2");

    auto a3 = post(alice, "alice3");
    auto a3_a = comment(a3, alice, "alice3-alice");

    ignore_unused_variable_warning({ a1_b, a1_s_a, b1_a, a2_s, b2, a3_a });

    {
        api::discussion_query q;
        q.start_author = sam.name;
        q.start_permlink = a2_s.permlink();
        q.limit = 10;
        auto discussions = _api.get_discussions_by_author(q);

        BOOST_REQUIRE_EQUAL(discussions.size(), 0u);
    }

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
}

#endif
