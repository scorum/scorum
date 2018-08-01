#include <boost/test/unit_test.hpp>

#include <scorum/protocol/betting/invariants_validation.hpp>

#include <defines.hpp>

namespace {
using namespace scorum;
using namespace scorum::protocol;
using namespace scorum::protocol::betting;

BOOST_AUTO_TEST_SUITE(invariants_validation_tests)

SCORUM_TEST_CASE(validate_wincases_test_positive)
{
    {
        auto wincase = total_over{ 500 };
        BOOST_REQUIRE_NO_THROW(validate_wincase(wincase, market_kind::total));
    }
    {
        auto wincase = total_under{ 1000 };
        BOOST_REQUIRE_NO_THROW(validate_wincase(wincase, market_kind::total));
    }
    {
        auto wincase = handicap_home_over{ -500 };
        BOOST_REQUIRE_NO_THROW(validate_wincase(wincase, market_kind::handicap));
    }
    {
        auto wincase = handicap_home_over{ 0 };
        BOOST_REQUIRE_NO_THROW(validate_wincase(wincase, market_kind::handicap));
    }
    {
        auto wincase = handicap_home_under{ 1000 };
        BOOST_REQUIRE_NO_THROW(validate_wincase(wincase, market_kind::handicap));
    }
    {
        auto wincase = result_home{};
        BOOST_REQUIRE_NO_THROW(validate_wincase(wincase, market_kind::result));
    }
}

SCORUM_TEST_CASE(validate_wincases_test_negativee)
{
    {
        auto wincase = total_over{ 400 }; // must be divisible by 500
        BOOST_REQUIRE_THROW(validate_wincase(wincase, market_kind::total), fc::assert_exception);
    }
    {
        auto wincase = total_under{ -1000 }; // must be positive
        BOOST_REQUIRE_THROW(validate_wincase(wincase, market_kind::total), fc::assert_exception);
    }
    {
        auto wincase = total_under{ 0 }; // must be positive
        BOOST_REQUIRE_THROW(validate_wincase(wincase, market_kind::total), fc::assert_exception);
    }
    {
        auto wincase = handicap_home_over{ -400 }; // must be divisible by 500
        BOOST_REQUIRE_THROW(validate_wincase(wincase, market_kind::handicap), fc::assert_exception);
    }
    {
        auto wincase = handicap_home_over{ 499 }; // must be divisible by 500
        BOOST_REQUIRE_THROW(validate_wincase(wincase, market_kind::handicap), fc::assert_exception);
    }
    {
        auto wincase = handicap_home_over{ 500 }; // markets do not match
        BOOST_REQUIRE_THROW(validate_wincase(wincase, market_kind::total), fc::assert_exception);
    }
}

SCORUM_TEST_CASE(validate_market_test_positive)
{
    market_type market;
    market.kind = market_kind::total;
    market.wincases = { { total_over{ 500 }, total_under{ 500 } } };

    BOOST_REQUIRE_NO_THROW(validate_market(market));
}

SCORUM_TEST_CASE(validate_market_test_negative)
{
    market_type market;

    market.kind = market_kind::total;
    market.wincases = { { total_over{ 500 }, goal_away_no{} } }; // different wincases
    BOOST_REQUIRE_THROW(validate_market(market), fc::assert_exception);

    market.kind = market_kind::total;
    market.wincases = { { goal_away_yes{}, goal_away_no{} } }; // wincases from another market
    BOOST_REQUIRE_THROW(validate_market(market), fc::assert_exception);

    market.kind = market_kind::total;
    market.wincases = { { total_over{ 500 }, total_under{ 1000 } } }; // wincases which do not form a valid pair
    BOOST_REQUIRE_THROW(validate_market(market), fc::assert_exception);
}

SCORUM_TEST_CASE(validate_market_wincase_list_empty_should_throw)
{
    market_type market;
    market.kind = market_kind::total;
    market.wincases = {};

    BOOST_REQUIRE_THROW(validate_market(market), fc::assert_exception);
}

SCORUM_TEST_CASE(validate_game_empty_market_list_should_throw)
{
    std::vector<market_type> markets;

    BOOST_REQUIRE_THROW(validate_markets(markets), fc::assert_exception);
}

SCORUM_TEST_CASE(validate_game_full_market_list_test_positive)
{
    soccer_game game;
    std::vector<market_type> markets
        = { { market_kind::result, { { result_home{}, result_draw_away{} } } },
            { market_kind::round, { { round_home{}, round_away{} } } },
            { market_kind::handicap, { { handicap_home_over{ 500 }, handicap_home_over{ 500 } } } },
            { market_kind::correct_score, { { correct_score_home_yes{}, correct_score_home_no{} } } },
            { market_kind::goal, { { goal_both_yes{}, goal_both_no{} } } },
            { market_kind::total, { { total_over{ 500 }, total_under{ 500 } } } } };

    BOOST_REQUIRE_NO_THROW(validate_game(game, markets));
}

SCORUM_TEST_CASE(validate_game_invalid_market_should_throw)
{
    soccer_game game;
    std::vector<market_type> markets
        = { { market_kind::total_goals, { { total_goals_home_over{ 500 }, total_goals_home_under{ 500 } } } },
            { market_kind::handicap, { { handicap_home_over{ 500 }, handicap_home_under{ 500 } } } } };

    BOOST_REQUIRE_THROW(validate_game(game, markets), fc::assert_exception);
}

BOOST_AUTO_TEST_SUITE_END()
}