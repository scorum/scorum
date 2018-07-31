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

using namespace scorum::tags;

BOOST_FIXTURE_TEST_SUITE(get_tags_by_category_tests, database_fixture::tags_fixture)

SCORUM_TEST_CASE(check_empty_input_should_assert)
{
    BOOST_REQUIRE_THROW(_api.get_tags_by_category("", "c1"), fc::assert_exception);
    BOOST_REQUIRE_THROW(_api.get_tags_by_category("d1", ""), fc::assert_exception);
    BOOST_REQUIRE_THROW(_api.get_tags_by_category("", ""), fc::assert_exception);
}

SCORUM_TEST_CASE(check_empty_tag)
{
    create_post(alice, "c1")
        .set_json(R"({"domains": ["d1"], "categories": ["c1"], "tags": ["","tag2",""]})")
        .in_block();

    auto cat1_tags = _api.get_tags_by_category("d1", "c1");

    BOOST_REQUIRE_EQUAL(cat1_tags.size(), 1u);
}

SCORUM_TEST_CASE(check_comment_with_category_and_domain_in_upper_case)
{
    create_post(alice, "c1")
        .set_json(R"({"domains": ["d1"], "categories": ["c1"], "tags": ["tag1","tag2","tag3"]})")
        .in_block();

    auto cat1_tags = _api.get_tags_by_category("D1", "C1");

    BOOST_REQUIRE_EQUAL(cat1_tags.size(), 3u);
}

SCORUM_TEST_CASE(check_comment_with_russian_in_tags)
{
    create_post(alice, "c1")
        .set_json(R"({"domains": ["Домен1"], "categories": ["Категория1"], "tags": ["Тег1","Тег2"]})")
        .in_block();

    auto tags_lowercase = _api.get_tags_by_category("домен1", "категория1");
    auto tags_uppercase = _api.get_tags_by_category("ДОМЕН1", "КАТЕГОРИЯ1");

    std::vector<std::pair<std::string, uint32_t>> expected = { { "тег2", 1 }, { "тег1", 1 } };

    BOOST_REQUIRE_EQUAL(tags_lowercase.size(), 2u);
    BOOST_REQUIRE_EQUAL_COLLECTIONS(tags_lowercase.begin(), tags_lowercase.end(), expected.begin(), expected.end());

    BOOST_REQUIRE_EQUAL(tags_uppercase.size(), 2u);
    BOOST_REQUIRE_EQUAL_COLLECTIONS(tags_uppercase.begin(), tags_uppercase.end(), expected.begin(), expected.end());
}

SCORUM_TEST_CASE(check_tag_should_be_truncated_to_24symbols)
{
    create_post(alice, "c")
        .set_json(R"({"domains": ["d"], "categories": ["c"], "tags": ["ТеГ12ТеГ12ТеГ12ТеГ12ТеГ12ТеГ12"]})")
        .in_block();

    auto tags = _api.get_tags_by_category("d", "c");

    // expected tags should be <=24 symbols of length
    std::vector<std::pair<std::string, uint32_t>> expected = { { "тег12тег12тег12тег12тег1", 1 } };

    BOOST_REQUIRE_EQUAL(tags.size(), 1u);
    BOOST_CHECK_EQUAL_COLLECTIONS(tags.begin(), tags.end(), expected.begin(), expected.end());
}

SCORUM_TEST_CASE(check_category_should_be_truncated_to_24symbols)
{
    create_post(alice, "c")
        .set_json(
            R"({"domains": ["d"], "categories": ["手手手手手田田田田田手手手手手田田田田田手手手手手"], "tags": ["tag"]})")
        .in_block();

    BOOST_CHECK_EQUAL(_api.get_tags_by_category("d", "手手手手手田田田田田手手手手手田田田田田手手手手手").size(), 1u);
    BOOST_CHECK_EQUAL(_api.get_tags_by_category("d", "手手手手手田田田田田手手手手手田田田田田手手手手").size(), 1u);
    BOOST_CHECK_EQUAL(_api.get_tags_by_category("d", "手手手手手田田田田田手手手手手田田田田田手手手").size(), 0u);
}

