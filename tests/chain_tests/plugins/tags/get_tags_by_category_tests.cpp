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

SCORUM_TEST_CASE(check_empty_input_should_assert)
{
    BOOST_REQUIRE_THROW(_api.get_tags_by_category("", "c1"), fc::assert_exception);
    BOOST_REQUIRE_THROW(_api.get_tags_by_category("d1", ""), fc::assert_exception);
    BOOST_REQUIRE_THROW(_api.get_tags_by_category("", ""), fc::assert_exception);
}

SCORUM_TEST_CASE(check_couple_categories_several_tags)
{
    // d1/c1
    create_post(alice, [](comment_operation& op) {
        op.title = "post1";
        op.body = "body";
        op.parent_author = SCORUM_ROOT_POST_PARENT_ACCOUNT;
        op.parent_permlink = "c1";
        op.json_metadata = R"({"domain": "d1", "category": "c1", "tags": ["c1", "d1", "tag1","tag2","tag3"]})";
    });

    // d1/c1
    create_post(bob, [](comment_operation& op) {
        op.title = "post2";
        op.body = "body";
        op.parent_author = SCORUM_ROOT_POST_PARENT_ACCOUNT;
        op.parent_permlink = "c1";
        op.json_metadata = R"({"domain": "d1", "category": "c1", "tags": ["c1", "d1", "tag2","tag3","tag4"]})";
    });

    // d1/c2
    create_post(sam, [](comment_operation& op) {
        op.title = "post2";
        op.body = "body";
        op.parent_author = SCORUM_ROOT_POST_PARENT_ACCOUNT;
        op.parent_permlink = "c2";
        op.json_metadata = R"({"domain": "d1", "category": "c2", "tags": ["c2", "d1", "tag1","tag2","tag3"]})";
    });

    // d1/c2
    create_post(dave, [](comment_operation& op) {
        op.title = "post3";
        op.body = "body";
        op.parent_author = SCORUM_ROOT_POST_PARENT_ACCOUNT;
        op.parent_permlink = "c2";
        op.json_metadata = R"({"domain": "d1", "category": "c2", "tags": ["c2", "d1", "tag3","tag4","tag5"]})";
    });

    auto cat1_tags = _api.get_tags_by_category("d1", "c1");
    auto cat2_tags = _api.get_tags_by_category("d1", "c2");
    auto cat3_tags = _api.get_tags_by_category("d1", "c3");

    // clang-format off
    std::vector<std::pair<std::string, uint32_t>> cat1_expected = { { "tag3", 2 }, { "tag2", 2 }, { "tag4", 1 }, { "tag1", 1 } };
    std::vector<std::pair<std::string, uint32_t>> cat2_expected = { { "tag3", 2 }, { "tag5", 1 }, { "tag4", 1 }, { "tag2", 1 }, { "tag1", 1 } };
    std::vector<std::pair<std::string, uint32_t>> cat3_expected = { };
    // clang-format on

    BOOST_REQUIRE_EQUAL(cat1_tags.size(), cat1_expected.size());
    BOOST_REQUIRE_EQUAL(cat2_tags.size(), cat2_expected.size());
    BOOST_REQUIRE_EQUAL(cat3_tags.size(), cat3_expected.size());

    BOOST_REQUIRE_EQUAL_COLLECTIONS(cat1_tags.begin(), cat1_tags.end(), cat1_expected.begin(), cat1_expected.end());
    BOOST_REQUIRE_EQUAL_COLLECTIONS(cat2_tags.begin(), cat2_tags.end(), cat2_expected.begin(), cat2_expected.end());
    BOOST_REQUIRE_EQUAL_COLLECTIONS(cat3_tags.begin(), cat3_tags.end(), cat3_expected.begin(), cat3_expected.end());
}

