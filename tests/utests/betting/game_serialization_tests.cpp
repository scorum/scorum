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
    create_game_operation get_soccer_create_game_operation() const
    {
        create_game_operation op;
        op.moderator = "moderator_name";
        op.name = "game_name";
        op.game = soccer_game{};
        op.start = time_point_sec{ 1461605400 };
        op.markets = get_markets();

        return op;
    }

    update_game_markets_operation get_update_game_markets_operation() const
    {
        update_game_markets_operation op;
        op.game_id = 0;
        op.moderator = "moderator_name";
        op.markets = get_markets();

        return op;
    }

    // clang-format off
    fc::string markets_json = R"(
                              "markets": [
                                [
                                  "result_home_market",
                                  {}
                                ],
                                [
                                  "result_draw_market",
                                  {}
                                ],
                                [
                                  "result_away_market",
                                  {}
                                ],
                                [
                                  "round_market",
                                  {}
                                ],
                                [
                                  "handicap_market",
                                  {
                                    "threshold": -500
                                  }
                                ],
                                [
                                  "handicap_market",
                                  {
                                    "threshold": 0
                                  }
                                ],
                                [
                                  "handicap_market",
                                  {
                                    "threshold": 1000
                                  }
                                ],
                                [
                                  "correct_score_market",
                                  {}
                                ],
                                [
                                  "correct_score_parametrized_market",
                                  {
                                    "home": 1,
                                    "away": 0
                                  }
                                ],
                                [
                                  "correct_score_parametrized_market",
                                  {
                                    "home": 1,
                                    "away": 1
                                  }
                                ],
                                [
                                  "goal_market",
                                  {}
                                ],
                                [
                                  "total_market",
                                  {
                                    "threshold": 0
                                  }
                                ],
                                [
                                  "total_market",
                                  {
                                    "threshold": 500
                                  }
                                ],
                                [
                                  "total_market",
                                  {
                                    "threshold": 1000
                                  }
                                ]
                              ]
                              )";
    fc::string create_markets_json_tpl = "{\"moderator\":\"moderator_name\", \"name\":\"game_name\",\"start\":\"2016-04-25T17:30:00\", \"game\":[\"soccer_game\",{}], ${markets}}";
    fc::string update_markets_json_tpl = "{\"moderator\":\"moderator_name\", \"game_id\":0,${markets}}";
    // clang-format on

    fc::flat_set<betting::market_type> get_markets() const
    {
        // clang-format off
        return { result_home_market{},
                 result_draw_market{},
                 result_away_market{},
                 round_market{},
                 handicap_market{1000},
                 handicap_market{-500},
                 handicap_market{0},
                 correct_score_market{},
                 correct_score_parametrized_market{1, 1},
                 correct_score_parametrized_market{1, 0},
                 goal_market{},
                 total_market{0},
                 total_market{500},
                 total_market{1000} };
        // clang-format on
    }

    void validate_soccer_create_game_operation(const create_game_operation& obj) const
    {
        BOOST_CHECK_EQUAL(obj.moderator, "moderator_name");
        BOOST_CHECK_EQUAL(obj.name, "game_name");
        BOOST_CHECK(obj.start == time_point_sec{ 1461605400 });
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
        var.visit([&](const auto& market) { result = market.kind; });
        return result;
    }

    wincase_pairs_type create_wincase_pairs(const market_type& var) const
    {
        wdump((var));
        wincase_pairs_type result;
        var.visit([&](const auto& market) {
            result = market.create_wincase_pairs();
            for (const auto& wp : result)
            {
                wdump((wp.first));
                wdump((wp.second));
            }
        });
        return result;
    }

    void validate_markets(const fc::flat_set<betting::market_type>& markets) const
    {
        BOOST_REQUIRE_EQUAL(markets.size(), 14u);

        auto pos = 0u;

        BOOST_CHECK(get_market_kind(*markets.nth(pos)) == market_kind::result);
        auto market_wincases = create_wincase_pairs(*markets.nth(pos));
        BOOST_CHECK_EQUAL(market_wincases.size(), 1u);

        pos += 3u;

        BOOST_CHECK(get_market_kind(*markets.nth(pos)) == market_kind::round);
        market_wincases = create_wincase_pairs(*markets.nth(pos));
        BOOST_CHECK_EQUAL(market_wincases.size(), 1u);

        pos += 1u;

        BOOST_CHECK(get_market_kind(*markets.nth(pos)) == market_kind::handicap);
        market_wincases = create_wincase_pairs(*markets.nth(pos));
        BOOST_CHECK_EQUAL(market_wincases.size(), 1u);
        BOOST_CHECK_EQUAL(market_wincases.nth(0)->first.get<handicap_home_over>().threshold, -500);
        BOOST_CHECK_EQUAL(market_wincases.nth(0)->second.get<handicap_home_under>().threshold, -500);

        pos += 3u;

        BOOST_CHECK(get_market_kind(*markets.nth(pos)) == market_kind::correct_score);
        market_wincases = create_wincase_pairs(*markets.nth(pos));
        BOOST_CHECK_EQUAL(market_wincases.size(), 3u);

        pos += 1u;

        BOOST_CHECK(get_market_kind(*markets.nth(pos)) == market_kind::correct_score);
        market_wincases = create_wincase_pairs(*markets.nth(pos));
        BOOST_CHECK_EQUAL(market_wincases.size(), 1u);
        BOOST_CHECK_EQUAL(market_wincases.nth(0)->first.get<correct_score_yes>().home, 1);
        BOOST_CHECK_EQUAL(market_wincases.nth(0)->first.get<correct_score_yes>().away, 0);
        BOOST_CHECK_EQUAL(market_wincases.nth(0)->second.get<correct_score_no>().home, 1);
        BOOST_CHECK_EQUAL(market_wincases.nth(0)->second.get<correct_score_no>().away, 0);

        pos += 2u;

        BOOST_CHECK(get_market_kind(*markets.nth(pos)) == market_kind::goal);
        BOOST_CHECK_EQUAL(market_wincases.size(), 1u);

        pos += 1u;

        BOOST_CHECK(get_market_kind(*markets.nth(pos)) == market_kind::total);
        BOOST_CHECK_EQUAL(market_wincases.size(), 1u);
    }
};

