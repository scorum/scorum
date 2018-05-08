#include <boost/test/unit_test.hpp>

#include <boost/algorithm/string.hpp>

#include <scorum/tags/tags_api.hpp>
#include <scorum/tags/tags_plugin.hpp>

#include <scorum/protocol/scorum_operations.hpp>

#include "database_trx_integration.hpp"
#include <sstream>

namespace boost {

std::ostringstream& operator<<(std::ostringstream& os, const std::pair<std::string, unsigned int>& p)
{
    os << '[' << p.first << ',' << p.second << ']';
    return os;
}
}

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

        void remove(Actor& actor)
        {
            delete_comment_operation op;
            op.author = author();
            op.permlink = permlink();

            fixture.push_operation<delete_comment_operation>(op, actor.private_key);

            fixture.generate_blocks(20 / SCORUM_BLOCK_INTERVAL);
        }

    private:
        comment_operation my;
        tags_fixture& fixture;
    };

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

        generate_blocks(20 / SCORUM_BLOCK_INTERVAL);

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

        create_post(initdelegate, [](comment_operation& op) {
            op.title = "root post";
            op.body = "body";
        });

        BOOST_REQUIRE_EQUAL(_api.get_discussions_by_created(query).size(), 1u);
    }
}

SCORUM_TEST_CASE(test_depth)
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

BOOST_AUTO_TEST_SUITE_END()

BOOST_FIXTURE_TEST_SUITE(get_tags_by_category_tests, tags_fixture)

SCORUM_TEST_CASE(check_couple_categories_several_tags)
{
    create_post(alice, [](comment_operation& op) {
        op.title = "post1";
        op.body = "body";
        op.parent_author = SCORUM_ROOT_POST_PARENT_ACCOUNT;
        op.parent_permlink = "category1";
        op.json_metadata = R"({"tags": ["tag1","tag2","tag3"]})";
    });

    create_post(bob, [](comment_operation& op) {
        op.title = "post2";
        op.body = "body";
        op.parent_author = SCORUM_ROOT_POST_PARENT_ACCOUNT;
        op.parent_permlink = "category1";
        op.json_metadata = R"({"tags": ["tag2","tag3","tag4"]})";
    });

    create_post(sam, [](comment_operation& op) {
        op.title = "post2";
        op.body = "body";
        op.parent_author = SCORUM_ROOT_POST_PARENT_ACCOUNT;
        op.parent_permlink = "category2";
        op.json_metadata = R"({"tags": ["tag1","tag2","tag3"]})";
    });

    create_post(dave, [](comment_operation& op) {
        op.title = "post3";
        op.body = "body";
        op.parent_author = SCORUM_ROOT_POST_PARENT_ACCOUNT;
        op.parent_permlink = "category2";
        op.json_metadata = R"({"tags": ["tag3","tag4","tag5"]})";
    });

    auto cat1_tags = _api.get_tags_by_category("category1");
    auto cat2_tags = _api.get_tags_by_category("category2");
    auto cat3_tags = _api.get_tags_by_category("category3");

    // clang-format off
    std::vector<std::pair<std::string, uint32_t>> cat1_ethalon = { { "tag3", 2 }, { "tag2", 2 }, { "tag4", 1 }, { "tag1", 1 } };
    std::vector<std::pair<std::string, uint32_t>> cat2_ethalon = { { "tag3", 2 }, { "tag5", 1 }, { "tag4", 1 }, { "tag2", 1 }, { "tag1", 1 } };
    std::vector<std::pair<std::string, uint32_t>> cat3_ethalon = { };
    // clang-format on

    BOOST_REQUIRE_EQUAL(cat1_tags.size(), cat1_ethalon.size());
    BOOST_REQUIRE_EQUAL(cat2_tags.size(), cat2_ethalon.size());
    BOOST_REQUIRE_EQUAL(cat3_tags.size(), cat3_ethalon.size());

    BOOST_REQUIRE_EQUAL_COLLECTIONS(cat1_tags.begin(), cat1_tags.end(), cat1_ethalon.begin(), cat1_ethalon.end());
    BOOST_REQUIRE_EQUAL_COLLECTIONS(cat2_tags.begin(), cat2_tags.end(), cat2_ethalon.begin(), cat2_ethalon.end());
    BOOST_REQUIRE_EQUAL_COLLECTIONS(cat3_tags.begin(), cat3_tags.end(), cat3_ethalon.begin(), cat3_ethalon.end());
}