SCORUM_TEST_CASE(check_different_domain_same_category)
{
    // d1/c1
    create_post(alice, [](comment_operation& op) {
        op.title = "post1";
        op.body = "body";
        op.parent_author = SCORUM_ROOT_POST_PARENT_ACCOUNT;
        op.parent_permlink = "c1";
        op.json_metadata = R"({"domain": "d1", "category": "c1", "tags": ["c1", "d1", "tag1","tag2","tag3"]})";
    });

    // d2/c1
    create_post(bob, [](comment_operation& op) {
        op.title = "post2";
        op.body = "body";
        op.parent_author = SCORUM_ROOT_POST_PARENT_ACCOUNT;
        op.parent_permlink = "c1";
        op.json_metadata = R"({"domain": "d2", "category": "c1", "tags": ["c1", "d2", "tag2","tag3","tag4"]})";
    });

    // d2/c1
    create_post(sam, [](comment_operation& op) {
        op.title = "post3";
        op.body = "body";
        op.parent_author = SCORUM_ROOT_POST_PARENT_ACCOUNT;
        op.parent_permlink = "c1";
        op.json_metadata = R"({"domain": "d2", "category": "c1", "tags": ["c1", "d2", "tag4","tag5"]})";
    });

    auto cat1_domain1_tags = _api.get_tags_by_category("d1", "c1");
    auto cat1_domain2_tags = _api.get_tags_by_category("d2", "c1");

    // clang-format off
    std::vector<std::pair<std::string, uint32_t>> cat1_domain1_expected = { { "tag3", 1 }, { "tag2", 1 }, { "tag1", 1 } };
    std::vector<std::pair<std::string, uint32_t>> cat1_domain2_expected = { { "tag4", 2 }, { "tag5", 1 }, { "tag3", 1 }, { "tag2", 1 } };
    // clang-format on

    BOOST_REQUIRE_EQUAL(cat1_domain1_tags.size(), cat1_domain1_expected.size());
    BOOST_REQUIRE_EQUAL(cat1_domain2_tags.size(), cat1_domain2_expected.size());

    BOOST_REQUIRE_EQUAL_COLLECTIONS(cat1_domain1_tags.begin(), cat1_domain1_tags.end(), cat1_domain1_expected.begin(),
                                    cat1_domain1_expected.end());
    BOOST_REQUIRE_EQUAL_COLLECTIONS(cat1_domain2_tags.begin(), cat1_domain2_tags.end(), cat1_domain2_expected.begin(),
                                    cat1_domain2_expected.end());
}

SCORUM_TEST_CASE(check_same_domain_different_category)
{
    // d1/c1
    create_post(alice, [](comment_operation& op) {
        op.title = "post1";
        op.body = "body";
        op.parent_author = SCORUM_ROOT_POST_PARENT_ACCOUNT;
        op.parent_permlink = "c1";
        op.json_metadata = R"({"domain": "d1", "category": "c1", "tags": ["c1", "d1", "tag1","tag2","tag3"]})";
    });

    // d2/c1
    create_post(bob, [](comment_operation& op) {
        op.title = "post2";
        op.body = "body";
        op.parent_author = SCORUM_ROOT_POST_PARENT_ACCOUNT;
        op.parent_permlink = "c2";
        op.json_metadata = R"({"domain": "d1", "category": "c2", "tags": ["c2", "d1", "tag2","tag3","tag4"]})";
    });

    // d2/c1
    create_post(sam, [](comment_operation& op) {
        op.title = "post3";
        op.body = "body";
        op.parent_author = SCORUM_ROOT_POST_PARENT_ACCOUNT;
        op.parent_permlink = "c2";
        op.json_metadata = R"({"domain": "d1", "category": "c2", "tags": ["c2", "d1", "tag4","tag5"]})";
    });

    auto cat1_domain1_tags = _api.get_tags_by_category("d1", "c1");
    auto cat1_domain2_tags = _api.get_tags_by_category("d1", "c2");

    // clang-format off
    std::vector<std::pair<std::string, uint32_t>> cat1_domain1_expected = { { "tag3", 1 }, { "tag2", 1 }, { "tag1", 1 } };
    std::vector<std::pair<std::string, uint32_t>> cat1_domain2_expected = { { "tag4", 2 }, { "tag5", 1 }, { "tag3", 1 }, { "tag2", 1 } };
    // clang-format on

    BOOST_REQUIRE_EQUAL(cat1_domain1_tags.size(), cat1_domain1_expected.size());
    BOOST_REQUIRE_EQUAL(cat1_domain2_tags.size(), cat1_domain2_expected.size());

    BOOST_REQUIRE_EQUAL_COLLECTIONS(cat1_domain1_tags.begin(), cat1_domain1_tags.end(), cat1_domain1_expected.begin(),
                                    cat1_domain1_expected.end());
    BOOST_REQUIRE_EQUAL_COLLECTIONS(cat1_domain2_tags.begin(), cat1_domain2_tags.end(), cat1_domain2_expected.begin(),
                                    cat1_domain2_expected.end());
}

