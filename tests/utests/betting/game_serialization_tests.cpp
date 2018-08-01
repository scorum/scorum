#include <boost/test/unit_test.hpp>

#include <scorum/protocol/operations.hpp>
#include <scorum/protocol/betting/game.hpp>
#include <scorum/protocol/betting/market.hpp>
#include <scorum/protocol/betting/wincase.hpp>
#include <scorum/protocol/betting/wincase_serialization.hpp>
#include <scorum/protocol/betting/game_serialization.hpp>

#include <defines.hpp>
#include <iostream>

namespace {
using namespace scorum;
using namespace scorum::protocol;
using namespace scorum::protocol::betting;

struct game_serialization_test_fixture
{
    create_game_operation get_soccer_game_operation() const
    {
        create_game_operation op;
        op.moderator = "moderator_name";
        op.name = "game_name";
        op.game = soccer_game{};
        op.start = time_point_sec{ 1461605400 };
        op.markets = { { market_kind::result,
                         { { result_home{}, result_draw_away{} },
                           { result_draw{}, result_home_away{} },
                           { result_away{}, result_home_draw{} } } },
                       { market_kind::round, { { round_home{}, round_away{} } } },
                       { market_kind::handicap,
                         { { handicap_home_over{ 1000 }, handicap_home_under{ 1000 } },
                           { handicap_home_over{ -500 }, handicap_home_under{ -500 } },
                           { handicap_home_over{ 0 }, handicap_home_under{ 0 } } } },
                       { market_kind::correct_score,
                         { { correct_score_yes{ 1, 1 }, correct_score_no{ 1, 1 } },
                           { correct_score_no{ 1, 0 }, correct_score_yes{ 1, 0 } },
                           { correct_score_home_yes{}, correct_score_home_no{} },
                           { correct_score_draw_yes{}, correct_score_draw_no{} },
                           { correct_score_away_no{}, correct_score_away_yes{} } } },
                       { market_kind::goal,
                         { { goal_home_yes{}, goal_home_no{} },
                           { goal_both_yes{}, goal_both_no{} },
                           { goal_away_yes{}, goal_away_no{} } } },
                       { market_kind::total,
                         { { total_over{ 0 }, total_under{ 0 } },
                           { total_over{ 500 }, total_under{ 500 } },
                           { total_over{ 1000 }, total_under{ 1000 } } } } };

        return op;
    }

