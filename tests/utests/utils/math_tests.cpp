#include <boost/test/unit_test.hpp>
#include <scorum/utils/math.hpp>

#include "defines.hpp"

using namespace scorum;

BOOST_AUTO_TEST_SUITE(ceil_tests)

BOOST_AUTO_TEST_CASE(starting_point_is_less_than_value)
{
    BOOST_CHECK_EQUAL(utils::ceil(10, 3, 3), 12); // 3|6|9|12
    BOOST_CHECK_EQUAL(utils::ceil(10, 4, 3), 10); // 4|7|10
    BOOST_CHECK_EQUAL(utils::ceil(10, 5, 4), 13); // 5|9|13
}

BOOST_AUTO_TEST_CASE(starting_point_is_greater_than_value)
{
    BOOST_CHECK_EQUAL(utils::ceil(10, 19, 4), 11); // 19|15|11
    BOOST_CHECK_EQUAL(utils::ceil(10, 11, 3), 11); // 11
    BOOST_CHECK_EQUAL(utils::ceil(10, 10, 4), 10); // 5|9|13
    BOOST_CHECK_EQUAL(utils::ceil(10, 17, 2), 11); // 17|15|13|11
}

BOOST_AUTO_TEST_SUITE_END()
