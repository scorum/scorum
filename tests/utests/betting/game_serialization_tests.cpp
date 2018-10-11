#include <boost/test/unit_test.hpp>

#include <scorum/protocol/operations.hpp>
#include <scorum/protocol/betting/game.hpp>
#include <scorum/protocol/betting/market.hpp>
#include <scorum/protocol/betting/wincase.hpp>
#include <scorum/protocol/betting/betting_serialization.hpp>

#include <boost/uuid/uuid_generators.hpp>

#include <defines.hpp>
#include <iostream>

namespace {
using namespace scorum;
using namespace scorum::protocol;

struct game_serialization_test_fixture
{
    uuid_type game_uuid = boost::uuids::string_generator()("e629f9aa-6b2c-46aa-8fa8-36770e7a7a5f");

    create_game_operation get_soccer_create_game_operation() const
    {
        create_game_operation op;
        op.uuid = game_uuid;
        op.moderator = "moderator_name";
        op.name = "game_name";
        op.game = soccer_game{};
        op.start_time = time_point_sec{ 1461605400 };
        op.auto_resolve_delay_sec = 33;
        op.markets = get_markets();

        return op;
    }

    update_game_markets_operation get_update_game_markets_operation() const
    {
        update_game_markets_operation op;
        op.uuid = game_uuid;
        op.moderator = "moderator_name";
        op.markets = get_markets();

        return op;
    }

    const std::string markets_json = R"([ [ "result_home", {} ],
                                          [ "result_draw", {} ],
                                          [ "result_away", {} ],
                                          [ "round_home", {} ],
                                          [ "handicap", { "threshold": -500 } ],
                                          [ "handicap", { "threshold": 0 } ],
                                          [ "handicap", { "threshold": 1000 } ],
                                          [ "correct_score_home", {} ],
                                          [ "correct_score_draw", {} ],
                                          [ "correct_score_away", {} ],
                                          [ "correct_score", { "home": 1, "away": 0 } ],
                                          [ "correct_score", { "home": 1, "away": 1 } ],
                                          [ "goal_home", {} ],
                                          [ "goal_both", {} ],
                                          [ "goal_away", {} ],
                                          [ "total", { "threshold": 0 } ],
                                          [ "total", { "threshold": 500 } ],
                                          [ "total", { "threshold": 1000 } ] ])";

    const std::string create_markets_json_tpl = R"({ "uuid":"e629f9aa-6b2c-46aa-8fa8-36770e7a7a5f",
                                                     "moderator": "moderator_name",
                                                     "name": "game_name",
                                                     "start_time": "2016-04-25T17:30:00",
                                                     "auto_resolve_delay_sec": 33,
                                                     "game": [ "soccer_game", {} ],
                                                     "markets": ${markets} })";

    const std::string update_markets_json_tpl = R"({ "moderator": "moderator_name",
                                                     "uuid":"e629f9aa-6b2c-46aa-8fa8-36770e7a7a5f",
                                                     "markets": ${markets} })";

    fc::flat_set<market_type> get_markets() const
    {
        // clang-format off
        return { result_home{},
                 result_draw{},
                 result_away{},
                 round_home{},
                 handicap{1000},
                 handicap{-500},
                 handicap{0},
                 correct_score_home{},
                 correct_score_draw{},
                 correct_score_away{},
                 correct_score{1, 1},
                 correct_score{1, 0},
                 goal_home{},
                 goal_both{},
                 goal_away{},
                 total{0},
                 total{500},
                 total{1000} };
        // clang-format on
    }

    void validate_soccer_create_game_operation(const create_game_operation& obj) const
    {
        BOOST_CHECK_EQUAL(obj.moderator, "moderator_name");
        BOOST_CHECK_EQUAL(obj.name, "game_name");
        BOOST_CHECK(obj.start_time == time_point_sec{ 1461605400 });
        BOOST_CHECK_NO_THROW(obj.game.get<soccer_game>());

        validate_markets(obj.markets);
    }

    void validate_update_game_markets_operation(const update_game_markets_operation& obj) const
    {
        BOOST_CHECK_EQUAL(obj.moderator, "moderator_name");

        validate_markets(obj.markets);
    }

    market_kind get_market_kind(const market_type& var) const
    {
        market_kind result;
        var.visit([&](const auto& market) { result = market.kind_v; });
        return result;
    }

    void validate_markets(const fc::flat_set<market_type>& markets) const
    {
        BOOST_REQUIRE_EQUAL(markets.size(), 18u);

        auto pos = 0u;

        BOOST_CHECK(get_market_kind(*markets.nth(pos)) == market_kind::result);

        pos += 3u;

        BOOST_CHECK(get_market_kind(*markets.nth(pos)) == market_kind::round);

        pos += 1u;

        BOOST_CHECK(get_market_kind(*markets.nth(pos)) == market_kind::handicap);
        auto market_wincases = create_wincases(*markets.nth(pos));
        BOOST_CHECK_EQUAL(market_wincases.first.get<handicap::over>().threshold, -500);
        BOOST_CHECK_EQUAL(market_wincases.second.get<handicap::under>().threshold, -500);

        pos += 3u;

        BOOST_CHECK(get_market_kind(*markets.nth(pos)) == market_kind::correct_score);
        market_wincases = create_wincases(*markets.nth(pos));

        pos += 3u;

        BOOST_CHECK(get_market_kind(*markets.nth(pos)) == market_kind::correct_score);
        market_wincases = create_wincases(*markets.nth(pos));
        BOOST_CHECK_EQUAL(market_wincases.first.get<correct_score::yes>().home, 1);
        BOOST_CHECK_EQUAL(market_wincases.first.get<correct_score::yes>().away, 0);
        BOOST_CHECK_EQUAL(market_wincases.second.get<correct_score::no>().home, 1);
        BOOST_CHECK_EQUAL(market_wincases.second.get<correct_score::no>().away, 0);

        pos += 2u;

        BOOST_CHECK(get_market_kind(*markets.nth(pos)) == market_kind::goal);

        pos += 3u;

        BOOST_CHECK(get_market_kind(*markets.nth(pos)) == market_kind::total);
    }
};

