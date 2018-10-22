#include <boost/test/unit_test.hpp>
#include <scorum/utils/take_n_range.hpp>
#include <boost/range/adaptor/transformed.hpp>
#include <boost/range/adaptor/filtered.hpp>

#include "defines.hpp"

namespace {
// clang-format off
struct foo
{
    std::string member;

    foo() = default;
    foo(std::string s) : member(std::move(s)) {}
    foo(const foo& x) : member(x.member) { was_copied = true; }
    foo(foo&& x) noexcept : member(std::move(x.member)) { was_moved = true; }
    foo& operator=(const foo& x) { member = x.member; was_copied = true; return *this; }
    foo& operator=(foo&& x) noexcept { member = std::move(x.member); was_moved = true; return *this; }

    bool was_copied = false;
    bool was_moved = false;
};

std::vector<foo> get_foo_vector()
{
    std::vector<foo> vec;
    vec.reserve(1);
    vec.emplace_back();

    return vec;
}
// clang-format on

using namespace scorum;

BOOST_AUTO_TEST_SUITE(take_n_range_tests)

BOOST_AUTO_TEST_CASE(multiple_iteration_test)
{
    std::vector<foo> vec = { { "1" }, { "2" }, { "3" } };

    auto rng = vec | utils::adaptors::take_n(2);

    {
        auto it = rng.begin();
        BOOST_CHECK_EQUAL(it->member, "1");
        ++it;
        BOOST_CHECK_EQUAL(it->member, "2");
    }
    {
        auto it = rng.begin();
        BOOST_CHECK_EQUAL(it->member, "1");
        ++it;
        BOOST_CHECK_EQUAL(it->member, "2");
    }
}

BOOST_AUTO_TEST_CASE(empty_underlying_range_take_test)
{
    std::vector<foo> vec;

    auto rng = vec | utils::adaptors::take_n(1);

    BOOST_CHECK_EQUAL(std::distance(rng.begin(), rng.end()), 0u);
}

BOOST_AUTO_TEST_CASE(take_more_than_underlying_range_has_test)
{
    std::vector<foo> vec = { { "1" }, { "2" } };

    auto rng = vec | utils::adaptors::take_n(5);

    BOOST_CHECK_EQUAL(std::distance(rng.begin(), rng.end()), 2u);
    auto it = rng.begin();
    BOOST_CHECK_EQUAL(it->member, "1");
    ++it;
    BOOST_CHECK_EQUAL(it->member, "2");
}

BOOST_AUTO_TEST_CASE(take_less_than_underlying_range_has_test)
{
    std::vector<foo> vec = { { "1" }, { "2" }, { "3" } };

    auto rng = vec | utils::adaptors::take_n(2);

    BOOST_CHECK_EQUAL(std::distance(rng.begin(), rng.end()), 2u);
    auto it = rng.begin();
    BOOST_CHECK_EQUAL(it->member, "1");
    ++it;
    BOOST_CHECK_EQUAL(it->member, "2");
}

BOOST_AUTO_TEST_CASE(take_zero_test)
{
    std::vector<foo> vec = { { "1" }, { "2" } };

    auto rng = vec | utils::adaptors::take_n(0);

    BOOST_CHECK_EQUAL(std::distance(rng.begin(), rng.end()), 0u);
}

BOOST_AUTO_TEST_CASE(non_const_underlying_range_able_to_modify_test)
{
    std::vector<foo> vec = { { "1" }, { "2" } };

    auto rng = boost::make_iterator_range(vec) | utils::adaptors::take_n(2);
    static_assert(std::is_same<decltype(*rng.begin()), foo&>::value, "Non-const rng should be forwarded as non-const");

    for (auto& x : rng)
        x.member = "0"; // able to modify value
    for (const auto& x : rng)
        BOOST_CHECK_EQUAL(x.member, "0");
}

BOOST_AUTO_TEST_CASE(const_underlying_range_unable_to_modify_test)
{
    std::vector<foo> vec = { { "0" }, { "0" } };
    const std::vector<foo>& vec_ref = vec;

    auto rng = boost::make_iterator_range(vec_ref) | utils::adaptors::take_n(2);

    static_assert(std::is_same<decltype(*rng.begin()), const foo&>::value, "Const rng should be forwarded as const");

    for (const auto& x : rng)
        BOOST_CHECK_EQUAL(x.member, "0");
}

BOOST_AUTO_TEST_CASE(lvalue_underlying_range_copy_test)
{
    std::vector<foo> vec;
    vec.reserve(1);
    vec.emplace_back();

    auto rng = vec | utils::adaptors::take_n(1);
    auto& fst_foo = *rng.begin();

    BOOST_CHECK(!fst_foo.was_moved);
    BOOST_CHECK(fst_foo.was_copied);
}

BOOST_AUTO_TEST_CASE(rvalue_underlying_range_move_test)
{
    auto rng = get_foo_vector() | utils::adaptors::take_n(1);
    auto& fst_foo = *rng.begin();

    BOOST_CHECK(!fst_foo.was_copied);
}

BOOST_AUTO_TEST_CASE(boost_adaptors_interaction_test)
{
    std::vector<foo> vec = { { "0" }, { "1" }, { "2" }, { "3" }, { "4" }, { "5" }, { "6" } };

    // clang-format off
    auto rng = boost::make_iterator_range(vec)
        | boost::adaptors::transformed([](const auto& x) { return std::stol(x.member); })
        | utils::adaptors::take_n(5)
        | boost::adaptors::filtered([](const auto& x) { return x % 2 == 0; });
    // clang-format on

    std::vector<long> result(rng.begin(), rng.end());
    std::vector<long> expected = { 0, 2, 4 };

    BOOST_CHECK_EQUAL_COLLECTIONS(result.begin(), result.end(), expected.begin(), expected.end());
}

BOOST_AUTO_TEST_SUITE_END()
}