SCORUM_TEST_CASE(check_domain_should_be_truncated_to_24symbols)
{
    create_post(alice, "cat")
        .set_json(R"({"domains": ["домендомендомендомендомен"], "categories": ["cat"], "tags": ["tag"]})")
        .in_block();

    BOOST_CHECK_EQUAL(_api.get_tags_by_category("домендомендомендомендомен", "cat").size(), 1u); // 25 symbols domain
    BOOST_CHECK_EQUAL(_api.get_tags_by_category("домендомендомендомендоме", "cat").size(), 1u); // 24 symbols domain
    BOOST_CHECK_EQUAL(_api.get_tags_by_category("домендомендомендомендом", "cat").size(), 0u); // 23 symbols domain
}

SCORUM_TEST_CASE(check_comment_with_chinese_in_tags)
{
    create_post(alice, "c1").set_json(R"({"domains": ["水"], "categories": ["田"], "tags": ["手手"]})").in_block();

    auto tags = _api.get_tags_by_category("水", "田");

    std::vector<std::pair<std::string, uint32_t>> expected = { { "手手", 1 } };

    BOOST_REQUIRE_EQUAL(tags.size(), 1u);
    BOOST_REQUIRE_EQUAL_COLLECTIONS(tags.begin(), tags.end(), expected.begin(), expected.end());
}

SCORUM_TEST_CASE(check_comment_with_tags_in_upper_case)
{
    create_post(alice, "c1")
        .set_json(R"({"domains": ["d1"], "categories": ["c1"], "tags": ["TAG1","TAG2"]})")
        .in_block();

    auto cat1_tags = _api.get_tags_by_category("d1", "c1");

    std::vector<std::pair<std::string, uint32_t>> cat1_expected = { { "tag2", 1 }, { "tag1", 1 } };

    BOOST_REQUIRE_EQUAL(cat1_tags.size(), cat1_expected.size());
    BOOST_REQUIRE_EQUAL_COLLECTIONS(cat1_tags.begin(), cat1_tags.end(), cat1_expected.begin(), cat1_expected.end());
}

SCORUM_TEST_CASE(check_couple_categories_several_tags)
{
    // d1/c1
    create_post(alice, "c1")
        .set_json(R"({"domains": ["d1"], "categories": ["c1"], "tags": ["tag1","tag2","tag3"]})")
        .in_block();

    // d1/c1
    create_post(bob, "c1")
        .set_json(R"({"domains": ["d1"], "categories": ["c1"], "tags": ["tag2","tag3","tag4"]})")
        .in_block();

    // d1/c2
    create_post(sam, "c2")
        .set_json(R"({"domains": ["d1"], "categories": ["c2"], "tags": ["tag1","tag2","tag3"]})")
        .in_block();

    // d1/c2
    create_post(dave, "c2")
        .set_json(R"({"domains": ["d1"], "categories": ["c2"], "tags": ["tag3","tag4","tag5"]})")
        .in_block();

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
    create_post(alice, "c1")
        .set_json(R"({"domains": ["d1"], "categories": ["c1"], "tags": ["tag1","tag2","tag3"]})")
        .in_block();

    // d2/c1
    create_post(bob, "c1")
        .set_json(R"({"domains": ["d2"], "categories": ["c1"], "tags": ["tag2","tag3","tag4"]})")
        .in_block();

    // d2/c1
    create_post(sam, "c1").set_json(R"({"domains": ["d2"], "categories": ["c1"], "tags": ["tag4","tag5"]})").in_block();

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
    create_post(alice, "c1")
        .set_json(R"({"domains": ["d1"], "categories": ["c1"], "tags": ["tag1","tag2","tag3"]})")
        .in_block();

    // d2/c1
    create_post(bob, "c2")
        .set_json(R"({"domains": ["d1"], "categories": ["c2"], "tags": ["tag2","tag3","tag4"]})")
        .in_block();

    // d2/c1
    create_post(sam, "c2").set_json(R"({"domains": ["d1"], "categories": ["c2"], "tags": ["tag4","tag5"]})").in_block();

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
    create_post(alice, "c1")
        .set_json(R"({"domains": ["d1"], "categories": ["c1"], "tags": ["tag1","tag2","tag3"]})")
        .in_block();

    auto tags = _api.get_tags_by_category("d1", "c1");

    std::vector<std::pair<std::string, uint32_t>> tags_expected = { { "tag3", 1 }, { "tag2", 1 }, { "tag1", 1 } };

    BOOST_REQUIRE_EQUAL(tags.size(), tags_expected.size());
    BOOST_REQUIRE_EQUAL_COLLECTIONS(tags.begin(), tags.end(), tags_expected.begin(), tags_expected.end());
}

