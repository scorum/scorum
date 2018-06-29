#include <boost/test/unit_test.hpp>
#include <fc/exception/exception.hpp>
#include <fc/io/datastream.hpp>
#include <scorum/chain/schema/comment_objects.hpp>
#include <scorum/chain/services/comment.hpp>
#include "database_blog_integration.hpp"

using namespace scorum::chain;

namespace database_fixture {
class comment_delete_fixture : public database_blog_integration_fixture
{
public:
    comment_delete_fixture()
    {
        open_database();

        actor(initdelegate).create_account(alice);
        actor(initdelegate).create_account(bob);
        actor(initdelegate).create_account(sam);

        actor(initdelegate).give_sp(alice, 1e9);
        actor(initdelegate).give_sp(bob, 1e9);
        actor(initdelegate).give_sp(sam, 1e9);
    }

    Actor alice = "alice";
    Actor bob = "bob";
    Actor sam = "sam";
};

BOOST_FIXTURE_TEST_SUITE(comment_deletion_tests, comment_delete_fixture)

SCORUM_TEST_CASE(shouldnt_delete_comment_which_has_children)
{
    auto p = create_post(alice).in_block();
    auto c1 = p.create_comment(bob).in_block();
    c1.create_comment(sam).in_block();

    delete_comment_operation op;
    op.author = c1.author();
    op.permlink = c1.permlink();

    BOOST_REQUIRE_THROW(push_operation(op), fc::assert_exception);
}

SCORUM_TEST_CASE(shouldnt_delete_post_which_has_children)
{
    auto p = create_post(alice).in_block();
    auto c1 = p.create_comment(bob).in_block();
    c1.create_comment(sam).in_block();

    delete_comment_operation op;
    op.author = p.author();
    op.permlink = p.permlink();

    BOOST_REQUIRE_THROW(push_operation(op), fc::assert_exception);
}

SCORUM_TEST_CASE(should_delete_comment_after_delete_children)
{
    auto p = create_post(alice).in_block();
    auto c1 = p.create_comment(bob).in_block();
    auto c2 = c1.create_comment(sam).in_block();
    BOOST_CHECK_EQUAL((db.get_index<comment_index, by_id>().size()), 3);

    delete_comment_operation op;
    op.author = c2.author();
    op.permlink = c2.permlink();

    push_operation(op);
    BOOST_CHECK_EQUAL((db.get_index<comment_index, by_id>().size()), 2);

    op.author = c1.author();
    op.permlink = c1.permlink();

    push_operation(op);
    BOOST_CHECK_EQUAL((db.get_index<comment_index, by_id>().size()), 1);
}

SCORUM_TEST_CASE(should_delete_post_after_delete_children)
{
    auto p = create_post(alice).in_block();
    auto c1 = p.create_comment(bob).in_block();
    BOOST_REQUIRE_EQUAL((db.get_index<comment_index, by_id>().size()), 2);

    delete_comment_operation op;
    op.author = c1.author();
    op.permlink = c1.permlink();

    push_operation(op);
    BOOST_CHECK_EQUAL((db.get_index<comment_index, by_id>().size()), 1);

    op.author = p.author();
    op.permlink = p.permlink();

    push_operation(op);
    BOOST_CHECK_EQUAL((db.get_index<comment_index, by_id>().size()), 0);
}

SCORUM_TEST_CASE(shouldnt_delete_comment_after_vote)
{
    auto p = create_post(alice).in_block();
    auto c1 = p.create_comment(bob).in_block();
    BOOST_CHECK_EQUAL((db.get_index<comment_index, by_id>().size()), 2);

    c1.vote(sam).in_block();

    delete_comment_operation op;
    op.author = c1.author();
    op.permlink = c1.permlink();

    BOOST_REQUIRE_THROW(push_operation(op), fc::assert_exception);
    BOOST_CHECK_EQUAL((db.get_index<comment_index, by_id>().size()), 2);
}

SCORUM_TEST_CASE(shouldnt_delete_post_after_vote)
{
    auto p = create_post(alice).in_block();
    BOOST_CHECK_EQUAL((db.get_index<comment_index, by_id>().size()), 1);

    p.vote(sam).in_block();

    delete_comment_operation op;
    op.author = p.author();
    op.permlink = p.permlink();

    BOOST_REQUIRE_THROW(push_operation(op), fc::assert_exception);
    BOOST_CHECK_EQUAL((db.get_index<comment_index, by_id>().size()), 1);
}

BOOST_AUTO_TEST_SUITE_END()
}
