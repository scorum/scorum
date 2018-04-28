#include <boost/test/unit_test.hpp>

#include <boost/algorithm/string.hpp>

#include <scorum/tags/tags_api.hpp>
#include <scorum/tags/tags_plugin.hpp>

#include <scorum/protocol/scorum_operations.hpp>

#include "database_trx_integration.hpp"

namespace tags_tests {

using namespace scorum::chain;
using namespace scorum::protocol;
using namespace scorum::app;
using namespace scorum::tags;

std::string title_to_permlink(const std::string& title)
{
    std::string permlink = title;

    std::replace_if(permlink.begin(), permlink.end(), [](char ch) { return !std::isalnum(ch); }, '-');

    return permlink;
}

struct tags_fixture : public database_fixture::database_trx_integration_fixture
{
    class Comment
    {
    public:
        Comment(const comment_operation& op, tags_fixture& f)
            : my(op)
            , fixture(f)
        {
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
                operation.permlink = title_to_permlink(operation.title);

            fixture.push_operation<comment_operation>(operation, actor.private_key);

            return Comment(operation, fixture);
        }

    private:
        comment_operation my;
        tags_fixture& fixture;
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

    template <typename OperationType, typename Constructor> OperationType push(private_key_type& key, Constructor&& c)
    {
        OperationType op;
        c(op);

        push_operation<OperationType>(op, key);

        return op;
    }

    Comment create_post(const std::string& title, const std::string& body)
    {
        comment_operation operation = push<comment_operation>(initdelegate.private_key, [&](comment_operation& op) {
            op.author = initdelegate.name;
            op.permlink = title_to_permlink(title);
            op.parent_permlink = "football";
            op.title = title;
            op.body = body;
            op.json_metadata = "{\"tags\" : [\"football\"]}";
        });

        return Comment(operation, *this);
    }
};

BOOST_AUTO_TEST_SUITE(title_to_permlink_tests)

SCORUM_TEST_CASE(replace_spaces_with_minus)
{
    BOOST_CHECK_EQUAL("one-two-three", title_to_permlink("one two three"));
}

SCORUM_TEST_CASE(replace_dot_and_coma_with_minus)
{
    BOOST_CHECK_EQUAL("one-two-three", title_to_permlink("one.two,three"));
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_FIXTURE_TEST_SUITE(tags_tests, tags_fixture)

SCORUM_TEST_CASE(get_discussions_by_created)
{
    {
        api::discussion_query query;
        query.limit = 1;

        BOOST_REQUIRE_EQUAL(_api.get_discussions_by_created(query).size(), 0u);

        create_post("title", "body");

        BOOST_REQUIRE_EQUAL(_api.get_discussions_by_created(query).size(), 1u);
    }
}

SCORUM_TEST_CASE(test_depth)
{
    {
        auto root = create_post("title", "body");

        auto root_child = root.create_comment(initdelegate, [](comment_operation& op) {
            op.title = "child one";
            op.body = "body";
        });

        auto root_child_child = root_child.create_comment(initdelegate, [](comment_operation& op) {
            op.title = "child two";
            op.body = "body";
        });

        const auto& index = this->db.get_index<comment_index>().indices().get<by_parent>();
        auto itr = index.begin();

        BOOST_REQUIRE_EQUAL(index.size(), 3u);

        BOOST_CHECK_EQUAL(itr->permlink, "title");
        BOOST_CHECK_EQUAL(itr->depth, 0);

        itr++;

        BOOST_CHECK_EQUAL(itr->permlink, "child-two");
        BOOST_CHECK_EQUAL(itr->depth, 2);

        itr++;

        BOOST_CHECK_EQUAL(itr->permlink, "child-one");
        BOOST_CHECK_EQUAL(itr->depth, 1);
    }
}

BOOST_AUTO_TEST_SUITE_END()

} // namespace tags_tests
