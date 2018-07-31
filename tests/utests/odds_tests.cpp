#include <boost/test/unit_test.hpp>

#include <scorum/protocol/odds.hpp>

#include "defines.hpp"

#include <limits>

namespace odds_tests {
using namespace scorum;
using namespace scorum::protocol;

BOOST_AUTO_TEST_SUITE(odds_tests)

BOOST_AUTO_TEST_CASE(odds_positive_creation_check)
{
    auto base_fraction = utils::make_fraction(400, 300);
    odds k = base_fraction;

    BOOST_CHECK_EQUAL(k.base(), base_fraction);
    BOOST_CHECK_EQUAL(k.simplified(), utils::make_fraction(4, 3));
    BOOST_CHECK_EQUAL(k.inverted(), utils::make_fraction(4, 1));

    BOOST_CHECK_NO_THROW(odds(utils::make_fraction(std::numeric_limits<int16_t>::max(), 2)));
}

BOOST_AUTO_TEST_CASE(odds_negative_creation_check)
{
    BOOST_REQUIRE_EQUAL(sizeof(odds_value_type), sizeof(int16_t));

    BOOST_CHECK_THROW(odds(utils::make_fraction(std::numeric_limits<int64_t>::max(), 2)), fc::overflow_exception);
    BOOST_CHECK_THROW(odds(utils::make_fraction(std::numeric_limits<int32_t>::max(), 2)), fc::overflow_exception);

    BOOST_CHECK_THROW(odds(utils::make_fraction(-2, 1)), fc::assert_exception);
    BOOST_CHECK_THROW(odds(utils::make_fraction(2, -1)), fc::assert_exception);
    BOOST_CHECK_THROW(odds(utils::make_fraction(-2, -1)), fc::assert_exception);
    BOOST_CHECK_THROW(odds(utils::make_fraction(2, 0)), fc::assert_exception);
    BOOST_CHECK_THROW(odds(utils::make_fraction(0, 1)), fc::assert_exception);
    BOOST_CHECK_THROW(odds(utils::make_fraction(0, 0)), fc::assert_exception);

    BOOST_CHECK_NO_THROW(odds(utils::make_fraction(2, 1)));
}

BOOST_AUTO_TEST_CASE(odds_str_check)
{
    BOOST_CHECK_THROW(odds::from_string(""), fc::exception);
    BOOST_CHECK_THROW(odds::from_string("230"), fc::exception);
    BOOST_CHECK_THROW(odds::from_string("aaaaaaaaa/30"), fc::exception);

    const std::string str = "30/2";

    odds k = odds::from_string(str);

    BOOST_CHECK_EQUAL(k.simplified(), utils::make_fraction(15, 1));

    BOOST_CHECK_EQUAL(k.to_string(), str);
}

BOOST_AUTO_TEST_CASE(odds_variant_check)
{
    odds k = utils::make_fraction(40, 30);

    fc::variant vk;

    BOOST_REQUIRE_NO_THROW(fc::to_variant(k, vk));

    odds k2;

    BOOST_REQUIRE_NO_THROW(fc::from_variant(vk, k2));

    BOOST_CHECK_EQUAL(k, k2);
}

BOOST_AUTO_TEST_CASE(odds_cast_to_fraction_check)
{
    odds k = utils::make_fraction(40, 30);

    BOOST_CHECK_EQUAL((odds_fraction_type)k, utils::make_fraction(4, 3));
}

BOOST_AUTO_TEST_CASE(odds_empty_check)
{
    odds k;

    BOOST_CHECK(!k);

    BOOST_CHECK_THROW(k.base(), fc::assert_exception);
    BOOST_CHECK_THROW(k.simplified(), fc::assert_exception);
    BOOST_CHECK_THROW(k.inverted(), fc::assert_exception);

    BOOST_CHECK_NE(k, odds(2, 1));
    BOOST_CHECK_EQUAL(k, odds());
}

BOOST_AUTO_TEST_SUITE_END()
}
