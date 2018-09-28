#include <boost/test/unit_test.hpp>

#include "tags_common.hpp"

namespace get_contents_tests {

using namespace scorum;
using namespace scorum::tags;

struct tags_get_contents_fixture : public database_fixture::tags_fixture
{
    tags_get_contents_fixture() = default;

    struct content_query_wrapper : public api::content_query
    {
        content_query_wrapper(std::string author_, std::string permlink_)
        {
            author = author_;
            permlink = permlink_;
        }
    };

    std::string default_metadata
        = R"({"domains": ["chain_tests"], "categories": ["tags_get_contents_tests"], "tags": ["test"]})";
};

BOOST_FIXTURE_TEST_SUITE(tags_get_contents_tests, tags_get_contents_fixture)

SCORUM_TEST_CASE(get_contents_negative_check)
{
    std::vector<api::content_query> qs;
    qs.reserve(2 * MAX_DISCUSSIONS_LIST_SIZE);
    for (auto permlink_num = 0u; permlink_num < 2 * MAX_DISCUSSIONS_LIST_SIZE; ++permlink_num)
    {
        api::content_query q;
        q.author = "alice";
        q.permlink = boost::lexical_cast<std::string>(permlink_num);
        qs.emplace_back(q);
    }
    BOOST_CHECK_THROW(_api.get_contents(qs), fc::assert_exception);
}

SCORUM_TEST_CASE(get_contents_positive_check)
{
    auto p_1 = create_post(alice).set_json(default_metadata).in_block();

    auto c_level1 = p_1.create_comment(sam).in_block();
    auto c_level2 = c_level1.create_comment(dave).in_block();

    auto p_2 = create_post(bob).set_json(default_metadata).in_block();

    std::vector<api::content_query> qs;

    qs.emplace_back(content_query_wrapper{ p_1.author(), p_1.permlink() });
    qs.emplace_back(content_query_wrapper{ p_2.author(), p_2.permlink() });
    qs.emplace_back(content_query_wrapper{ c_level1.author(), c_level1.permlink() });
    qs.emplace_back(content_query_wrapper{ c_level2.author(), c_level2.permlink() });

    auto result = _api.get_contents(qs);

    BOOST_REQUIRE_EQUAL(result.size(), 4u);

    BOOST_CHECK(
        std::find_if(std::begin(result), std::end(result),
                     [&](const api::discussion& d) { return d.author == p_1.author() && d.permlink == p_1.permlink(); })
        != result.end());

    BOOST_CHECK(
        std::find_if(std::begin(result), std::end(result),
                     [&](const api::discussion& d) { return d.author == p_2.author() && d.permlink == p_2.permlink(); })
        != result.end());

    BOOST_CHECK(std::find_if(std::begin(result), std::end(result),
                             [&](const api::discussion& d) {
                                 return d.author == c_level1.author() && d.permlink == c_level1.permlink();
                             })
                != result.end());

    BOOST_CHECK(std::find_if(std::begin(result), std::end(result),
                             [&](const api::discussion& d) {
                                 return d.author == c_level2.author() && d.permlink == c_level2.permlink();
                             })
                != result.end());
}

SCORUM_TEST_CASE(get_contents_single_record_check)
{
    auto p_1 = create_post(alice).set_json(default_metadata).in_block();

    auto result = _api.get_contents({ content_query_wrapper{ p_1.author(), p_1.permlink() } });

    BOOST_REQUIRE_EQUAL(result.size(), 1u);

    BOOST_CHECK(
        std::find_if(std::begin(result), std::end(result),
                     [&](const api::discussion& d) { return d.author == p_1.author() && d.permlink == p_1.permlink(); })
        != result.end());
}
BOOST_AUTO_TEST_SUITE_END()
}
