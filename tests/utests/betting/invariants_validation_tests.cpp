#include <boost/test/unit_test.hpp>

#include <scorum/protocol/betting/invariants_validation.hpp>
#include <scorum/protocol/betting/market.hpp>

#include <defines.hpp>

namespace {
using namespace scorum;
using namespace scorum::protocol;
using namespace scorum::protocol::betting;

BOOST_AUTO_TEST_SUITE(invariants_validation_tests)

SCORUM_TEST_CASE(validate_wincases_test_positive)
{
    {
        auto wincase = total::over{ 500 };
        BOOST_REQUIRE_NO_THROW(validate_wincase(wincase));
    }
    {
        auto wincase = total::under{ 1000 };
        BOOST_REQUIRE_NO_THROW(validate_wincase(wincase));
    }
    {
        auto wincase = handicap::over{ -500 };
        BOOST_REQUIRE_NO_THROW(validate_wincase(wincase));
    }
    {
        auto wincase = handicap::over{ 0 };
        BOOST_REQUIRE_NO_THROW(validate_wincase(wincase));
    }
    {
        auto wincase = handicap::under{ 1000 };
        BOOST_REQUIRE_NO_THROW(validate_wincase(wincase));
    }
    {
        auto wincase = result_home::yes{};
        BOOST_REQUIRE_NO_THROW(validate_wincase(wincase));
    }
}

SCORUM_TEST_CASE(validate_wincases_test_negative)
{
    {
        auto wincase = total::over{ 400 }; // must be divisible by 500
        BOOST_REQUIRE_THROW(validate_wincase(wincase), fc::assert_exception);
    }
    {
        auto wincase = total::under{ -1000 }; // must be positive
        BOOST_REQUIRE_THROW(validate_wincase(wincase), fc::assert_exception);
    }
    {
        auto wincase = total::under{ 0 }; // must be positive
        BOOST_REQUIRE_THROW(validate_wincase(wincase), fc::assert_exception);
    }
    {
        auto wincase = handicap::over{ -400 }; // must be divisible by 500
        BOOST_REQUIRE_THROW(validate_wincase(wincase), fc::assert_exception);
    }
    {
        auto wincase = handicap::over{ 499 }; // must be divisible by 500
        BOOST_REQUIRE_THROW(validate_wincase(wincase), fc::assert_exception);
    }
}

SCORUM_TEST_CASE(validate_game_full_market_list_test_positive)
{
    soccer_game game;
    // clang-format off
    fc::flat_set<market_type> markets = { result_home{},
                                          result_draw{},
                                          result_away{},
                                          round_home{},
                                          handicap{ 500 },
                                          correct_score_home{},
                                          correct_score_draw{},
                                          correct_score_away{},
                                          correct_score{1, 2},
                                          goal_home{},
                                          goal_both{},
                                          goal_away{},
                                          total{ 500 },
                                          total_goals_home{},
                                          total_goals_away{}};
    // clang-format on

    BOOST_REQUIRE_NO_THROW(validate_game(game, markets));
}

SCORUM_TEST_CASE(validate_game_invalid_market_should_negative)
{
    hockey_game game; // there are some alien markets (handicap_market, total_goals_market)
    // clang-format off
    fc::flat_set<market_type> markets = { result_home{},
                                          goal_home{},
                                          handicap{ 500 },
                                          total_goals_home{} };
    // clang-format on

    BOOST_REQUIRE_THROW(validate_game(game, markets), fc::assert_exception);
}

SCORUM_TEST_CASE(validate_if_wincase_in_game_positive)
{
    soccer_game game;

    BOOST_REQUIRE_NO_THROW(validate_if_wincase_in_game(game, correct_score::yes{ 2, 1 }));
}

SCORUM_TEST_CASE(validate_if_wincase_in_game_negative)
{
    hockey_game game;

    BOOST_REQUIRE_NO_THROW(validate_if_wincase_in_game(game, goal_home::yes{}));

    BOOST_REQUIRE_THROW(validate_if_wincase_in_game(game, correct_score::yes{ 2, 1 }), fc::assert_exception);
}

BOOST_AUTO_TEST_SUITE_END()
}
