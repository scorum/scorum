#pragma once

#include <scorum/tags/tags_objects.hpp>
#include <scorum/tags/tags_api.hpp>
#include <scorum/tags/tags_plugin.hpp>

#include "database_blog_integration.hpp"

namespace database_fixture {

using namespace scorum::chain;
using namespace scorum::protocol;
using namespace scorum::app;
using namespace scorum::tags;

struct tags_fixture : public database_blog_integration_fixture
{
    api_context _api_ctx;
    scorum::tags::tags_api _api;

    Actor alice = Actor("alice");
    Actor bob = Actor("bob");
    Actor sam = Actor("sam");
    Actor dave = Actor("dave");

    tags_fixture()
        : _api_ctx(app, TAGS_API_NAME, std::make_shared<api_session_data>())
        , _api(_api_ctx)
    {
        init_plugin<scorum::tags::tags_plugin>();

        open_database();

        actor(initdelegate).create_account(alice);
        actor(initdelegate).create_account(bob);
        actor(initdelegate).create_account(sam);
        actor(initdelegate).create_account(dave);
    }
};
}
