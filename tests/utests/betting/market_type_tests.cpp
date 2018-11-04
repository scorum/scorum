#include <boost/test/unit_test.hpp>
#include <scorum/protocol/betting/market.hpp>
#include <defines.hpp>

namespace {
using namespace scorum;
using namespace scorum::protocol;

BOOST_AUTO_TEST_SUITE(market_type_tests)

SCORUM_TEST_CASE(create_wincases_test)
{
    {
        market_type m = result_home{};
        auto wincases = create_wincases(m);
        BOOST_CHECK_NO_THROW(wincases.first.get<result_home::yes>());
        BOOST_CHECK_NO_THROW(wincases.second.get<result_home::no>());
    }
    {
        auto wincases = create_wincases(result_draw{});
        BOOST_CHECK_NO_THROW(wincases.first.get<result_draw::yes>());
        BOOST_CHECK_NO_THROW(wincases.second.get<result_draw::no>());
    }
    {
        auto m = handicap{ -500 };
        auto wincases = create_wincases(m);
        BOOST_CHECK_NO_THROW(wincases.first.get<handicap::over>());
        BOOST_CHECK_NO_THROW(wincases.second.get<handicap::under>());
        BOOST_CHECK_EQUAL(wincases.first.get<handicap::over>().threshold, -500);
        BOOST_CHECK_EQUAL(wincases.second.get<handicap::under>().threshold, -500);
    }
    {
        auto m = correct_score{ 1, 2 };
        auto wincases = create_wincases(m);
        BOOST_CHECK_NO_THROW(wincases.first.get<correct_score::yes>());
        BOOST_CHECK_NO_THROW(wincases.second.get<correct_score::no>());
        BOOST_CHECK_EQUAL(wincases.first.get<correct_score::yes>().home, 1);
        BOOST_CHECK_EQUAL(wincases.first.get<correct_score::yes>().away, 2);
        BOOST_CHECK_EQUAL(wincases.second.get<correct_score::no>().home, 1);
        BOOST_CHECK_EQUAL(wincases.second.get<correct_score::no>().away, 2);
    }
}

SCORUM_TEST_CASE(has_trd_state_test_positive)
{
    BOOST_CHECK(has_trd_state(handicap{ 0 }));
    BOOST_CHECK(has_trd_state(handicap{ 1000 }));
    BOOST_CHECK(has_trd_state(handicap{ 2000 }));
    BOOST_CHECK(has_trd_state(handicap{ -2000 }));

    BOOST_CHECK(has_trd_state(total{ 1000 }));
    BOOST_CHECK(has_trd_state(total{ 2000 }));
}

SCORUM_TEST_CASE(has_trd_state_test_negative)
{
    BOOST_CHECK(!has_trd_state(handicap{ 500 }));
    BOOST_CHECK(!has_trd_state(handicap{ 1500 }));
    BOOST_CHECK(!has_trd_state(handicap{ 2500 }));
    BOOST_CHECK(!has_trd_state(handicap{ -1500 }));
    BOOST_CHECK(!has_trd_state(total{ 500 }));
    BOOST_CHECK(!has_trd_state(total{ 1500 }));
    BOOST_CHECK(!has_trd_state(result_home{}));
    BOOST_CHECK(!has_trd_state(result_draw{}));
    BOOST_CHECK(!has_trd_state(correct_score_away{}));
    BOOST_CHECK(!has_trd_state(correct_score{ 3, 2 }));
    BOOST_CHECK(!has_trd_state(goal_away{}));
}

BOOST_AUTO_TEST_SUITE_END()
}