SCORUM_TEST_CASE(check_no_posts)
{
    auto cat1_tags = _api.get_tags_by_category("d1", "c1");
    BOOST_REQUIRE_EQUAL(cat1_tags.size(), 0ul);
}

SCORUM_TEST_CASE(check_tags_doesnt_contain_category_and_domain)
{
    create_post(alice, [](comment_operation& op) {
        op.title = "post1";
        op.body = "body";
        op.parent_author = SCORUM_ROOT_POST_PARENT_ACCOUNT;
        op.parent_permlink = "c1";
        op.json_metadata = R"({"domain": "d1", "category": "c1", "tags": ["tag1","tag2","tag3"]})";
    });

    auto tags = _api.get_tags_by_category("d1", "c1");

    std::vector<std::pair<std::string, uint32_t>> tags_expected = { { "tag3", 1 }, { "tag2", 1 }, { "tag1", 1 } };

    BOOST_REQUIRE_EQUAL(tags.size(), tags_expected.size());
    BOOST_REQUIRE_EQUAL_COLLECTIONS(tags.begin(), tags.end(), tags_expected.begin(), tags_expected.end());
}

SCORUM_TEST_CASE(check_json_metadata_doesnt_contain_domain)
{
    create_post(alice, [](comment_operation& op) {
        op.title = "post1";
        op.body = "body";
        op.parent_author = SCORUM_ROOT_POST_PARENT_ACCOUNT;
        op.parent_permlink = "c1";
        op.json_metadata = R"({"category": "c1", "tags": ["tag1","tag2","tag3"]})";
    });

    BOOST_REQUIRE_EQUAL((db.get_index<category_stats_index, by_category>().size()), 0u);
}

SCORUM_TEST_CASE(check_json_metadata_doesnt_contain_category)
{
    create_post(alice, [](comment_operation& op) {
        op.title = "post1";
        op.body = "body";
        op.parent_author = SCORUM_ROOT_POST_PARENT_ACCOUNT;
        op.parent_permlink = "c1";
        op.json_metadata = R"({"domain": "d1", "tags": ["tag1","tag2","tag3"]})";
    });

    BOOST_REQUIRE_EQUAL((db.get_index<category_stats_index, by_category>().size()), 0u);
}

