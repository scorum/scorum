#include <boost/test/unit_test.hpp>

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

            fixture.generate_blocks(20 / SCORUM_BLOCK_INTERVAL);

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

    template <typename Constructor> Comment create_post(Actor& actor, Constructor&& c)
    {
        comment_operation operation;
        operation.author = actor.name;

        c(operation);

        if (operation.permlink.empty())
            operation.permlink = title_to_permlink(operation.title);

        if (operation.parent_permlink.empty())
            operation.parent_permlink = "category";

        push_operation<comment_operation>(operation, actor.private_key);

        generate_blocks(5 * 60 / SCORUM_BLOCK_INTERVAL);

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
    api::discussion_query query;
    query.limit = 1;

    BOOST_REQUIRE_EQUAL(_api.get_discussions_by_created(query).size(), 0u);

    create_post(initdelegate, [](comment_operation& op) {
        op.title = "root post";
        op.body = "body";
    });

    BOOST_REQUIRE_EQUAL(_api.get_discussions_by_created(query).size(), 1u);
}

SCORUM_TEST_CASE(test_comments_depth_counter)
{
    auto get_comments_with_depth = [](scorum::chain::database& db) {
        std::map<std::string, uint16_t> result;

        const auto& index = db.get_index<comment_index>().indices().get<by_parent>();

        for (auto itr = index.begin(); itr != index.end(); ++itr)
        {
            result.insert(std::make_pair(fc::to_string(itr->permlink), itr->depth));
        }

        return result;
    };

    auto root = create_post(initdelegate, [](comment_operation& op) {
        op.title = "root post";
        op.body = "body";
    });

    auto root_child = root.create_comment(initdelegate, [](comment_operation& op) {
        op.title = "child one";
        op.body = "body";
    });

    auto root_child_child = root_child.create_comment(initdelegate, [](comment_operation& op) {
        op.title = "child two";
        op.body = "body";
    });

    auto check_list = get_comments_with_depth(this->db);

    BOOST_REQUIRE_EQUAL(0u, check_list[root.permlink()]);
    BOOST_REQUIRE_EQUAL(1u, check_list[root_child.permlink()]);
    BOOST_REQUIRE_EQUAL(2u, check_list[root_child_child.permlink()]);
}

SCORUM_TEST_CASE(get_discussions_by_created_return_post_and_it_comments_if_its_id_0)
{
    auto root = create_post(initdelegate, [](comment_operation& op) {
        op.title = "root";
        op.body = "body";
    });

    auto comment_level_1 = root.create_comment(initdelegate, [](comment_operation& op) {
        op.title = "level 1";
        op.body = "body";
    });

    api::discussion_query query;
    query.limit = 100;

    BOOST_REQUIRE_EQUAL(_api.get_discussions_by_created(query).size(), 2u);
}

SCORUM_TEST_CASE(get_discussions_by_created_return_two_posts)
{
    create_post(initdelegate, [](comment_operation& op) {
        op.title = "zero";
        op.body = "post";
    });

    auto root = create_post(initdelegate, [](comment_operation& op) {
        op.title = "one";
        op.body = "post";
    });

    auto comment_level_1 = root.create_comment(initdelegate, [](comment_operation& op) {
        op.title = "level 1";
        op.body = "body";
    });

    auto comment_level_2 = comment_level_1.create_comment(initdelegate, [](comment_operation& op) {
        op.title = "level 2";
        op.body = "body";
    });

    api::discussion_query query;
    query.limit = 100;

    const auto discussions = _api.get_discussions_by_created(query);

    BOOST_CHECK_EQUAL(discussions.size(), 2u);

    BOOST_CHECK_EQUAL(discussions[0].permlink, "one");
    BOOST_CHECK_EQUAL(discussions[1].permlink, "zero");
}

SCORUM_TEST_CASE(post_without_tags_creates_one_empty_tag)
{
    auto& index = db.get_index<scorum::tags::tag_index>().indices().get<scorum::tags::by_comment>();

    BOOST_REQUIRE_EQUAL(0u, index.size());

    create_post(initdelegate, [](comment_operation& op) {
        op.title = "zero";
        op.body = "post";
    });

    BOOST_REQUIRE_EQUAL(1u, index.size());

    auto itr = index.begin();

    BOOST_CHECK_EQUAL(itr->tag, "");
}

SCORUM_TEST_CASE(create_tags_from_json_metadata)
{
    create_post(initdelegate, [](comment_operation& op) {
        op.title = "zero";
        op.body = "post";
        op.json_metadata = "{\"tags\" : [\"football\", \"tenis\"]}";
    });

    auto& index = db.get_index<scorum::tags::tag_index>().indices().get<scorum::tags::by_comment>();
    BOOST_REQUIRE_EQUAL(3u, index.size());

    auto itr = index.begin();

    BOOST_CHECK_EQUAL(itr->tag, "");

    itr++;

    BOOST_CHECK_EQUAL(itr->tag, "football");

    itr++;

    BOOST_CHECK_EQUAL(itr->tag, "tenis");
}

SCORUM_TEST_CASE(create_two_posts_with_same_tags)
{
    create_post(initdelegate, [](comment_operation& op) {
        op.title = "zero";
        op.body = "post";
        op.json_metadata = "{\"tags\" : [\"football\"]}";
    });

    create_post(initdelegate, [](comment_operation& op) {
        op.title = "one";
        op.body = "post";
        op.json_metadata = "{\"tags\" : [\"football\"]}";
    });

    auto& index = db.get_index<scorum::tags::tag_index>().indices().get<scorum::tags::by_comment>();
    BOOST_REQUIRE_EQUAL(4u, index.size());

    auto itr = index.begin();

    BOOST_CHECK_EQUAL(itr->tag, "");
    BOOST_CHECK(itr->comment == 0u);

    itr++;

    BOOST_CHECK_EQUAL(itr->tag, "football");
    BOOST_CHECK(itr->comment == 0u);

    itr++;

    BOOST_CHECK_EQUAL(itr->tag, "");
    BOOST_CHECK(itr->comment == 1u);

    itr++;

    BOOST_CHECK_EQUAL(itr->tag, "football");
    BOOST_CHECK(itr->comment == 1u);
}

BOOST_AUTO_TEST_SUITE_END()

} // namespace tags_tests
