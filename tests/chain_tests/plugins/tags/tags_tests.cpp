#include <boost/test/unit_test.hpp>

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

    template <typename OperationType, typename Constructor> OperationType push(private_key_type& key, Constructor&& c)
    {
        OperationType op;
        c(op);

        push_operation<OperationType>(op, key);

        return op;
    }

    comment_operation create_post()
    {
        return push<comment_operation>(initdelegate.private_key, [](comment_operation& op) {
            op.author = initdelegate.name;
            op.permlink = "root";
            op.parent_permlink = "football";
            op.title = "Post title";
            op.body = "Post body";
            op.json_metadata = "{\"tags\" : [\"football\"]}";
        });
    }
};

BOOST_FIXTURE_TEST_SUITE(tags_tests, tags_fixture)

SCORUM_TEST_CASE(get_discussions_by_created)
{
    {
        api::discussion_query query;
        query.limit = 1;

        BOOST_REQUIRE_EQUAL(_api.get_discussions_by_created(query).size(), 0u);

        create_post();

        BOOST_REQUIRE_EQUAL(_api.get_discussions_by_created(query).size(), 1u);
    }
}

BOOST_AUTO_TEST_SUITE_END()

} // namespace tags_tests
