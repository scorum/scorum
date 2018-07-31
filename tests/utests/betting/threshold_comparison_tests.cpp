#include <boost/test/unit_test.hpp>

#include <scorum/protocol/betting/wincase.hpp>

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
        int goals = 0;
        total_over over_05{ { 500 } };
        total_under under_05{ { 500 } };
        BOOST_CHECK(!(goals > over_05.threshold));
        BOOST_CHECK((goals < under_05.threshold));

        total_over over_10{ { 1000 } };
        total_under under_10{ { 1000 } };
        BOOST_CHECK(!(goals > over_10.threshold));
        BOOST_CHECK((goals < under_10.threshold));

        total_over over_15{ { 1500 } };
        total_under under_15{ { 1500 } };
        BOOST_CHECK(!(goals > over_15.threshold));
        BOOST_CHECK((goals < under_15.threshold));
    }
    {
        int goals = 1;
        total_over over_05{ { 500 } };
        total_under under_05{ { 500 } };
        BOOST_CHECK((goals > over_05.threshold));
        BOOST_CHECK(!(goals < under_05.threshold));

        total_over over_10{ { 1000 } };
        total_under under_10{ { 1000 } };
        BOOST_CHECK(!(goals > over_10.threshold));
        BOOST_CHECK(!(goals < under_10.threshold));

        total_over over_15{ { 1500 } };
        total_under under_15{ { 1500 } };
        BOOST_CHECK(!(goals > over_15.threshold));
        BOOST_CHECK((goals < under_15.threshold));

        total_over over_20{ { 2000 } };
        total_under under_20{ { 2000 } };
        BOOST_CHECK(!(goals > over_20.threshold));
        BOOST_CHECK((goals < under_20.threshold));
    }
    {
        int goals = 2;
        total_over over_05{ { 500 } };
        total_under under_05{ { 500 } };
        BOOST_CHECK((goals > over_05.threshold));
        BOOST_CHECK(!(goals < under_05.threshold));

        total_over over_10{ { 1000 } };
        total_under under_10{ { 1000 } };
        BOOST_CHECK((goals > over_10.threshold));
        BOOST_CHECK(!(goals < under_10.threshold));

        total_over over_15{ { 1500 } };
        total_under under_15{ { 1500 } };
        BOOST_CHECK((goals > over_15.threshold));
        BOOST_CHECK(!(goals < under_15.threshold));

        total_over over_20{ { 2000 } };
        total_under under_20{ { 2000 } };
        BOOST_CHECK(!(goals > over_20.threshold));
        BOOST_CHECK(!(goals < under_20.threshold));
    }
}

BOOST_AUTO_TEST_CASE(handicap_tests)
{
    {
        int home_minus_away_delta = 1;
        handicap_home_over over_15{ { 1500 } };
        handicap_home_under under_15{ { 1500 } };
        BOOST_CHECK(!(home_minus_away_delta > over_15.threshold));
        BOOST_CHECK((home_minus_away_delta < under_15.threshold));

        handicap_home_over over_10{ { 1000 } };
        handicap_home_under under_10{ { 1000 } };
        BOOST_CHECK(!(home_minus_away_delta > over_10.threshold));
        BOOST_CHECK(!(home_minus_away_delta < under_10.threshold));

        handicap_home_over over_05{ { 500 } };
        handicap_home_under under_05{ { 500 } };
        BOOST_CHECK((home_minus_away_delta > over_05.threshold));
        BOOST_CHECK(!(home_minus_away_delta < under_05.threshold));

        handicap_home_over over_00{ { 0 } };
        handicap_home_under under_00{ { 0 } };
        BOOST_CHECK((home_minus_away_delta > over_00.threshold));
        BOOST_CHECK(!(home_minus_away_delta < under_00.threshold));
    }
    {
        int home_minus_away_delta = 0;

        handicap_home_over over_10{ { 1000 } };
        handicap_home_under under_10{ { 1000 } };
        BOOST_CHECK(!(home_minus_away_delta > over_10.threshold));
        BOOST_CHECK((home_minus_away_delta < under_10.threshold));

        handicap_home_over over_05{ { 500 } };
        handicap_home_under under_05{ { 500 } };
        BOOST_CHECK(!(home_minus_away_delta > over_05.threshold));
        BOOST_CHECK((home_minus_away_delta < under_05.threshold));

        handicap_home_over over_00{ { 0 } };
        handicap_home_under under_00{ { 0 } };
        BOOST_CHECK(!(home_minus_away_delta > over_00.threshold));
        BOOST_CHECK(!(home_minus_away_delta < under_00.threshold));

        handicap_home_over over_m05{ { -500 } };
        handicap_home_under under_m05{ { -500 } };
        BOOST_CHECK((home_minus_away_delta > over_m05.threshold));
        BOOST_CHECK(!(home_minus_away_delta < under_m05.threshold));

        handicap_home_over over_m10{ { -1000 } };
        handicap_home_under under_m10{ { -1000 } };
        BOOST_CHECK((home_minus_away_delta > over_m10.threshold));
        BOOST_CHECK(!(home_minus_away_delta < under_m10.threshold));
    }
    {
        int home_minus_away_delta = -1;

        handicap_home_over over_05{ { 500 } };
        handicap_home_under under_05{ { 500 } };
        BOOST_CHECK(!(home_minus_away_delta > over_05.threshold));
        BOOST_CHECK((home_minus_away_delta < under_05.threshold));

        handicap_home_over over_00{ { 0 } };
        handicap_home_under under_00{ { 0 } };
        BOOST_CHECK(!(home_minus_away_delta > over_00.threshold));
        BOOST_CHECK((home_minus_away_delta < under_00.threshold));

        handicap_home_over over_m05{ { -500 } };
        handicap_home_under under_m05{ { -500 } };
        BOOST_CHECK(!(home_minus_away_delta > over_m05.threshold));
        BOOST_CHECK((home_minus_away_delta < under_m05.threshold));

        handicap_home_over over_m10{ { -1000 } };
        handicap_home_under under_m10{ { -1000 } };
        BOOST_CHECK(!(home_minus_away_delta > over_m10.threshold));
        BOOST_CHECK(!(home_minus_away_delta < under_m10.threshold));

        handicap_home_over over_m15{ { -1500 } };
        handicap_home_under under_m15{ { -1500 } };
        BOOST_CHECK((home_minus_away_delta > over_m15.threshold));
        BOOST_CHECK(!(home_minus_away_delta < under_m15.threshold));

        handicap_home_over over_m20{ { -2000 } };
        handicap_home_under under_m20{ { -2000 } };
        BOOST_CHECK((home_minus_away_delta > over_m20.threshold));
        BOOST_CHECK(!(home_minus_away_delta < under_m20.threshold));
    }
}

BOOST_AUTO_TEST_SUITE_END()
}
