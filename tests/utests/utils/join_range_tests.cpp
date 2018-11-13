#include <boost/test/unit_test.hpp>
#include <scorum/utils/range/join_range.hpp>
#include <scorum/utils/collect_range_adaptor.hpp>
#include <boost/range/adaptor/transformed.hpp>
#include <boost/range/adaptor/filtered.hpp>

#include "defines.hpp"

namespace {
using namespace scorum;

BOOST_AUTO_TEST_SUITE(join_range_tests)

BOOST_AUTO_TEST_CASE(first_range_empty_test)
{
    using namespace utils::adaptors;

    std::vector<std::string> fst;
    std::vector<std::string> snd = { "1", "2", "3" };

    auto actual = fst | joined(snd) | collect<std::vector>();
    auto expected = std::vector<std::string>{ "1", "2", "3" };

    BOOST_CHECK_EQUAL_COLLECTIONS(actual.begin(), actual.end(), expected.begin(), expected.end());
}

BOOST_AUTO_TEST_CASE(snd_range_empty_test)
{
    using namespace utils::adaptors;

    std::vector<std::string> fst = { "1", "2", "3" };
    std::vector<std::string> snd;

    auto actual = fst | joined(snd) | collect<std::vector>();
    auto expected = std::vector<std::string>{ "1", "2", "3" };

    BOOST_CHECK_EQUAL_COLLECTIONS(actual.begin(), actual.end(), expected.begin(), expected.end());
}

BOOST_AUTO_TEST_CASE(both_range_empty_test)
{
    using namespace utils::adaptors;

    std::vector<std::string> fst;
    std::vector<std::string> snd;

    auto actual = fst | joined(snd) | collect<std::vector>();
    auto expected = std::vector<std::string>();

    BOOST_CHECK_EQUAL_COLLECTIONS(actual.begin(), actual.end(), expected.begin(), expected.end());
}

BOOST_AUTO_TEST_CASE(both_contain_values_test)
{
    using namespace utils::adaptors;

    std::vector<std::string> fst = { "4", "5" };
    std::vector<std::string> snd = { "1", "2", "3" };

    auto actual = fst | joined(snd) | collect<std::vector>();
    auto expected = std::vector<std::string>{ "4", "5", "1", "2", "3" };

    BOOST_CHECK_EQUAL_COLLECTIONS(actual.begin(), actual.end(), expected.begin(), expected.end());
}

BOOST_AUTO_TEST_CASE(iterate_multiple_times_test)
{
    using namespace utils::adaptors;

    std::vector<std::string> fst = { "4", "5" };
    std::vector<std::string> snd = { "1", "2", "3" };

    auto actual = fst | joined(snd) | collect<std::vector>();
    auto expected = std::vector<std::string>{ "4", "5", "1", "2", "3" };

    BOOST_CHECK_EQUAL_COLLECTIONS(actual.begin(), actual.end(), expected.begin(), expected.end());
    BOOST_CHECK_EQUAL_COLLECTIONS(actual.begin(), actual.end(), expected.begin(), expected.end());
}

BOOST_AUTO_TEST_CASE(combine_with_boost_adaptors_test)
{
    using namespace utils::adaptors;
    using namespace boost::adaptors;

    std::vector<std::string> fst = { "4", "52", "c5" };
    std::vector<std::string> snd = { "12", "2", "3" };

    auto actual = fst //
        | filtered([](auto x) { return x.size() == 1; }) //
        | joined(snd) //
        | transformed([](auto x) { return x + x; }) //
        | collect<std::vector>();

    auto expected = std::vector<std::string>{ "44", "1212", "22", "33" };

    BOOST_CHECK_EQUAL_COLLECTIONS(actual.begin(), actual.end(), expected.begin(), expected.end());
    BOOST_CHECK_EQUAL_COLLECTIONS(actual.begin(), actual.end(), expected.begin(), expected.end());
}

BOOST_AUTO_TEST_CASE(move_only_underlying_type_test)
{
    using namespace utils::adaptors;

    // clang-format off
    struct foo
    {
        std::string s;
        foo(std::string s) : s(s) {}
        foo(const foo&) = delete;
        foo& operator=(const foo&) = delete;
        foo(foo&&) = default;
        foo& operator=(foo&&) = default;
    };
    // clang-format on

    std::vector<foo> fst;
    fst.emplace_back("1");
    fst.emplace_back("2");
    std::vector<foo> snd;
    snd.emplace_back("3");

    std::vector<std::string> actual;
    auto rng = fst | joined(snd);

    for (const auto& x : rng)
        actual.push_back(x.s);
    auto expected = std::vector<std::string>{ "1", "2", "3" };

    BOOST_CHECK_EQUAL_COLLECTIONS(actual.begin(), actual.end(), expected.begin(), expected.end());
}

BOOST_AUTO_TEST_CASE(const_underlying_type_test)
{
    using namespace utils::adaptors;

    struct foo
    {
        std::string s;
    };

    std::vector<foo> fst;
    fst.push_back({ "1" });
    fst.push_back({ "2" });
    std::vector<foo> snd;
    snd.push_back({ "3" });

    auto const_fst = boost::make_iterator_range(fst.cbegin(), fst.cend());
    auto const_snd = boost::make_iterator_range(snd.cbegin(), snd.cend());

    auto rng = const_fst | joined(const_snd);

    static_assert(std::is_same<decltype(rng)::iterator::reference, const foo&>::value, "should be const");

    std::vector<std::string> actual;
    for (const auto& x : rng)
        actual.push_back(x.s);
    auto expected = std::vector<std::string>{ "1", "2", "3" };

    BOOST_CHECK_EQUAL_COLLECTIONS(actual.begin(), actual.end(), expected.begin(), expected.end());
}

BOOST_AUTO_TEST_SUITE_END()
}
