#pragma once
#include "tags_common.hpp"

namespace tags_tests {

using namespace scorum;

struct get_discussions_by_common : public tags_fixture
{
    using discussion = scorum::tags::api::discussion;

    get_discussions_by_common()
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
}
