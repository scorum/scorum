#include <boost/test/unit_test.hpp>

#include <scorum/protocol/betting/market.hpp>

#include <defines.hpp>

namespace {
using namespace scorum;
using namespace scorum::protocol;
using namespace scorum::protocol::betting;

BOOST_AUTO_TEST_SUITE(threshold_comparison_tests)

BOOST_AUTO_TEST_CASE(total_goals_tests)
{
    /*
     * During over/under markets resolving you should check if value ('goals' in total market for example) is
     * greater then 'over' market and less then 'under' market
     */
    {
        int16_t threshold = 0;
        total::over over_05{ 500 };
        total::under under_05{ 500 };
        BOOST_CHECK(!(threshold > over_05.threshold));
        BOOST_CHECK((threshold < under_05.threshold));

        total::over over_10{ 1000 };
        total::under under_10{ 1000 };
        BOOST_CHECK(!(threshold > over_10.threshold));
        BOOST_CHECK((threshold < under_10.threshold));

        total::over over_15{ 1500 };
        total::under under_15{ 1500 };
        BOOST_CHECK(!(threshold > over_15.threshold));
        BOOST_CHECK((threshold < under_15.threshold));
    }
    {
        int goals = 1000;
        total::over over_05{ 500 };
        total::under under_05{ 500 };
        BOOST_CHECK((goals > over_05.threshold));
        BOOST_CHECK(!(goals < under_05.threshold));

        total::over over_10{ 1000 };
        total::under under_10{ 1000 };
        BOOST_CHECK(!(goals > over_10.threshold));
        BOOST_CHECK(!(goals < under_10.threshold));

        total::over over_15{ 1500 };
        total::under under_15{ 1500 };
        BOOST_CHECK(!(goals > over_15.threshold));
        BOOST_CHECK((goals < under_15.threshold));

        total::over over_20{ 2000 };
        total::under under_20{ 2000 };
        BOOST_CHECK(!(goals > over_20.threshold));
        BOOST_CHECK((goals < under_20.threshold));
    }
    {
        int goals = 2000;
        total::over over_05{ 500 };
        total::under under_05{ 500 };
        BOOST_CHECK((goals > over_05.threshold));
        BOOST_CHECK(!(goals < under_05.threshold));

        total::over over_10{ 1000 };
        total::under under_10{ 1000 };
        BOOST_CHECK((goals > over_10.threshold));
        BOOST_CHECK(!(goals < under_10.threshold));

        total::over over_15{ 1500 };
        total::under under_15{ 1500 };
        BOOST_CHECK((goals > over_15.threshold));
        BOOST_CHECK(!(goals < under_15.threshold));

        total::over over_20{ 2000 };
        total::under under_20{ 2000 };
        BOOST_CHECK(!(goals > over_20.threshold));
        BOOST_CHECK(!(goals < under_20.threshold));
    }
}

BOOST_AUTO_TEST_CASE(handicap_home_tests)
{
    {
        int home_minus_away_delta = 1000;
        handicap::over over_15{ 1500 };
        handicap::under under_15{ 1500 };
        BOOST_CHECK(!(home_minus_away_delta > over_15.threshold));
        BOOST_CHECK((home_minus_away_delta < under_15.threshold));

        handicap::over over_10{ 1000 };
        handicap::under under_10{ 1000 };
        BOOST_CHECK(!(home_minus_away_delta > over_10.threshold));
        BOOST_CHECK(!(home_minus_away_delta < under_10.threshold));

        handicap::over over_05{ 500 };
        handicap::under under_05{ 500 };
        BOOST_CHECK((home_minus_away_delta > over_05.threshold));
        BOOST_CHECK(!(home_minus_away_delta < under_05.threshold));

        handicap::over over_00{ 0 };
        handicap::under under_00{ 0 };
        BOOST_CHECK((home_minus_away_delta > over_00.threshold));
        BOOST_CHECK(!(home_minus_away_delta < under_00.threshold));
    }
    {
        int home_minus_away_delta = 0;

        handicap::over over_10{ 1000 };
        handicap::under under_10{ 1000 };
        BOOST_CHECK(!(home_minus_away_delta > over_10.threshold));
        BOOST_CHECK((home_minus_away_delta < under_10.threshold));

        handicap::over over_05{ 500 };
        handicap::under under_05{ 500 };
        BOOST_CHECK(!(home_minus_away_delta > over_05.threshold));
        BOOST_CHECK((home_minus_away_delta < under_05.threshold));

        handicap::over over_00{ 0 };
        handicap::under under_00{ 0 };
        BOOST_CHECK(!(home_minus_away_delta > over_00.threshold));
        BOOST_CHECK(!(home_minus_away_delta < under_00.threshold));

        handicap::over over_m05{ -500 };
        handicap::under under_m05{ -500 };
        BOOST_CHECK((home_minus_away_delta > over_m05.threshold));
        BOOST_CHECK(!(home_minus_away_delta < under_m05.threshold));

        handicap::over over_m10{ -1000 };
        handicap::under under_m10{ -1000 };
        BOOST_CHECK((home_minus_away_delta > over_m10.threshold));
        BOOST_CHECK(!(home_minus_away_delta < under_m10.threshold));
    }
    {
        int home_minus_away_delta = -1000;

        handicap::over over_05{ 500 };
        handicap::under under_05{ 500 };
        BOOST_CHECK(!(home_minus_away_delta > over_05.threshold));
        BOOST_CHECK((home_minus_away_delta < under_05.threshold));

        handicap::over over_00{ 0 };
        handicap::under under_00{ 0 };
        BOOST_CHECK(!(home_minus_away_delta > over_00.threshold));
        BOOST_CHECK((home_minus_away_delta < under_00.threshold));

        handicap::over over_m05{ -500 };
        handicap::under under_m05{ -500 };
        BOOST_CHECK(!(home_minus_away_delta > over_m05.threshold));
        BOOST_CHECK((home_minus_away_delta < under_m05.threshold));

        handicap::over over_m10{ -1000 };
        handicap::under under_m10{ -1000 };
        BOOST_CHECK(!(home_minus_away_delta > over_m10.threshold));
        BOOST_CHECK(!(home_minus_away_delta < under_m10.threshold));

        handicap::over over_m15{ -1500 };
        handicap::under under_m15{ -1500 };
        BOOST_CHECK((home_minus_away_delta > over_m15.threshold));
        BOOST_CHECK(!(home_minus_away_delta < under_m15.threshold));

        handicap::over over_m20{ -2000 };
        handicap::under under_m20{ -2000 };
        BOOST_CHECK((home_minus_away_delta > over_m20.threshold));
        BOOST_CHECK(!(home_minus_away_delta < under_m20.threshold));
    }
}

BOOST_AUTO_TEST_SUITE_END()
}
