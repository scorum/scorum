#include <boost/test/unit_test.hpp>
#include <scorum/utils/range/flatten_range.hpp>
#include <scorum/utils/collect_range_adaptor.hpp>
#include <boost/range/adaptor/transformed.hpp>
#include <boost/range/adaptor/filtered.hpp>

#include "defines.hpp"

namespace {
using namespace scorum;

BOOST_AUTO_TEST_SUITE(flatten_range_tests)

BOOST_AUTO_TEST_CASE(flatten_empty_range_test)
{
    using namespace utils::adaptors;

    std::vector<std::vector<std::string>> rng;

    auto actual = rng | flatten | collect<std::vector>();
    auto expected = std::vector<std::string>{};

    BOOST_CHECK_EQUAL_COLLECTIONS(actual.begin(), actual.end(), expected.begin(), expected.end());
}

BOOST_AUTO_TEST_CASE(flatten_range_with_empty_ranges_test)
{
    using namespace utils::adaptors;

    std::vector<std::vector<std::string>> rng = { {}, {}, {} };

    auto actual = rng | flatten | collect<std::vector>();
    auto expected = std::vector<std::string>{};

    BOOST_CHECK_EQUAL_COLLECTIONS(actual.begin(), actual.end(), expected.begin(), expected.end());
}

BOOST_AUTO_TEST_CASE(flatten_range_with_mixed_empty_and_nonempty_ranges_test)
{
    using namespace utils::adaptors;

    std::vector<std::vector<std::string>> rng = { {}, { "1", "2" }, {}, { "3" } };

    auto actual = rng | flatten | collect<std::vector>();
    auto expected = std::vector<std::string>{ "1", "2", "3" };

    BOOST_CHECK_EQUAL_COLLECTIONS(actual.begin(), actual.end(), expected.begin(), expected.end());
}

BOOST_AUTO_TEST_CASE(iterate_multiple_times_test)
{
    using namespace utils::adaptors;

    std::vector<std::vector<std::string>> rng = { {}, { "1", "2" }, { "3" }, {} };

    auto actual = rng | flatten | collect<std::vector>();
    auto expected = std::vector<std::string>{ "1", "2", "3" };

    BOOST_CHECK_EQUAL_COLLECTIONS(actual.begin(), actual.end(), expected.begin(), expected.end());
    BOOST_CHECK_EQUAL_COLLECTIONS(actual.begin(), actual.end(), expected.begin(), expected.end());
}

BOOST_AUTO_TEST_CASE(combine_with_boost_adaptors_test)
{
    using namespace utils::adaptors;
    using namespace boost::adaptors;

    std::vector<std::string> strs = { "01", "234", "5" };

    auto actual = strs //
        | transformed([](auto str) { return str; }) //
        | flatten //
        | transformed([](auto x) { return (int)x - '0'; }) //
        | collect<std::vector>();

    auto expected = std::vector<int>{ 0, 1, 2, 3, 4, 5 };

    BOOST_CHECK_EQUAL_COLLECTIONS(actual.begin(), actual.end(), expected.begin(), expected.end());
}

BOOST_AUTO_TEST_CASE(const_underlying_type_test)
{
    using namespace utils::adaptors;

    struct foo
    {
        std::string s;
    };

    std::vector<foo> vec1 = { { "1" } };
    std::vector<foo> vec2 = { { "2" } };
    auto const_fst = boost::make_iterator_range(vec1.cbegin(), vec1.cend());
    auto const_snd = boost::make_iterator_range(vec2.cbegin(), vec2.cend());

    auto rng_of_rngs = std::vector<decltype(const_fst)>{ const_fst, const_snd };

    auto rng = rng_of_rngs | flatten;

    static_assert(std::is_same<decltype(rng)::iterator::reference, const foo&>::value, "should be const");

    std::vector<std::string> actual;
    for (const auto& x : rng)
        actual.push_back(x.s);
    auto expected = std::vector<std::string>{ "1", "2" };

    BOOST_CHECK_EQUAL_COLLECTIONS(actual.begin(), actual.end(), expected.begin(), expected.end());
}

BOOST_AUTO_TEST_CASE(underlying_ranges_were_copied)
{
    using namespace utils::adaptors;

    // clang-format off
    struct foo
    {
        std::string s;
        bool was_copied = false;

        foo(std::string s) : s(s){ }
        foo(const foo&) { was_copied = true; }
        foo& operator=(const foo&) { was_copied = true; return *this; }
        foo(foo&&) = default;
    };
    // clang-format on

    std::vector<foo> rng;
    rng.emplace_back("1");
    std::vector<std::vector<foo>> rng_of_rngs;
    rng_of_rngs.push_back(std::move(rng));

    BOOST_REQUIRE(!rng_of_rngs[0][0].was_copied);

    auto flatten_rng = rng_of_rngs | flatten;

    for (const auto& x : flatten_rng)
        BOOST_CHECK(x.was_copied);
}

BOOST_AUTO_TEST_CASE(underlying_ranges_were_not_copied)
{
    using namespace utils::adaptors;
    using namespace boost::adaptors;

    // clang-format off
    struct foo
    {
        std::string s;
        bool was_copied = false;

        foo(std::string s) : s(s){ }
        foo(const foo&) { was_copied = true; }
        foo& operator=(const foo&) { was_copied = true; return *this; }
        foo(foo&&) = default;
    };
    // clang-format on

    std::vector<foo> rng;
    rng.emplace_back("1");
    std::vector<std::vector<foo>> rng_of_rngs;
    rng_of_rngs.push_back(std::move(rng));

    BOOST_REQUIRE(!rng_of_rngs[0][0].was_copied);

    auto flatten_rng = rng_of_rngs //
        | transformed([](auto& rng) { return boost::make_iterator_range(rng); }) //
        | flatten;

    for (const auto& x : flatten_rng)
        BOOST_CHECK(!x.was_copied);
}

BOOST_AUTO_TEST_SUITE_END()
}