SCORUM_TEST_CASE(check_json_metadata_doesnt_contain_domain)
{
    create_post(alice, "c1").set_json(R"({"category": "c1", "tags": ["tag1","tag2","tag3"]})").in_block();

    BOOST_REQUIRE_EQUAL((db.get_index<scorum::tags::category_stats_index, scorum::tags::by_category>().size()), 0u);
}

SCORUM_TEST_CASE(check_json_metadata_doesnt_contain_category)
{
    create_post(alice, "c1").set_json(R"({"domain": "d1", "tags": ["tag1","tag2","tag3"]})").in_block();

    BOOST_REQUIRE_EQUAL((db.get_index<category_stats_index, by_category>().size()), 0u);
}

SCORUM_TEST_CASE(check_post_removed)
{
    // d1/c1
    create_post(alice, "c1")
        .set_json(R"({"domains": ["d1"], "categories": ["c1"], "tags": ["tag1","tag2","tag3"]})")
        .in_block();

    // d1/c1
    auto post2 = create_post(bob, "c1")
                     .set_json(R"({"domains": ["d1"], "categories": ["c1"], "tags": ["tag2","tag3","tag4"]})")
                     .in_block();

    auto cat_tags_before = _api.get_tags_by_category("d1", "c1");

    std::vector<std::pair<std::string, uint32_t>> cat_tags_before_expected
        = { { "tag3", 2 }, { "tag2", 2 }, { "tag4", 1 }, { "tag1", 1 } };

    BOOST_REQUIRE_EQUAL(cat_tags_before.size(), cat_tags_before_expected.size());
    BOOST_REQUIRE_EQUAL_COLLECTIONS(cat_tags_before.begin(), cat_tags_before.end(), cat_tags_before_expected.begin(),
                                    cat_tags_before_expected.end());

    post2.remove();

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
    create_post(alice, "c1")
        .set_json(R"({"domains": ["d1"], "categories": ["c1"], "tags": ["tag1","tag2","tag3"]})")
        .in_block();

    // d1/c1
    auto post2 = create_post(bob, "c1")
                     .set_json(R"({"domains": ["d1"], "categories": ["c1"], "tags": ["tag2","tag3","tag4"]})")
                     .in_block_with_delay();

    auto cat_tags_before = _api.get_tags_by_category("d1", "c1");

    std::vector<std::pair<std::string, uint32_t>> cat_tags_before_expected
        = { { "tag3", 2 }, { "tag2", 2 }, { "tag4", 1 }, { "tag1", 1 } };

    BOOST_REQUIRE_EQUAL(cat_tags_before.size(), cat_tags_before_expected.size());
    BOOST_REQUIRE_EQUAL_COLLECTIONS(cat_tags_before.begin(), cat_tags_before.end(), cat_tags_before_expected.begin(),
                                    cat_tags_before_expected.end());

    // changing post2's tags
    create_post(bob, "c1")
        .set_permlink(post2.permlink())
        .set_json(R"({"domains": ["d1"], "categories": ["c1"], "tags": ["tag1","tag2","tag4"]})")
        .in_block();

    auto cat_tags_after = _api.get_tags_by_category("d1", "c1");

    std::vector<std::pair<std::string, uint32_t>> cat_tags_after_expected
        = { { "tag2", 2 }, { "tag1", 2 }, { "tag4", 1 }, { "tag3", 1 } };

    BOOST_REQUIRE_EQUAL(cat_tags_after.size(), cat_tags_after_expected.size());
    BOOST_REQUIRE_EQUAL_COLLECTIONS(cat_tags_after.begin(), cat_tags_after.end(), cat_tags_after_expected.begin(),
                                    cat_tags_after_expected.end());
}

BOOST_AUTO_TEST_SUITE_END()

#endif