BOOST_FIXTURE_TEST_SUITE(game_serialization_tests, game_serialization_test_fixture)

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
    op.game = soccer_game{};
    op.markets = { correct_score_market{}, correct_score_market{} };

    auto json = fc::json::to_string(op);
    // clang-format off
    auto json_comp = fc::json::to_string(fc::json::from_string(
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
                                                    "correct_score_market",
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
                                                    "correct_score_market",
                                                    {}
                                                 ],
                                                 [
                                                    "correct_score_market",
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
    op.game = soccer_game{};
    op.markets = { correct_score_parametrized_market{ 1, 1 }, correct_score_market{} };

    auto json = fc::json::to_string(op);
    // clang-format off
    auto json_comp = fc::json::to_string(fc::json::from_string(
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
                                                    "correct_score_market",
                                                    {}
                                                 ],
                                                 [
                                                    "correct_score_parametrized_market",
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
                                              "start":"1970-01-01T00:00:00",
                                              "game":[
                                                 "soccer_game",
                                                 {}
                                              ],
                                              "markets":[
                                                  [
                                                  "correct_score_parametrized_market",
                                                      {
                                                        "home":1,
                                                        "away":1
                                                      }
                                                   ],
                                                   [
                                                       "correct_score_parametrized_market",
                                                       {
                                                         "home":1,
                                                         "away":1
                                                       }
                                                   ],
                                                   [
                                                       "correct_score_market",
                                                       {}
                                                   ],
                                                   [
                                                       "correct_score_market",
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

    BOOST_CHECK_EQUAL(hex, "0e6d6f64657261746f725f6e616d650967616d655f6e616d6518541e57000e000102030"
                           "40cfe04000004e80305060100000006010001000708000008f40108e803");
}

SCORUM_TEST_CASE(create_game_binary_deserialization_test)
{
    auto hex = "0e6d6f64657261746f725f6e616d650967616d655f6e616d6518541e57000e000102030"
               "40cfe04000004e80305060100000006010001000708000008f40108e803";

    char buffer[1000];
    fc::from_hex(hex, buffer, sizeof(buffer));
    auto obj = fc::raw::unpack<create_game_operation>(buffer, sizeof(buffer));

    validate_soccer_create_game_operation(obj);
}

SCORUM_TEST_CASE(update_game_markets_binary_serialization_test)
{
    auto op = get_update_game_markets_operation();

    auto hex = fc::to_hex(fc::raw::pack(op));

    BOOST_CHECK_EQUAL(hex, "0e6d6f64657261746f725f6e616d6500000000000000000e000102030"
                           "40cfe04000004e80305060100000006010001000708000008f40108e803");
}

SCORUM_TEST_CASE(update_game_markets_binary_deserialization_test)
{
    auto hex = "0e6d6f64657261746f725f6e616d6500000000000000000e000102030"
               "40cfe04000004e80305060100000006010001000708000008f40108e803";

    char buffer[1000];
    fc::from_hex(hex, buffer, sizeof(buffer));
    auto obj = fc::raw::unpack<update_game_markets_operation>(buffer, sizeof(buffer));

    validate_update_game_markets_operation(obj);
}

BOOST_AUTO_TEST_SUITE_END()
}
