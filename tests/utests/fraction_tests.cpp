#include <boost/test/unit_test.hpp>

#include <scorum/utils/fraction.hpp>

#include <scorum/protocol/asset.hpp>

#include "defines.hpp"

#include <limits>

namespace fraction_tests {
using namespace scorum;
using namespace scorum::protocol;

BOOST_AUTO_TEST_SUITE(fraction_tests)

BOOST_AUTO_TEST_CASE(fraction_creation_check)
{
    SCORUM_REQUIRE_THROW(utils::make_fraction(ASSET_SP(10).amount, 0);, fc::assert_exception);

    {
        auto f = utils::make_fraction(1, 2);
        BOOST_CHECK_EQUAL(f.numerator, 1);
        BOOST_CHECK_EQUAL(f.denominator, 2);
    }
    {
        auto f = utils::make_fraction(ASSET_SP(10).amount, 2);
        BOOST_CHECK_EQUAL(f.numerator, 10);
        BOOST_CHECK_EQUAL(f.denominator, 2);
    }
    {
        auto f = utils::make_fraction(1, ASSET_SP(10).amount);
        BOOST_CHECK_EQUAL(f.numerator, 1);
        BOOST_CHECK_EQUAL(f.denominator, 10);
    }
    {
        auto f = utils::make_fraction(ASSET_SP(10).amount, ASSET_SCR(20).amount);
        BOOST_CHECK_EQUAL(f.numerator, 10);
        BOOST_CHECK_EQUAL(f.denominator, 20);
    }
}

BOOST_AUTO_TEST_CASE(multiply_by_fractional_negative_check)
{
    // negative value
    SCORUM_REQUIRE_THROW(utils::multiply_by_fractional(-10, 1, 2), fc::assert_exception);
    SCORUM_REQUIRE_THROW(utils::multiply_by_fractional(10, -1, 2), fc::assert_exception);
    SCORUM_REQUIRE_THROW(utils::multiply_by_fractional(10, 1, -2), fc::assert_exception);
    SCORUM_REQUIRE_THROW(utils::multiply_by_fractional(-10, -1, -2), fc::assert_exception);
    // zero denominator
    SCORUM_REQUIRE_THROW(utils::multiply_by_fractional(10, 1, 0), fc::assert_exception);
}

BOOST_AUTO_TEST_CASE(multiply_by_fractional_positive_check)
{
    // claculate half off int32 max
    BOOST_CHECK_EQUAL(utils::multiply_by_fractional(std::numeric_limits<int32_t>::max(), 50, 100),
                      std::numeric_limits<int32_t>::max() / 2);

    // claculate half off int64 max
    BOOST_CHECK_EQUAL(utils::multiply_by_fractional(std::numeric_limits<int64_t>::max(), 50, 100),
                      std::numeric_limits<int64_t>::max() / 2);

    // claculate half off asset_symbol_type max
    BOOST_CHECK_EQUAL(utils::multiply_by_fractional(std::numeric_limits<asset_symbol_type>::max(), 50, 100),
                      std::numeric_limits<asset_symbol_type>::max() / 2);
}

BOOST_AUTO_TEST_CASE(asset_with_fractional_operations_check)
{
    // calculate 20 % of 100 SP
    {
        auto value = ASSET_SP(100e+9);
        value *= utils::make_fraction(20, 100);
        BOOST_CHECK_EQUAL(value, ASSET_SP(20e+9));
    }

    // calculate 20 % of 100 SP
    {
        auto value = ASSET_SP(100e+9) * utils::make_fraction(20, 100);
        BOOST_CHECK_EQUAL(value, ASSET_SP(20e+9));
    }

    // calculate 20 % of maximum SP
    {
        auto value = asset::maximum(SP_SYMBOL) * utils::make_fraction(20, 100);
        BOOST_CHECK_EQUAL(value, asset::maximum(SP_SYMBOL) / 5);
    }
}

BOOST_AUTO_TEST_CASE(fraction_simplify_check)
{
    BOOST_CHECK(utils::make_fraction(20'000, 100'000).simplify() == utils::make_fraction(1, 5));

    BOOST_CHECK(utils::make_fraction(8, 12).simplify() == utils::make_fraction(2, 3));

    BOOST_CHECK(utils::make_fraction(2, 3).simplify() == utils::make_fraction(2, 3));
}

BOOST_AUTO_TEST_CASE(fraction_invert_check)
{
    BOOST_CHECK(utils::make_fraction(2, 3).invert() == utils::make_fraction(1, 3));

    BOOST_CHECK(utils::make_fraction(12, 20).invert() == utils::make_fraction(8, 20));
}

BOOST_AUTO_TEST_SUITE_END()
}
