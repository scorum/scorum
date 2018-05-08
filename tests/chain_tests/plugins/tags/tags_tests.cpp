#include <boost/test/unit_test.hpp>

#include "tags_common.hpp"

namespace tags_tests {

BOOST_AUTO_TEST_SUITE(title_to_permlink_tests)

SCORUM_TEST_CASE(replace_spaces_with_minus)
{
    BOOST_CHECK_EQUAL("one-two-three", tags_fixture::title_to_permlink("one two three"));
}

SCORUM_TEST_CASE(replace_dot_and_coma_with_minus)
{
    BOOST_CHECK_EQUAL("one-two-three", tags_fixture::title_to_permlink("one.two,three"));
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

/// TODO: this is wrong behaviour, `get_discussions_by_created` should return only posts when
/// parent_author & parent_permlink is not set.
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

#ifndef IS_LOW_MEM
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
#endif

BOOST_AUTO_TEST_SUITE_END()

} // namespace tags_tests
