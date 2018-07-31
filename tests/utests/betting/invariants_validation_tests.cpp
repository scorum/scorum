#include <boost/test/unit_test.hpp>

#include <scorum/protocol/betting/invariants_validation.hpp>

#include <defines.hpp>

namespace {
using namespace scorum;
using namespace scorum::protocol;
using namespace scorum::protocol::betting;

BOOST_AUTO_TEST_SUITE(invariants_validation_tests)

SCORUM_TEST_CASE(validate_market_test_positive)
{
    market_type market;
    market.kind = market_kind::total;
    market.wincases = { total_over{}, total_under{} };

    BOOST_REQUIRE_NO_THROW(validate_market(market));
}

SCORUM_TEST_CASE(validate_market_test_negative)
{
    market_type market;

    market.kind = market_kind::total;
    market.wincases = { total_over{}, goal_away_no{} };
    BOOST_REQUIRE_THROW(validate_market(market), fc::assert_exception);

    market.kind = market_kind::goal;
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
    std::vector<market_type> markets = { { market_kind::result, { result_home{} } },
                                         { market_kind::round, { round_home{} } },
                                         { market_kind::handicap, { handicap_home_over{} } },
                                         { market_kind::correct_score, { correct_score_home_no{} } },
                                         { market_kind::goal, { goal_both_no{} } },
                                         { market_kind::total, { total_over{} } } };

    BOOST_REQUIRE_NO_THROW(validate_game(game, markets));
}

SCORUM_TEST_CASE(validate_game_invalid_market_should_throw)
{
    soccer_game game;
    std::vector<market_type> markets = { { market_kind::total_goals, { total_goals_home_over{} } },
                                         { market_kind::handicap, { handicap_home_over{} } } };

    BOOST_REQUIRE_THROW(validate_game(game, markets), fc::assert_exception);
}

BOOST_AUTO_TEST_SUITE_END()
}