BOOST_FIXTURE_TEST_SUITE(game_serialization_tests, game_serialization_test_fixture)

template <typename T> std::string to_hex(T t)
{
    return fc::to_hex(fc::raw::pack(market_type(t)));
}

SCORUM_TEST_CASE(serialize_markets)
{
    BOOST_CHECK_EQUAL(to_hex(result_home()), "00");
    BOOST_CHECK_EQUAL(to_hex(result_draw()), "01");
    BOOST_CHECK_EQUAL(to_hex(result_away()), "02");
    BOOST_CHECK_EQUAL(to_hex(round_home()), "03");
    BOOST_CHECK_EQUAL(to_hex(handicap()), "040000");
    BOOST_CHECK_EQUAL(to_hex(correct_score_home()), "05");
    BOOST_CHECK_EQUAL(to_hex(correct_score_draw()), "06");
    BOOST_CHECK_EQUAL(to_hex(correct_score_away()), "07");
    BOOST_CHECK_EQUAL(to_hex(correct_score()), "0800000000");
    BOOST_CHECK_EQUAL(to_hex(goal_home()), "09");
    BOOST_CHECK_EQUAL(to_hex(goal_both()), "0a");
    BOOST_CHECK_EQUAL(to_hex(goal_away()), "0b");
    BOOST_CHECK_EQUAL(to_hex(total()), "0c0000");
    BOOST_CHECK_EQUAL(to_hex(total_goals_home()), "0d0000");
    BOOST_CHECK_EQUAL(to_hex(total_goals_away()), "0e0000");
}

SCORUM_TEST_CASE(serialize_soccer_with_empty_markets)
{
    create_game_operation op;
    op.uuid = game_uuid;
    op.moderator = "admin";
    op.name = "game name";
    op.start_time = time_point_sec::from_iso_string("2018-08-03T10:12:43");
    op.auto_resolve_delay_sec = 33;
    op.game = soccer_game{};
    op.markets = {};

    auto hex = fc::to_hex(fc::raw::pack(op));

    BOOST_CHECK_EQUAL(hex, "e629f9aa6b2c46aa8fa836770e7a7a5f0561646d696e0967616d65206e616d659b2a645b210000000000");
}

