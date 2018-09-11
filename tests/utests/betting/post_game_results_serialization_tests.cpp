#include <boost/test/unit_test.hpp>

#include <scorum/protocol/operations.hpp>
#include <scorum/protocol/betting/game.hpp>
#include <scorum/protocol/betting/market.hpp>
#include <scorum/protocol/betting/betting_serialization.hpp>

#include <defines.hpp>
#include <iostream>

namespace {
using namespace scorum;
using namespace scorum::protocol;

struct post_game_results_serialization_test_fixture
{
    post_game_results_operation create_soccer_post_game_results_operation() const
    {
        post_game_results_operation op;
        op.game_id = 42;
        op.moderator = "homer";
        op.wincases = wincases;

        return op;
    }

    void validate_soccer_post_game_results_operation(const post_game_results_operation& op) const
    {
        BOOST_CHECK_EQUAL(op.moderator, "homer");
        BOOST_CHECK_EQUAL(op.game_id, 42);

        validate_wincases(op.wincases);
    }

    void validate_wincases(const fc::flat_set<wincase_type>& wincases) const
    {
        BOOST_REQUIRE_EQUAL(wincases.size(), 17u);

        BOOST_CHECK_NO_THROW(wincases.nth(0)->get<result_home::yes>());
        BOOST_CHECK_NO_THROW(wincases.nth(1)->get<result_draw::no>());
        BOOST_CHECK_NO_THROW(wincases.nth(2)->get<result_away::yes>());
        BOOST_CHECK_NO_THROW(wincases.nth(3)->get<round_home::no>());

        BOOST_CHECK_NO_THROW(wincases.nth(4)->get<handicap::over>());
        BOOST_CHECK_EQUAL(wincases.nth(4)->get<handicap::over>().threshold, 1000);
        BOOST_CHECK_NO_THROW(wincases.nth(5)->get<handicap::under>());
        BOOST_CHECK_EQUAL(wincases.nth(5)->get<handicap::under>().threshold, -500);
        BOOST_CHECK_NO_THROW(wincases.nth(6)->get<handicap::under>());
        BOOST_CHECK_EQUAL(wincases.nth(6)->get<handicap::under>().threshold, 0);

        BOOST_CHECK_NO_THROW(wincases.nth(7)->get<correct_score_home::yes>());
        BOOST_CHECK_NO_THROW(wincases.nth(8)->get<correct_score_draw::no>());
        BOOST_CHECK_NO_THROW(wincases.nth(9)->get<correct_score_away::no>());
        BOOST_CHECK_NO_THROW(wincases.nth(10)->get<correct_score::yes>());
        BOOST_CHECK_EQUAL(wincases.nth(10)->get<correct_score::yes>().home, 1);
        BOOST_CHECK_EQUAL(wincases.nth(10)->get<correct_score::yes>().away, 2);
        BOOST_CHECK_NO_THROW(wincases.nth(11)->get<correct_score::no>());
        BOOST_CHECK_EQUAL(wincases.nth(11)->get<correct_score::no>().home, 3);
        BOOST_CHECK_EQUAL(wincases.nth(11)->get<correct_score::no>().away, 2);

        BOOST_CHECK_NO_THROW(wincases.nth(12)->get<goal_home::yes>());
        BOOST_CHECK_NO_THROW(wincases.nth(13)->get<goal_both::no>());
        BOOST_CHECK_NO_THROW(wincases.nth(14)->get<goal_away::yes>());

        BOOST_CHECK_NO_THROW(wincases.nth(15)->get<total::over>());
        BOOST_CHECK_EQUAL(wincases.nth(15)->get<total::over>().threshold, 0);
        BOOST_CHECK_NO_THROW(wincases.nth(16)->get<total::under>());
        BOOST_CHECK_EQUAL(wincases.nth(16)->get<total::under>().threshold, 1000);
    }

    // clang-format off
    const fc::flat_set<wincase_type> wincases = { result_home::yes{},
                                                  result_draw::no{},
                                                  result_away::yes{},
                                                  round_home::no{},
                                                  handicap::over{1000},
                                                  handicap::under{-500},
                                                  handicap::under{0},
                                                  correct_score_home::yes{},
                                                  correct_score_draw::no{},
                                                  correct_score_away::no{},
                                                  correct_score::yes{ 1, 2 },
                                                  correct_score::no{ 3, 2 },
                                                  goal_home::yes{},
                                                  goal_both::no{},
                                                  goal_away::yes{},
                                                  total::over{ 0 },
                                                  total::under{ 1000 } };
    // clang-format on

    const std::string wincases_json = R"([ [ "result_home::yes", {} ],
                                           [ "result_draw::no", {} ],
                                           [ "result_away::yes", {} ],
                                           [ "round_home::no", {} ],
                                           [ "handicap::over", { "threshold": 1000 } ],
                                           [ "handicap::under", { "threshold": -500 } ],
                                           [ "handicap::under", { "threshold": 0 } ],
                                           [ "correct_score_home::yes", {} ],
                                           [ "correct_score_draw::no", {} ],
                                           [ "correct_score_away::no", {} ],
                                           [ "correct_score::yes", { "home": 1, "away": 2 } ],
                                           [ "correct_score::no", { "home": 3, "away": 2 } ],
                                           [ "goal_home::yes", {} ],
                                           [ "goal_both::no", {} ],
                                           [ "goal_away::yes", {} ],
                                           [ "total::over", { "threshold": 0 } ],
                                           [ "total::under", { "threshold": 1000 } ] ])";

    const std::string post_results_json_tpl = R"({
                                                   "moderator": "homer",
                                                   "game_id": 42,
                                                   "wincases": ${wincases}
                                                 })";
};

BOOST_FIXTURE_TEST_SUITE(post_game_results_serialization_tests, post_game_results_serialization_test_fixture)

SCORUM_TEST_CASE(post_game_results_json_serialization_test)
{
    auto op = create_soccer_post_game_results_operation();

    auto json = fc::json::to_string(op);

    auto json_ethalon
        = fc::format_string(post_results_json_tpl, fc::mutable_variant_object()("wincases", wincases_json));
    json_ethalon = fc::json::to_string(fc::json::from_string(json_ethalon));

    BOOST_CHECK_EQUAL(json, json_ethalon);
}

SCORUM_TEST_CASE(post_game_results_json_deserialization_test)
{
    auto json = fc::format_string(post_results_json_tpl, fc::mutable_variant_object()("wincases", wincases_json));
    auto obj = fc::json::from_string(json).as<post_game_results_operation>();

    validate_soccer_post_game_results_operation(obj);
}

SCORUM_TEST_CASE(post_game_results_binary_serialization_test)
{
    auto op = create_soccer_post_game_results_operation();

    auto hex = fc::to_hex(fc::raw::pack(op));

    BOOST_CHECK_EQUAL(
        hex, "05686f6d65722a00000000000000110003040708e803090cfe0900000a0d0f1001000200110300020012151618000019e803");
}

SCORUM_TEST_CASE(post_game_results_binary_deserialization_test)
{
    auto hex = "05686f6d65722a00000000000000110003040708e803090cfe0900000a0d0f1001000200110300020012151618000019e803";

    char buffer[1000];
    fc::from_hex(hex, buffer, sizeof(buffer));
    auto obj = fc::raw::unpack<post_game_results_operation>(buffer, sizeof(buffer));

    validate_soccer_post_game_results_operation(obj);
}

BOOST_AUTO_TEST_SUITE_END()
}