SCORUM_TEST_CASE(check_no_posts)
{
    auto cat1_tags = _api.get_tags_by_category("category1");
    BOOST_REQUIRE_EQUAL(cat1_tags.size(), 0ul);
}

SCORUM_TEST_CASE(check_post_removed)
{
    create_post(alice, [](comment_operation& op) {
        op.title = "post1";
        op.body = "body";
        op.parent_author = SCORUM_ROOT_POST_PARENT_ACCOUNT;
        op.parent_permlink = "category1";
        op.json_metadata = R"({"tags": ["tag1","tag2","tag3"]})";
    });

    auto post2 = create_post(bob, [](comment_operation& op) {
        op.title = "post2";
        op.body = "body";
        op.parent_author = SCORUM_ROOT_POST_PARENT_ACCOUNT;
        op.parent_permlink = "category1";
        op.json_metadata = R"({"tags": ["tag2","tag3","tag4"]})";
    });

    auto cat_tags_before = _api.get_tags_by_category("category1");

    std::vector<std::pair<std::string, uint32_t>> cat_tags_before_ethalon
        = { { "tag3", 2 }, { "tag2", 2 }, { "tag4", 1 }, { "tag1", 1 } };

    BOOST_REQUIRE_EQUAL(cat_tags_before.size(), cat_tags_before_ethalon.size());
    BOOST_REQUIRE_EQUAL_COLLECTIONS(cat_tags_before.begin(), cat_tags_before.end(), cat_tags_before_ethalon.begin(),
                                    cat_tags_before_ethalon.end());

    post2.remove(initdelegate);

    auto cat_tags_after = _api.get_tags_by_category("category1");

    std::vector<std::pair<std::string, uint32_t>> cat_tags_after_ethalon
        = { { "tag3", 1 }, { "tag2", 1 }, { "tag1", 1 } };

    BOOST_REQUIRE_EQUAL(cat_tags_after.size(), cat_tags_after_ethalon.size());
    BOOST_REQUIRE_EQUAL_COLLECTIONS(cat_tags_after.begin(), cat_tags_after.end(), cat_tags_after_ethalon.begin(),
                                    cat_tags_after_ethalon.end());
}

SCORUM_TEST_CASE(check_posts_tags_changed)
{
    create_post(alice, [](comment_operation& op) {
        op.title = "post1";
        op.body = "body";
        op.parent_author = SCORUM_ROOT_POST_PARENT_ACCOUNT;
        op.parent_permlink = "category1";
        op.json_metadata = R"({"tags": ["tag1","tag2","tag3"]})";
    });

    create_post(bob, [](comment_operation& op) {
        op.title = "post2";
        op.body = "body";
        op.parent_author = SCORUM_ROOT_POST_PARENT_ACCOUNT;
        op.parent_permlink = "category1";
        op.json_metadata = R"({"tags": ["tag2","tag3","tag4"]})";
    });

    auto cat_tags_before = _api.get_tags_by_category("category1");

    std::vector<std::pair<std::string, uint32_t>> cat_tags_before_ethalon
        = { { "tag3", 2 }, { "tag2", 2 }, { "tag4", 1 }, { "tag1", 1 } };

    BOOST_REQUIRE_EQUAL(cat_tags_before.size(), cat_tags_before_ethalon.size());
    BOOST_REQUIRE_EQUAL_COLLECTIONS(cat_tags_before.begin(), cat_tags_before.end(), cat_tags_before_ethalon.begin(),
                                    cat_tags_before_ethalon.end());

    // changing post2's tags
    create_post(bob, [](comment_operation& op) {
        op.title = "post2";
        op.body = "body";
        op.parent_author = SCORUM_ROOT_POST_PARENT_ACCOUNT;
        op.parent_permlink = "category1";
        op.json_metadata = R"({"tags": ["tag1","tag2","tag4"]})";
    });

    auto cat_tags_after = _api.get_tags_by_category("category1");

    std::vector<std::pair<std::string, uint32_t>> cat_tags_after_ethalon
        = { { "tag2", 2 }, { "tag1", 2 }, { "tag4", 1 }, { "tag3", 1 } };

    BOOST_REQUIRE_EQUAL(cat_tags_after.size(), cat_tags_after_ethalon.size());
    BOOST_REQUIRE_EQUAL_COLLECTIONS(cat_tags_after.begin(), cat_tags_after.end(), cat_tags_after_ethalon.begin(),
                                    cat_tags_after_ethalon.end());
}

BOOST_AUTO_TEST_SUITE_END()

} // namespace tags_tests