SCORUM_TEST_CASE(serialize_soccer_with_total_1000)
{
    create_game_operation op;
    op.uuid = game_uuid;
    op.moderator = "admin";
    op.name = "game name";
    op.start_time = time_point_sec::from_iso_string("2018-08-03T10:12:43");
    op.auto_resolve_delay_sec = 33;
    op.game = soccer_game{};
    op.markets = { total{ 1000 } };

    auto hex = fc::to_hex(fc::raw::pack(op));

    BOOST_CHECK_EQUAL(hex,
                      "e629f9aa6b2c46aa8fa836770e7a7a5f0561646d696e0967616d65206e616d659b2a645b2100000000010ce803");
}

SCORUM_TEST_CASE(create_game_json_serialization_test)
{
    auto op = get_soccer_create_game_operation();

    auto json = fc::json::to_string(op);

    auto json_comp = fc::format_string(create_markets_json_tpl, fc::mutable_variant_object()("markets", markets_json));
    json_comp = fc::json::to_string(fc::json::from_string(json_comp));

    BOOST_CHECK_EQUAL(json, json_comp);
}

SCORUM_TEST_CASE(create_game_json_deserialization_test)
{
    auto json = fc::format_string(create_markets_json_tpl, fc::mutable_variant_object()("markets", markets_json));
    auto obj = fc::json::from_string(json).as<create_game_operation>();

    validate_soccer_create_game_operation(obj);
}

