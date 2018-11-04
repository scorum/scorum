#include <boost/test/unit_test.hpp>
#include <scorum/utils/static_variant_comparison.hpp>
#include <fc/static_variant.hpp>
#include <defines.hpp>

namespace {
using namespace scorum;
using namespace std::string_literals;

using variant_type = fc::static_variant<int, float, std::string>;

BOOST_AUTO_TEST_SUITE(static_variant_comparison_tests)

SCORUM_TEST_CASE(variant_less_test_positive)
{
    // underlying types ordering
    BOOST_CHECK(utils::variant_less(variant_type{ 0 }, variant_type{ 0.f }));
    BOOST_CHECK(utils::variant_less(variant_type{ 0 }, variant_type{ "0"s }));
    BOOST_CHECK(utils::variant_less(variant_type{ 0.f }, variant_type{ "0"s }));

    // values ordering
    BOOST_CHECK(utils::variant_less(variant_type{ 0 }, variant_type{ 1 }));
    BOOST_CHECK(utils::variant_less(variant_type{ 0.f }, variant_type{ 1.f }));
    BOOST_CHECK(utils::variant_less(variant_type{ "0"s }, variant_type{ "1"s }));
}

SCORUM_TEST_CASE(variant_less_test_negative)
{
    // opposite underlying types ordering
    BOOST_CHECK(!utils::variant_less(variant_type{ 0.f }, variant_type{ 0 }));
    BOOST_CHECK(!utils::variant_less(variant_type{ "0"s }, variant_type{ 0 }));
    BOOST_CHECK(!utils::variant_less(variant_type{ "0"s }, variant_type{ 0.f }));

    // opposite values ordering
    BOOST_CHECK(!utils::variant_less(variant_type{ 1 }, variant_type{ 0 }));
    BOOST_CHECK(!utils::variant_less(variant_type{ 1.f }, variant_type{ 0.f }));
    BOOST_CHECK(!utils::variant_less(variant_type{ "1"s }, variant_type{ "0"s }));

    // equal values
    BOOST_CHECK(!utils::variant_less(variant_type{ 1 }, variant_type{ 1 }));
    BOOST_CHECK(!utils::variant_less(variant_type{ 1.f }, variant_type{ 1.f }));
    BOOST_CHECK(!utils::variant_less(variant_type{ "1"s }, variant_type{ "1"s }));
}

SCORUM_TEST_CASE(variant_eq_test_positive)
{
    BOOST_CHECK(utils::variant_eq(variant_type{ 0 }, variant_type{ 0 }));
    BOOST_CHECK(utils::variant_eq(variant_type{ 0.f }, variant_type{ 0.f }));
    BOOST_CHECK(utils::variant_eq(variant_type{ "0"s }, variant_type{ "0"s }));
}

SCORUM_TEST_CASE(variant_eq_test_negative)
{
    // different types
    BOOST_CHECK(!utils::variant_eq(variant_type{ 0.f }, variant_type{ 0 }));
    BOOST_CHECK(!utils::variant_eq(variant_type{ "0"s }, variant_type{ 0 }));
    BOOST_CHECK(!utils::variant_eq(variant_type{ "0"s }, variant_type{ 0.f }));

    // different values
    BOOST_CHECK(!utils::variant_eq(variant_type{ 1 }, variant_type{ 0 }));
    BOOST_CHECK(!utils::variant_eq(variant_type{ 0 }, variant_type{ 1 }));
    BOOST_CHECK(!utils::variant_eq(variant_type{ 1.f }, variant_type{ 0.f }));
    BOOST_CHECK(!utils::variant_eq(variant_type{ 0.f }, variant_type{ 1.f }));
    BOOST_CHECK(!utils::variant_eq(variant_type{ "1"s }, variant_type{ "0"s }));
    BOOST_CHECK(!utils::variant_eq(variant_type{ "0"s }, variant_type{ "1"s }));
}

BOOST_AUTO_TEST_SUITE_END()
}
