#ifndef IS_LOW_MEM
#include "tags_common.hpp"
#include <scorum/tags/tags_api_objects.hpp>
#include <scorum/common_api/config.hpp>
#include <boost/test/unit_test.hpp>

namespace tags_tests {

using namespace scorum;

struct get_discussions_by_author_fixture : public tags_fixture
{
    using discussion = scorum::tags::api::discussion;

    get_discussions_by_author_fixture()
    {
        actor(initdelegate).create_account(alice);
        actor(initdelegate).create_account(bob);
        actor(initdelegate).create_account(sam);
    }

    Comment post(Actor& author, const std::string& permlink)
    {
        return create_post(author, [&](comment_operation& o) {
            o.permlink = permlink;
            o.body = "body";
        });
    }

    Comment comment(Comment& post, Actor& commenter, const std::string& permlink)
    {
        return post.create_comment(commenter, [&](comment_operation& o) {
            o.permlink = permlink;
            o.body = "body";
        });
    }

    std::vector<discussion>::const_iterator find(const std::vector<discussion>& discussions,
                                                 const std::string& permlink)
    {
        return std::find_if(begin(discussions), end(discussions),
                            [&](const discussion& d) { return d.permlink == permlink; });
    }

    void ignore_unused_variable_warning(std::initializer_list<std::reference_wrapper<Comment>> lst)
    {
        boost::ignore_unused_variable_warning(lst);
    }

    Actor alice = "alice";
    Actor bob = "bob";
    Actor sam = "sam";
};

BOOST_FIXTURE_TEST_SUITE(get_discussions_by_author_before_date_tests, get_discussions_by_author_fixture)

SCORUM_TEST_CASE(limit_overflow_should_assert)
{
    BOOST_REQUIRE_NO_THROW(
        _api.get_discussions_by_author_before_date("alice", "", fc::time_point_sec(), MAX_DISCUSSIONS_LIST_SIZE));
    BOOST_REQUIRE_THROW(
        _api.get_discussions_by_author_before_date("alice", "", fc::time_point_sec(), MAX_DISCUSSIONS_LIST_SIZE + 1),
        fc::assert_exception);
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

    auto discussions
        = _api.get_discussions_by_author_before_date(alice.name, "", fc::time_point_sec(), MAX_DISCUSSIONS_LIST_SIZE);

    BOOST_REQUIRE_EQUAL(discussions.size(), 3);

    BOOST_CHECK(find(discussions, a3.permlink()) != end(discussions));
    BOOST_CHECK(find(discussions, a2.permlink()) != end(discussions));
    BOOST_CHECK(find(discussions, a1.permlink()) != end(discussions));
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

    auto discussions = _api.get_discussions_by_author_before_date(alice.name, a2.permlink(), fc::time_point_sec(),
                                                                  MAX_DISCUSSIONS_LIST_SIZE);

    BOOST_REQUIRE_EQUAL(discussions.size(), 2);

    BOOST_CHECK(find(discussions, a3.permlink()) == end(discussions));
    BOOST_CHECK(find(discussions, a2.permlink()) != end(discussions));
    BOOST_CHECK(find(discussions, a1.permlink()) != end(discussions));
}

SCORUM_TEST_CASE(check_filtered_by_date)
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

    auto b1_create_time = db.head_block_time();

    auto b1 = post(bob, "bob1");
    auto b1_a = comment(b1, alice, "bob1-alice");

    auto a2 = post(alice, "alice2");
    auto a2_s = comment(a2, sam, "alice2-sam");

    auto b2_create_time = db.head_block_time();

    auto b2 = post(bob, "bob2");

    auto a3 = post(alice, "alice3");
    auto a3_a = comment(a3, alice, "alice3-alice");

    ignore_unused_variable_warning({ a1_b, a1_s_a, b1_a, a2_s, b2, a3_a });

    {
        auto discussions
            = _api.get_discussions_by_author_before_date(alice.name, "", b2_create_time, MAX_DISCUSSIONS_LIST_SIZE);

        BOOST_REQUIRE_EQUAL(discussions.size(), 2);

        BOOST_CHECK(find(discussions, a3.permlink()) == end(discussions));
        BOOST_CHECK(find(discussions, a2.permlink()) != end(discussions));
        BOOST_CHECK(find(discussions, a1.permlink()) != end(discussions));
    }
    {
        auto discussions
            = _api.get_discussions_by_author_before_date(alice.name, "", b1_create_time, MAX_DISCUSSIONS_LIST_SIZE);

        BOOST_REQUIRE_EQUAL(discussions.size(), 1);

        BOOST_CHECK(find(discussions, a3.permlink()) == end(discussions));
        BOOST_CHECK(find(discussions, a2.permlink()) == end(discussions));
        BOOST_CHECK(find(discussions, a1.permlink()) != end(discussions));
    }
}

SCORUM_TEST_CASE(check_filtered_by_permlink_and_date_where_permlink_is_newer)
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

    auto b1_create_time = db.head_block_time();

    auto b1 = post(bob, "bob1");
    auto b1_a = comment(b1, alice, "bob1-alice");

    auto a2 = post(alice, "alice2");
    auto a2_s = comment(a2, sam, "alice2-sam");

    auto b2 = post(bob, "bob2");

    auto a3 = post(alice, "alice3");
    auto a3_a = comment(a3, alice, "alice3-alice");

    ignore_unused_variable_warning({ a1_b, a1_s_a, b1_a, a2_s, b2, a3_a });

    auto discussions = _api.get_discussions_by_author_before_date(alice.name, a2.permlink(), b1_create_time,
                                                                  MAX_DISCUSSIONS_LIST_SIZE);

    BOOST_REQUIRE_EQUAL(discussions.size(), 1);

    BOOST_CHECK(find(discussions, a3.permlink()) == end(discussions));
    BOOST_CHECK(find(discussions, a2.permlink()) == end(discussions));
    BOOST_CHECK(find(discussions, a1.permlink()) != end(discussions));
}

SCORUM_TEST_CASE(check_filtered_by_date_and_limit)
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

    auto except_two_last_blocks = SCORUM_BLOCK_INTERVAL * 2;

    auto discussions = _api.get_discussions_by_author_before_date(alice.name, a2.permlink(),
                                                                  db.head_block_time() - except_two_last_blocks, 1);

    BOOST_REQUIRE_EQUAL(discussions.size(), 1);

    BOOST_CHECK(find(discussions, a3.permlink()) == end(discussions));
    BOOST_CHECK(find(discussions, a2.permlink()) != end(discussions));
    BOOST_CHECK(find(discussions, a1.permlink()) == end(discussions));
}

BOOST_AUTO_TEST_SUITE_END()
}

#endif
