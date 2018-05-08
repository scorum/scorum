#include "blogging_common.hpp"

#include <sstream>

namespace database_fixture {

blogging_common_fixture::blogging_common_fixture()
    : comment_service(db.comment_service())
    , comment_vote_service(db.comment_vote_service())
    , account_service(db.account_service())
    , dgp_service(db.dynamic_global_property_service())
{
}

std::string blogging_common_fixture::create_next_post_permlink()
{
    static int next = 0;
    std::stringstream store;
    store << "blog-" << ++next;
    return store.str();
}

std::string blogging_common_fixture::get_comment_permlink(const std::string& post_permlink)
{
    return std::string("re-") + post_permlink;
}

const comment_object& blogging_common_fixture::post(const Actor& author, const std::string& post_permlink)
{
    comment_operation comment;

    comment.author = author.name;
    comment.permlink = post_permlink;
    comment.parent_permlink = "posts";
    comment.title = "foo";
    comment.body = "bar";

    push_operation_only(comment, author.private_key);

    return comment_service.get(author.name, post_permlink);
}

const comment_object&
blogging_common_fixture::comment(const Actor& author_for_post, const std::string& post_permlink, const Actor& author)
{
    comment_operation comment;

    comment.author = author.name;
    comment.permlink = get_comment_permlink(post_permlink);
    comment.parent_author = author_for_post.name;
    comment.parent_permlink = post_permlink;
    comment.title = "re: foo";
    comment.body = "re: bar";

    push_operation_only(comment, author.private_key);

    return comment_service.get(author.name, comment.permlink);
}

const comment_vote_object&
blogging_common_fixture::vote(const Actor& author, const std::string& permlink, const Actor& voter, int wight /*= 100*/)
{
    vote_operation vote;

    vote.voter = voter.name;
    vote.author = author.name;
    vote.permlink = permlink;
    vote.weight = (int16_t)wight;

    push_operation_only(vote, voter.private_key);

    auto comment_id = comment_service.get(author.name, permlink).id;
    auto voter_id = account_service.get_account(voter.name).id;
    return comment_vote_service.get(comment_id, voter_id);
}

blogging_common_with_accounts_fixture::blogging_common_with_accounts_fixture()
    : alice("alice")
    , bob("bob")
    , sam("sam")
    , simon("simon")
{
    open_database();

    actor(initdelegate).create_account(alice);
    actor(initdelegate).give_scr(alice, feed_amount);
    actor(initdelegate).give_sp(alice, feed_amount);

    actor(initdelegate).create_account(bob);
    actor(initdelegate).give_scr(bob, feed_amount / 2);
    actor(initdelegate).give_sp(bob, feed_amount / 2);

    actor(initdelegate).create_account(sam);
    actor(initdelegate).give_scr(sam, feed_amount / 3);
    actor(initdelegate).give_sp(sam, feed_amount / 3);

    actor(initdelegate).create_account(simon);
    actor(initdelegate).give_scr(simon, feed_amount / 4);
    actor(initdelegate).give_sp(simon, feed_amount / 4);
}
}