SCORUM_TEST_CASE(check_post_removed)
{
    // d1/c1
    create_post(alice, [](comment_operation& op) {
        op.title = "post1";
        op.body = "body";
        op.parent_author = SCORUM_ROOT_POST_PARENT_ACCOUNT;
        op.parent_permlink = "c1";
        op.json_metadata = R"({"domain": "d1", "category": "c1", "tags": ["c1", "d1", "tag1","tag2","tag3"]})";
    });

    // d1/c1
    auto post2 = create_post(bob, [](comment_operation& op) {
        op.title = "post2";
        op.body = "body";
        op.parent_author = SCORUM_ROOT_POST_PARENT_ACCOUNT;
        op.parent_permlink = "c1";
        op.json_metadata = R"({"domain": "d1", "category": "c1", "tags": ["c1", "d1", "tag2","tag3","tag4"]})";
    });

    auto cat_tags_before = _api.get_tags_by_category("d1", "c1");

    std::vector<std::pair<std::string, uint32_t>> cat_tags_before_expected
        = { { "tag3", 2 }, { "tag2", 2 }, { "tag4", 1 }, { "tag1", 1 } };

    BOOST_REQUIRE_EQUAL(cat_tags_before.size(), cat_tags_before_expected.size());
    BOOST_REQUIRE_EQUAL_COLLECTIONS(cat_tags_before.begin(), cat_tags_before.end(), cat_tags_before_expected.begin(),
                                    cat_tags_before_expected.end());

    post2.remove(initdelegate);

    auto cat_tags_after = _api.get_tags_by_category("d1", "c1");

    std::vector<std::pair<std::string, uint32_t>> cat_tags_after_expected
        = { { "tag3", 1 }, { "tag2", 1 }, { "tag1", 1 } };

    BOOST_REQUIRE_EQUAL(cat_tags_after.size(), cat_tags_after_expected.size());
    BOOST_REQUIRE_EQUAL_COLLECTIONS(cat_tags_after.begin(), cat_tags_after.end(), cat_tags_after_expected.begin(),
                                    cat_tags_after_expected.end());
}

SCORUM_TEST_CASE(check_posts_tags_changed)
{
    // d1/c1
    create_post(alice, [](comment_operation& op) {
        op.title = "post1";
        op.body = "body";
        op.parent_author = SCORUM_ROOT_POST_PARENT_ACCOUNT;
        op.parent_permlink = "c1";
        op.json_metadata = R"({"domain": "d1", "category": "c1", "tags": ["c1", "d1", "tag1","tag2","tag3"]})";
    });

    // d1/c1
    create_post(bob, [](comment_operation& op) {
        op.title = "post2";
        op.body = "body";
        op.parent_author = SCORUM_ROOT_POST_PARENT_ACCOUNT;
        op.parent_permlink = "c1";
        op.json_metadata = R"({"domain": "d1", "category": "c1", "tags": ["c1", "d1", "tag2","tag3","tag4"]})";
    });

    auto cat_tags_before = _api.get_tags_by_category("d1", "c1");

    std::vector<std::pair<std::string, uint32_t>> cat_tags_before_expected
        = { { "tag3", 2 }, { "tag2", 2 }, { "tag4", 1 }, { "tag1", 1 } };

    BOOST_REQUIRE_EQUAL(cat_tags_before.size(), cat_tags_before_expected.size());
    BOOST_REQUIRE_EQUAL_COLLECTIONS(cat_tags_before.begin(), cat_tags_before.end(), cat_tags_before_expected.begin(),
                                    cat_tags_before_expected.end());

    // changing post2's tags
    create_post(bob, [](comment_operation& op) {
        op.title = "post2";
        op.body = "body";
        op.parent_author = SCORUM_ROOT_POST_PARENT_ACCOUNT;
        op.parent_permlink = "c1";
        op.json_metadata = R"({"domain": "d1", "category": "c1", "tags": ["c1", "d1", "tag1","tag2","tag4"]})";
    });

    auto cat_tags_after = _api.get_tags_by_category("d1", "c1");

    std::vector<std::pair<std::string, uint32_t>> cat_tags_after_expected
        = { { "tag2", 2 }, { "tag1", 2 }, { "tag4", 1 }, { "tag3", 1 } };

    BOOST_REQUIRE_EQUAL(cat_tags_after.size(), cat_tags_after_expected.size());
    BOOST_REQUIRE_EQUAL_COLLECTIONS(cat_tags_after.begin(), cat_tags_after.end(), cat_tags_after_expected.begin(),
                                    cat_tags_after_expected.end());
}

BOOST_AUTO_TEST_SUITE_END()
}

#endif