SCORUM_TEST_CASE(markets_duplicates_serialization_test)
{
    create_game_operation op;
    op.uuid = game_uuid;
    op.game = soccer_game{};
    op.markets = { correct_score_home{}, correct_score_home{} };
    op.auto_resolve_delay_sec = 33;

    auto json = fc::json::to_string(op);
    // clang-format off
    auto json_comp = fc::json::to_string(fc::json::from_string(
                                           R"(
                                           {
                                              "uuid":"e629f9aa-6b2c-46aa-8fa8-36770e7a7a5f"
                                              "moderator":"",
                                              "name":"",
                                              "start_time":"1970-01-01T00:00:00",
                                              "auto_resolve_delay_sec": 33,
                                              "game":[
                                                 "soccer_game",
                                                 {}
                                              ],
                                              "markets":[
                                                 [
                                                    "correct_score_home",
                                                    {}
                                                 ]
                                              ]
                                           }
                                           )"));
    // clang-format on
    BOOST_CHECK_EQUAL(json, json_comp);
}

SCORUM_TEST_CASE(markets_duplicates_deserialization_test)
{
    // clang-format off
    auto json_with_duplicates = fc::json::to_string(fc::json::from_string(
                                           R"(
                                           {
                                              "moderator":"",
                                              "name":"",
                                              "start":"1970-01-01T00:00:00",
                                              "game":[
                                                 "soccer_game",
                                                 {}
                                              ],
                                              "markets":[
                                                 [
                                                    "correct_score_home",
                                                    {}
                                                 ],
                                                 [
                                                    "correct_score_home",
                                                    {}
                                                 ]
                                              ]
                                           }
                                           )"));
    // clang-format on

    auto obj = fc::json::from_string(json_with_duplicates).as<create_game_operation>();

    BOOST_REQUIRE_EQUAL(obj.markets.size(), 1u);
}

SCORUM_TEST_CASE(wincases_duplicates_serialization_test)
{
    create_game_operation op;
    op.uuid = game_uuid;
    op.game = soccer_game{};
    op.markets = { correct_score{ 1, 1 }, correct_score_home{} };
    op.auto_resolve_delay_sec = 33;

    auto json = fc::json::to_string(op);
    // clang-format off
    auto json_comp = fc::json::to_string(fc::json::from_string(
                                           R"(
                                           {
                                              "uuid":"e629f9aa-6b2c-46aa-8fa8-36770e7a7a5f"
                                              "moderator":"",
                                              "name":"",
                                              "start_time":"1970-01-01T00:00:00",
                                              "auto_resolve_delay_sec":33,
                                              "game":[
                                                 "soccer_game",
                                                 {}
                                              ],
                                              "markets":[
                                                 [
                                                    "correct_score_home",
                                                    {}
                                                 ],
                                                 [
                                                    "correct_score",
                                                    {
                                                       "home":1,
                                                       "away":1
                                                    }
                                                 ]
                                              ]
                                           }
                                           )"));
    // clang-format on
    BOOST_CHECK_EQUAL(json, json_comp);
}

SCORUM_TEST_CASE(wincases_duplicates_deserialization_test)
{
    // clang-format off
    auto json_with_duplicates = fc::json::to_string(fc::json::from_string(
                                           R"(
                                           {
                                              "moderator":"",
                                              "name":"",
                                              "start_time":"1970-01-01T00:00:00",
                                              "game":[
                                                 "soccer_game",
                                                 {}
                                              ],
                                              "markets":[
                                                  [
                                                  "correct_score",
                                                      {
                                                        "home":1,
                                                        "away":1
                                                      }
                                                   ],
                                                   [
                                                       "correct_score",
                                                       {
                                                         "home":1,
                                                         "away":1
                                                       }
                                                   ],
                                                   [
                                                       "correct_score_home",
                                                       {}
                                                   ],
                                                   [
                                                       "correct_score_home",
                                                       {}
                                                   ]
                                              ]
                                           }
                                           )"));
    // clang-format on

    auto obj = fc::json::from_string(json_with_duplicates).as<create_game_operation>();

    BOOST_REQUIRE_EQUAL(obj.markets.size(), 2u);
}

SCORUM_TEST_CASE(update_game_markets_json_serialization_test)
{
    auto op = get_update_game_markets_operation();

    auto json = fc::json::to_string(op);

    auto json_comp = fc::format_string(update_markets_json_tpl, fc::mutable_variant_object()("markets", markets_json));
    json_comp = fc::json::to_string(fc::json::from_string(json_comp));

    BOOST_CHECK_EQUAL(json, json_comp);
}

SCORUM_TEST_CASE(update_game_markets_json_deserialization_test)
{
    auto json = fc::format_string(update_markets_json_tpl, fc::mutable_variant_object()("markets", markets_json));
    auto obj = fc::json::from_string(json).as<update_game_markets_operation>();

    validate_update_game_markets_operation(obj);
}

SCORUM_TEST_CASE(create_game_binary_serialization_test)
{
    auto op = get_soccer_create_game_operation();

    auto hex = fc::to_hex(fc::raw::pack(op));

    BOOST_CHECK_EQUAL(hex, "e629f9aa6b2c46aa8fa836770e7a7a5f0e6d6f64657261746f725f6e616d650967616d655f6e616d6518541e572"
                           "1000000001200010203040cfe04000004e80305060708010000000801000100090a0b0c00000cf4010ce803");
}

SCORUM_TEST_CASE(create_game_binary_deserialization_test)
{
    auto hex = "e629f9aa6b2c46aa8fa836770e7a7a5f0e6d6f64657261746f725f6e616d650967616d655f6e616d6518541e572"
               "1000000001200010203040cfe04000004e80305060708010000000801000100090a0b0c00000cf4010ce803";

    char buffer[1000];
    fc::from_hex(hex, buffer, sizeof(buffer));
    auto obj = fc::raw::unpack<create_game_operation>(buffer, sizeof(buffer));

    validate_soccer_create_game_operation(obj);
}

SCORUM_TEST_CASE(update_game_markets_binary_serialization_test)
{
    auto op = get_update_game_markets_operation();

    auto hex = fc::to_hex(fc::raw::pack(op));

    BOOST_CHECK_EQUAL(hex, "0e6d6f64657261746f725f6e616d65e629f9aa6b2c46aa8fa836770e7a7a5f1200010203040cfe04000004e8030"
                           "5060708010000000801000100090a0b0c00000cf4010ce803");
}

SCORUM_TEST_CASE(update_game_markets_binary_deserialization_test)
{
    auto hex = "0e6d6f64657261746f725f6e616d65e629f9aa6b2c46aa8fa836770e7a7a5f1200010203040cfe04000004e8030"
               "5060708010000000801000100090a0b0c00000cf4010ce803";

    char buffer[1000];
    fc::from_hex(hex, buffer, sizeof(buffer));
    auto obj = fc::raw::unpack<update_game_markets_operation>(buffer, sizeof(buffer));

    validate_update_game_markets_operation(obj);
}

BOOST_AUTO_TEST_SUITE_END()
}