    void validate_soccer_game_operation(const create_game_operation& obj) const
    {
        BOOST_CHECK_EQUAL(obj.moderator, "moderator_name");
        BOOST_CHECK_EQUAL(obj.name, "game_name");
        BOOST_CHECK(obj.start == time_point_sec{ 1461605400 });
        BOOST_CHECK_NO_THROW(obj.game.get<soccer_game>());

        BOOST_CHECK(obj.markets[0].kind == market_kind::result);
        BOOST_CHECK_EQUAL(obj.markets[0].wincases.size(), 3u);
        BOOST_CHECK_NO_THROW(obj.markets[0].wincases[0].first.get<result_home>());
        BOOST_CHECK_NO_THROW(obj.markets[0].wincases[0].second.get<result_draw_away>());
        BOOST_CHECK_NO_THROW(obj.markets[0].wincases[2].first.get<result_away>());
        BOOST_CHECK_NO_THROW(obj.markets[0].wincases[2].second.get<result_home_draw>());

        BOOST_CHECK(obj.markets[1].kind == market_kind::round);
        BOOST_CHECK_EQUAL(obj.markets[1].wincases.size(), 1u);
        BOOST_CHECK_NO_THROW(obj.markets[1].wincases[0].first.get<round_home>());
        BOOST_CHECK_NO_THROW(obj.markets[1].wincases[0].second.get<round_away>());

        BOOST_CHECK(obj.markets[2].kind == market_kind::handicap);
        BOOST_CHECK_EQUAL(obj.markets[2].wincases.size(), 3u);
        BOOST_CHECK_EQUAL(obj.markets[2].wincases[0].first.get<handicap_home_over>().threshold.value, 1000);
        BOOST_CHECK_EQUAL(obj.markets[2].wincases[0].second.get<handicap_home_under>().threshold.value, 1000);
        BOOST_CHECK_EQUAL(obj.markets[2].wincases[1].first.get<handicap_home_over>().threshold.value, -500);
        BOOST_CHECK_EQUAL(obj.markets[2].wincases[1].second.get<handicap_home_under>().threshold.value, -500);

        BOOST_CHECK(obj.markets[3].kind == market_kind::correct_score);
        BOOST_CHECK_EQUAL(obj.markets[3].wincases.size(), 5u);
        BOOST_CHECK_EQUAL(obj.markets[3].wincases[0].first.get<correct_score_yes>().home, 1);
        BOOST_CHECK_EQUAL(obj.markets[3].wincases[0].first.get<correct_score_yes>().away, 1);
        BOOST_CHECK_EQUAL(obj.markets[3].wincases[0].second.get<correct_score_no>().home, 1);
        BOOST_CHECK_EQUAL(obj.markets[3].wincases[0].second.get<correct_score_no>().away, 1);
        BOOST_CHECK_NO_THROW(obj.markets[3].wincases[2].first.get<correct_score_home_yes>());
        BOOST_CHECK_NO_THROW(obj.markets[3].wincases[2].second.get<correct_score_home_no>());
        BOOST_CHECK_NO_THROW(obj.markets[3].wincases[4].first.get<correct_score_away_no>());
        BOOST_CHECK_NO_THROW(obj.markets[3].wincases[4].second.get<correct_score_away_yes>());

        BOOST_CHECK(obj.markets[4].kind == market_kind::goal);
        BOOST_CHECK_EQUAL(obj.markets[4].wincases.size(), 3u);
        BOOST_CHECK_NO_THROW(obj.markets[4].wincases[1].first.get<goal_both_yes>());
        BOOST_CHECK_NO_THROW(obj.markets[4].wincases[1].second.get<goal_both_no>());

        BOOST_CHECK(obj.markets[5].kind == market_kind::total);
        BOOST_CHECK_EQUAL(obj.markets[5].wincases.size(), 3u);
        BOOST_CHECK_EQUAL(obj.markets[5].wincases[0].first.get<total_over>().threshold.value, 0);
        BOOST_CHECK_EQUAL(obj.markets[5].wincases[0].second.get<total_under>().threshold.value, 0);
    }
};

BOOST_FIXTURE_TEST_SUITE(game_serialization_tests, game_serialization_test_fixture)

SCORUM_TEST_CASE(create_game_json_serialization_test)
{
    auto op = get_soccer_game_operation();

    auto json = fc::json::to_string(op);

    BOOST_CHECK_EQUAL(
        json,
        R"({"moderator":"moderator_name","name":"game_name","start":"2016-04-25T17:30:00","game":["soccer_game",{}],"markets":[{"kind":"result","wincases":[[["result_home",{}],["result_draw_away",{}]],[["result_draw",{}],["result_home_away",{}]],[["result_away",{}],["result_home_draw",{}]]]},{"kind":"round","wincases":[[["round_home",{}],["round_away",{}]]]},{"kind":"handicap","wincases":[[["handicap_home_over",{"threshold":{"value":1000}}],["handicap_home_under",{"threshold":{"value":1000}}]],[["handicap_home_over",{"threshold":{"value":-500}}],["handicap_home_under",{"threshold":{"value":-500}}]],[["handicap_home_over",{"threshold":{"value":0}}],["handicap_home_under",{"threshold":{"value":0}}]]]},{"kind":"correct_score","wincases":[[["correct_score_yes",{"home":1,"away":1}],["correct_score_no",{"home":1,"away":1}]],[["correct_score_no",{"home":1,"away":0}],["correct_score_yes",{"home":1,"away":0}]],[["correct_score_home_yes",{}],["correct_score_home_no",{}]],[["correct_score_draw_yes",{}],["correct_score_draw_no",{}]],[["correct_score_away_no",{}],["correct_score_away_yes",{}]]]},{"kind":"goal","wincases":[[["goal_home_yes",{}],["goal_home_no",{}]],[["goal_both_yes",{}],["goal_both_no",{}]],[["goal_away_yes",{}],["goal_away_no",{}]]]},{"kind":"total","wincases":[[["total_over",{"threshold":{"value":0}}],["total_under",{"threshold":{"value":0}}]],[["total_over",{"threshold":{"value":500}}],["total_under",{"threshold":{"value":500}}]],[["total_over",{"threshold":{"value":1000}}],["total_under",{"threshold":{"value":1000}}]]]}]})");
}

SCORUM_TEST_CASE(create_game_binary_serialization_test)
{
    auto op = get_soccer_game_operation();

    auto hex = fc::to_hex(fc::raw::pack(op));

    BOOST_CHECK_EQUAL(hex, "0e6d6f64657261746f725f6e616d650967616d655f6e616d6518541e57000600000000000000000300030104020"
                           "5010000000000000001060702000000000000000308e80309e803080cfe090cfe08000009000003000000000000"
                           "00050a010001000b010001000b010000000a010000000c0d0e0f111004000000000000000312131415161705000"
                           "000000000000318000019000018f40119f40118e80319e803");
}

SCORUM_TEST_CASE(create_game_json_deserialization_test)
{
    auto json
        = R"({"moderator":"moderator_name","name":"game_name","start":"2016-04-25T17:30:00","game":["soccer_game",{}],"markets":[{"kind":"result","wincases":[[["result_home",{}],["result_draw_away",{}]],[["result_draw",{}],["result_home_away",{}]],[["result_away",{}],["result_home_draw",{}]]]},{"kind":"round","wincases":[[["round_home",{}],["round_away",{}]]]},{"kind":"handicap","wincases":[[["handicap_home_over",{"threshold":{"value":1000}}],["handicap_home_under",{"threshold":{"value":1000}}]],[["handicap_home_over",{"threshold":{"value":-500}}],["handicap_home_under",{"threshold":{"value":-500}}]],[["handicap_home_over",{"threshold":{"value":0}}],["handicap_home_under",{"threshold":{"value":0}}]]]},{"kind":"correct_score","wincases":[[["correct_score_yes",{"home":1,"away":1}],["correct_score_no",{"home":1,"away":1}]],[["correct_score_no",{"home":1,"away":0}],["correct_score_yes",{"home":1,"away":0}]],[["correct_score_home_yes",{}],["correct_score_home_no",{}]],[["correct_score_draw_yes",{}],["correct_score_draw_no",{}]],[["correct_score_away_no",{}],["correct_score_away_yes",{}]]]},{"kind":"goal","wincases":[[["goal_home_yes",{}],["goal_home_no",{}]],[["goal_both_yes",{}],["goal_both_no",{}]],[["goal_away_yes",{}],["goal_away_no",{}]]]},{"kind":"total","wincases":[[["total_over",{"threshold":{"value":0}}],["total_under",{"threshold":{"value":0}}]],[["total_over",{"threshold":{"value":500}}],["total_under",{"threshold":{"value":500}}]],[["total_over",{"threshold":{"value":1000}}],["total_under",{"threshold":{"value":1000}}]]]}]})";

    auto obj = fc::json::from_string(json).as<create_game_operation>();

    validate_soccer_game_operation(obj);
}

SCORUM_TEST_CASE(create_game_binary_deserialization_test)
{
    auto hex = "0e6d6f64657261746f725f6e616d650967616d655f6e616d6518541e57000600000000000000000300030104020"
               "5010000000000000001060702000000000000000308e80309e803080cfe090cfe08000009000003000000000000"
               "00050a010001000b010001000b010000000a010000000c0d0e0f111004000000000000000312131415161705000"
               "000000000000318000019000018f40119f40118e80319e803";

    char buffer[1000];
    fc::from_hex(hex, buffer, sizeof(buffer));
    auto obj = fc::raw::unpack<create_game_operation>(buffer, sizeof(buffer));

    validate_soccer_game_operation(obj);
}

BOOST_AUTO_TEST_SUITE_END()
}