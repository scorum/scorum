#include <boost/test/unit_test.hpp>
#include <scorum/utils/string_algorithm.hpp>

#include "defines.hpp"

#include <scorum/protocol/asset.hpp>
#include <scorum/rewards_math/formulas.hpp>

using namespace scorum;

BOOST_AUTO_TEST_SUITE(string_substring_tests)

BOOST_AUTO_TEST_CASE(check_substring_russian_test)
{
    BOOST_CHECK_EQUAL(utils::substring("тест", 0, 2), "те");
    BOOST_CHECK_EQUAL(utils::substring("тест", 1, 2), "ес");
    BOOST_CHECK_EQUAL(utils::substring("тест", 1, 10), "ест");
    BOOST_CHECK_EQUAL(utils::substring("тест", 100, 10), "");
}

BOOST_AUTO_TEST_CASE(check_substring_english_test)
{
    BOOST_CHECK_EQUAL(utils::substring("test", 0, 2), "te");
    BOOST_CHECK_EQUAL(utils::substring("test", 1, 2), "es");
    BOOST_CHECK_EQUAL(utils::substring("test", 1, 10), "est");
    BOOST_CHECK_EQUAL(utils::substring("test", 100, 200), "");
}

BOOST_AUTO_TEST_CASE(check_mixing_russian_english_test)
{
    BOOST_CHECK_EQUAL(utils::substring("testтест", 0, 6), "testте");
    BOOST_CHECK_EQUAL(utils::substring("testтест", 1, 6), "estтес");
    BOOST_CHECK_EQUAL(utils::substring("testтест", 7, 1), "т");
    BOOST_CHECK_EQUAL(utils::substring("testтест", 8, 1), "");
}

BOOST_AUTO_TEST_CASE(check_substring_empty_string)
{
    BOOST_CHECK_EQUAL(utils::substring("", 0, 2), "");
    BOOST_CHECK_EQUAL(utils::substring("", 1, 2), "");
}

BOOST_AUTO_TEST_SUITE_END()
