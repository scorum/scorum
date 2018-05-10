#ifndef IS_LOW_MEM

#include <boost/test/unit_test.hpp>

#include "tags_common.hpp"

namespace std {

std::ostream& operator<<(std::ostream& os, const std::pair<std::string, unsigned int>& p)
{
    os << '[' << p.first << ',' << p.second << ']';
    return os;
}

std::ostringstream& operator<<(std::ostringstream& os, const std::pair<std::string, unsigned int>& p)
{
    static_cast<std::ostream&>(os) << p;
    return os;
}
}

namespace tags_tests {

struct get_tags_by_category_fixture : public tags_fixture
{
    Actor alice = Actor("alice");
    Actor bob = Actor("bob");
    Actor sam = Actor("sam");
    Actor dave = Actor("dave");

    get_tags_by_category_fixture()
    {
        actor(initdelegate).create_account(alice);
        actor(initdelegate).create_account(bob);
        actor(initdelegate).create_account(sam);
        actor(initdelegate).create_account(dave);
    }
};

BOOST_FIXTURE_TEST_SUITE(get_tags_by_category_tests, get_tags_by_category_fixture)

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
}

#endif
