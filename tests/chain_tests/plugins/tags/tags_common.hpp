#pragma once

#include <boost/algorithm/string.hpp>

#include <scorum/tags/tags_objects.hpp>
#include <scorum/tags/tags_api.hpp>
#include <scorum/tags/tags_plugin.hpp>

#include <scorum/protocol/scorum_operations.hpp>

#include "database_trx_integration.hpp"

namespace tags_tests {

using namespace scorum::chain;
using namespace scorum::protocol;
using namespace scorum::app;
using namespace scorum::tags;

struct tags_fixture : public database_fixture::database_trx_integration_fixture
{
    static std::string title_to_permlink(const std::string& title)
    {
        std::string permlink = title;

        std::replace_if(permlink.begin(), permlink.end(), [](char ch) { return !std::isalnum(ch); }, '-');

        return permlink;
    }

    class Comment
    {
    public:
        Comment(const comment_operation& op, tags_fixture* f)
            : my(op)
            , fixture(f)
        {
        }

        Comment& operator=(const Comment& c)
        {
            this->fixture = c.fixture;
            this->my = c.my;

            return *this;
        }

        std::string title() const
        {
            return my.title;
        }

        std::string body() const
        {
            return my.body;
        }

        std::string author() const
        {
            return my.author;
        }

        std::string permlink() const
        {
            return my.permlink;
        }

        template <typename Constructor> Comment create_comment(Actor& actor, Constructor&& c)
        {
            comment_operation operation;
            operation.author = actor.name;
            operation.parent_author = my.author;
            operation.parent_permlink = my.permlink;
            c(operation);

            if (operation.permlink.empty())
                operation.permlink = tags_fixture::title_to_permlink(operation.title);

            fixture->push_operation<comment_operation>(operation, actor.private_key);

            fixture->generate_blocks(fixture->db.head_block_time() + SCORUM_MIN_REPLY_INTERVAL);

            return Comment(operation, fixture);
        }

    private:
        comment_operation my;
        tags_fixture* fixture;
    };

    api_context _api_ctx;
    scorum::tags::tags_api _api;

    Actor alice;

    tags_fixture()
        : _api_ctx(app, TAGS_API_NAME, std::make_shared<api_session_data>())
        , _api(_api_ctx)
    {
        init_plugin<scorum::tags::tags_plugin>();

        open_database();
    }

    template <typename Constructor> Comment create_post(Actor& actor, Constructor&& c)
    {
        comment_operation operation;
        operation.author = actor.name;

        c(operation);

        if (operation.permlink.empty())
            operation.permlink = tags_fixture::title_to_permlink(operation.title);

        if (operation.parent_permlink.empty())
            operation.parent_permlink = "category";

        push_operation<comment_operation>(operation, actor.private_key);

        generate_blocks(db.head_block_time() + SCORUM_MIN_ROOT_COMMENT_INTERVAL);

        return Comment(operation, this);
    }
};
}
