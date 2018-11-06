#include <boost/test/unit_test.hpp>

#include <scorum/protocol/operations.hpp>
#include <scorum/protocol/betting/game.hpp>
#include <scorum/protocol/betting/market.hpp>
#include <scorum/protocol/betting/wincase.hpp>

#include <boost/uuid/uuid_generators.hpp>

#include <defines.hpp>
#include <detail.hpp>
#include <iostream>

using ::detail::to_hex;
using ::detail::flatten;

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
        op.json_metadata = "{}";
        op.game = soccer_game{};
        op.start_time = time_point_sec{ 1461605400 };
        op.auto_resolve_delay_sec = 33;
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
                                                     "json_metadata": "{}",
                                                     "start_time": "2016-04-25T17:30:00",
                                                     "auto_resolve_delay_sec": 33,
                                                     "game": [ "soccer_game", {} ],
                                                     "markets": ${markets} })";

    const std::string update_markets_json_tpl = R"({ "uuid":"e629f9aa-6b2c-46aa-8fa8-36770e7a7a5f",
                                                     "moderator": "moderator_name",
                                                     "markets": ${markets} })";

    std::vector<market_type> get_markets() const
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
        BOOST_CHECK(obj.start_time == time_point_sec{ 1461605400 });
        BOOST_CHECK_NO_THROW(obj.game.get<soccer_game>());

        validate_markets(obj.markets);
    }

    market_kind get_market_kind(const market_type& var) const
    {
        market_kind result;
        var.visit([&](const auto& market) { result = market.kind_v; });
        return result;
    }

    void validate_markets(const std::vector<market_type>& m) const
    {
        fc::flat_set<market_type> markets(m.begin(), m.end());

        FC_ASSERT(m.size() == markets.size());

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

SCORUM_TEST_CASE(serialize_markets)
{
    BOOST_CHECK_EQUAL(to_hex(market_type(result_home())), "00");
    BOOST_CHECK_EQUAL(to_hex(market_type(result_draw())), "01");
    BOOST_CHECK_EQUAL(to_hex(market_type(result_away())), "02");
    BOOST_CHECK_EQUAL(to_hex(market_type(round_home())), "03");
    BOOST_CHECK_EQUAL(to_hex(market_type(handicap())), "040000");
    BOOST_CHECK_EQUAL(to_hex(market_type(correct_score_home())), "05");
    BOOST_CHECK_EQUAL(to_hex(market_type(correct_score_draw())), "06");
    BOOST_CHECK_EQUAL(to_hex(market_type(correct_score_away())), "07");
    BOOST_CHECK_EQUAL(to_hex(market_type(correct_score())), "0800000000");
    BOOST_CHECK_EQUAL(to_hex(market_type(goal_home())), "09");
    BOOST_CHECK_EQUAL(to_hex(market_type(goal_both())), "0a");
    BOOST_CHECK_EQUAL(to_hex(market_type(goal_away())), "0b");
    BOOST_CHECK_EQUAL(to_hex(market_type(total())), "0c0000");
    BOOST_CHECK_EQUAL(to_hex(market_type(total_goals_home())), "0d0000");
    BOOST_CHECK_EQUAL(to_hex(market_type(total_goals_away())), "0e0000");
}

SCORUM_TEST_CASE(create_game_op_each_piece_serialization_test)
{
    BOOST_CHECK_EQUAL(to_hex(game_uuid), "e629f9aa6b2c46aa8fa836770e7a7a5f");
    BOOST_CHECK_EQUAL(to_hex(account_name_type("admin")), "0561646d696e");
    BOOST_CHECK_EQUAL(to_hex(std::string("{}")), "027b7d");
    BOOST_CHECK_EQUAL(to_hex(time_point_sec::from_iso_string("2018-08-03T10:12:43")), "9b2a645b");
    BOOST_CHECK_EQUAL(to_hex((uint32_t)33), "21000000");
    BOOST_CHECK_EQUAL(to_hex(game_type(soccer_game{})), "00");
    BOOST_CHECK_EQUAL(to_hex(std::vector<market_type>{}), "00");
}

SCORUM_TEST_CASE(serialize_soccer_with_empty_markets)
{
    create_game_operation op;
    op.uuid = game_uuid;
    op.moderator = "admin";
    op.json_metadata = "{}";
    op.start_time = time_point_sec::from_iso_string("2018-08-03T10:12:43");
    op.auto_resolve_delay_sec = 33;
    op.game = soccer_game{};
    op.markets = {};

    scorum::protocol::operation ops = op;
    auto hex = to_hex(ops);

    BOOST_CHECK_EQUAL(hex, "23e629f9aa6b2c46aa8fa836770e7a7a5f0561646d696e027b7d9b2a645b210000000000");
}

SCORUM_TEST_CASE(serialize_cancel_game)
{
    cancel_game_operation op;
    op.uuid = game_uuid;
    op.moderator = "admin";

    scorum::protocol::operation ops = op;
    auto hex = to_hex(ops);

    BOOST_CHECK_EQUAL(hex, "24e629f9aa6b2c46aa8fa836770e7a7a5f0561646d696e");
}

SCORUM_TEST_CASE(serialize_update_game_start_time)
{
    update_game_start_time_operation op;
    op.uuid = game_uuid;
    op.moderator = "admin";
    op.start_time = time_point_sec::from_iso_string("2018-08-03T10:12:43");

    scorum::protocol::operation ops = op;
    auto hex = to_hex(ops);

    BOOST_CHECK_EQUAL(hex, "26e629f9aa6b2c46aa8fa836770e7a7a5f0561646d696e9b2a645b");
}

SCORUM_TEST_CASE(serialize_update_game_markets)
{
    update_game_markets_operation op;
    op.uuid = game_uuid;
    op.moderator = "admin";
    op.markets = { total{ 1000 } };

    scorum::protocol::operation ops = op;
    auto hex = to_hex(ops);

    BOOST_CHECK_EQUAL(hex, "25e629f9aa6b2c46aa8fa836770e7a7a5f0561646d696e010ce803");
}

SCORUM_TEST_CASE(serialize_soccer_with_total_1000)
{
    create_game_operation op;
    op.uuid = game_uuid;
    op.moderator = "admin";
    op.json_metadata = "{}";
    op.start_time = time_point_sec::from_iso_string("2018-08-03T10:12:43");
    op.auto_resolve_delay_sec = 33;
    op.game = soccer_game{};
    op.markets = { total{ 1000 } };

    scorum::protocol::operation ops = op;
    auto hex = to_hex(ops);

    BOOST_CHECK_EQUAL(hex, "23e629f9aa6b2c46aa8fa836770e7a7a5f0561646d696e027b7d9b2a645b2100000000010ce803");
}

SCORUM_TEST_CASE(serialize_post_game_results_to_hex)
{
    post_game_results_operation op;
    op.uuid = game_uuid;
    op.moderator = "admin";
    op.wincases = { correct_score::yes{ 17, 23 } };

    scorum::protocol::operation ops = op;
    auto hex = to_hex(ops);

    BOOST_CHECK_EQUAL(hex, "27e629f9aa6b2c46aa8fa836770e7a7a5f0561646d696e011011001700");
}


SCORUM_TEST_CASE(serialize_post_bet_to_hex)
{
    post_bet_operation op;
    op.uuid = game_uuid;  // should be user_uuid
    op.better = "admin";
    op.game_uuid = game_uuid;
    op.wincase = correct_score::yes{ 17, 23 };
    op.odds = { 1, 2 };
    op.stake = asset::from_string("10.000000000 SCR");
    op.live = true;

    scorum::protocol::operation ops = op;
    auto hex = to_hex(ops);

    BOOST_CHECK_EQUAL(hex, "28e629f9aa6b2c46aa8fa836770e7a7a5f0561646d696ee629f9aa6b2c46aa8fa836770e7a7a5f10110017000"
        "10000000200000000e40b5402000000095343520000000001");
}

SCORUM_TEST_CASE(serialize_cancel_pending_bet_to_hex)
{
    cancel_pending_bets_operation op;
    op.bet_uuids = {game_uuid};
    op.better = "admin";

    scorum::protocol::operation ops = op;
    auto hex = to_hex(ops);

    BOOST_CHECK_EQUAL(hex, "2901e629f9aa6b2c46aa8fa836770e7a7a5f0561646d696e");
}

SCORUM_TEST_CASE(create_game_json_serialization_test)
{
    auto op = get_soccer_create_game_operation();

    BOOST_CHECK_EQUAL(flatten(R"({
                                   "uuid": "e629f9aa-6b2c-46aa-8fa8-36770e7a7a5f",
                                   "moderator": "moderator_name",
                                   "json_metadata": "{}",
                                   "start_time": "2016-04-25T17:30:00",
                                   "auto_resolve_delay_sec": 33,
                                   "game": [ "soccer_game", {} ],
                                   "markets": [
                                     [ "result_home", {} ],
                                     [ "result_draw", {} ],
                                     [ "result_away", {} ],
                                     [ "round_home", {} ],
                                     [ "handicap", { "threshold": 1000 } ],
                                     [ "handicap", { "threshold": -500 } ],
                                     [ "handicap", { "threshold": 0 } ],
                                     [ "correct_score_home", {} ],
                                     [ "correct_score_draw", {} ],
                                     [ "correct_score_away", {} ],
                                     [ "correct_score", { "home": 1, "away": 1 } ],
                                     [ "correct_score", { "home": 1, "away": 0 } ],
                                     [ "goal_home", {} ],
                                     [ "goal_both", {} ],
                                     [ "goal_away", {} ],
                                     [ "total", { "threshold": 0 } ],
                                     [ "total", { "threshold": 500 } ],
                                     [ "total", { "threshold": 1000 } ]
                                   ]
                                 })"),
                      fc::json::to_string(op));
}

SCORUM_TEST_CASE(create_game_json_deserialization_test)
{
    auto json = fc::format_string(create_markets_json_tpl, fc::mutable_variant_object()("markets", markets_json));
    auto obj = fc::json::from_string(json).as<create_game_operation>();

    validate_soccer_create_game_operation(obj);
}

SCORUM_TEST_CASE(create_game_operation_allows_put_duplicate_markets)
{
    create_game_operation op;
    op.uuid = game_uuid;
    op.json_metadata = "{}";
    op.game = soccer_game{};
    op.markets = { correct_score_home{}, correct_score_home{} };
    op.auto_resolve_delay_sec = 33;

    BOOST_CHECK_EQUAL(flatten(R"({
                                   "uuid": "e629f9aa-6b2c-46aa-8fa8-36770e7a7a5f",
                                   "moderator": "",
                                   "json_metadata": "{}",
                                   "start_time": "1970-01-01T00:00:00",
                                   "auto_resolve_delay_sec": 33,
                                   "game": [ "soccer_game", {} ],
                                   "markets": [
                                     [ "correct_score_home", {} ],
                                     [ "correct_score_home", {} ]
                                   ]
                                 })"),
                      fc::json::to_string(op));
}

SCORUM_TEST_CASE(deserialize_operation_with_duplicate_markets)
{
    auto json_with_duplicates = R"({
                                     "moderator": "",
                                     "json_metadata": "{}",
                                     "start": "1970-01-01T00:00:00",
                                     "game": [ "soccer_game", {} ],
                                     "markets": [
                                       [ "correct_score_home", {} ],
                                       [ "correct_score_home", {} ]
                                     ]
                                   })";

    auto obj = fc::json::from_string(json_with_duplicates).as<create_game_operation>();

    BOOST_REQUIRE_EQUAL(obj.markets.size(), 2u);
}

SCORUM_TEST_CASE(wincases_duplicates_serialization_test)
{
    create_game_operation op;
    op.uuid = game_uuid;
    op.json_metadata = "{}";
    op.game = soccer_game{};
    op.markets = { correct_score{ 1, 1 }, correct_score_home{} };
    op.auto_resolve_delay_sec = 33;

    BOOST_CHECK_EQUAL(flatten(R"({
                                   "uuid": "e629f9aa-6b2c-46aa-8fa8-36770e7a7a5f",
                                   "moderator": "",
                                   "json_metadata": "{}",
                                   "start_time": "1970-01-01T00:00:00",
                                   "auto_resolve_delay_sec": 33,
                                   "game": [ "soccer_game", {} ],
                                   "markets": [
                                     [ "correct_score", { "home": 1, "away": 1 } ],
                                     [ "correct_score_home", {} ]
                                   ]
                                 })"),
                      fc::json::to_string(op));
}

SCORUM_TEST_CASE(no_sorting_for_markets)
{
    create_game_operation op;
    op.markets = { correct_score{ 1, 1 }, correct_score_home{} };

    BOOST_CHECK(op.markets.at(0).which() == market_type::tag<correct_score>::value);
    BOOST_CHECK(op.markets.at(1).which() == market_type::tag<correct_score_home>::value);

    op.markets = { correct_score_home{}, correct_score{ 1, 1 } };

    BOOST_CHECK(op.markets.at(1).which() == market_type::tag<correct_score>::value);
    BOOST_CHECK(op.markets.at(0).which() == market_type::tag<correct_score_home>::value);
}

SCORUM_TEST_CASE(allow_duplicate_markets)
{
    create_game_operation op;
    op.markets = { correct_score_home{}, correct_score_home{}, correct_score_home{} };

    BOOST_REQUIRE_EQUAL(3u, op.markets.size());

    BOOST_CHECK(op.markets.at(0).which() == market_type::tag<correct_score_home>::value);
    BOOST_CHECK(op.markets.at(1).which() == market_type::tag<correct_score_home>::value);
    BOOST_CHECK(op.markets.at(2).which() == market_type::tag<correct_score_home>::value);
}

SCORUM_TEST_CASE(wincases_duplicates_deserialization_test)
{
    auto json_with_duplicates = flatten(R"({
                                             "moderator": "",
                                             "json_metadata": "{}",
                                             "start_time": "1970-01-01T00:00:00",
                                             "game": [ "soccer_game", {} ],
                                             "markets": [
                                               [ "correct_score", { "home": 1, "away": 1 } ],
                                               [ "correct_score", { "home": 1, "away": 1 } ],
                                               [ "correct_score_home", {} ],
                                               [ "correct_score_home", {} ]
                                             ]
                                           })");

    auto obj = fc::json::from_string(json_with_duplicates).as<create_game_operation>();

    BOOST_REQUIRE_EQUAL(obj.markets.size(), 4u);
}

SCORUM_TEST_CASE(create_game_binary_serialization_test)
{
    auto op = get_soccer_create_game_operation();

    auto hex = to_hex(op);

    BOOST_CHECK_EQUAL(to_hex(op.uuid), "e629f9aa6b2c46aa8fa836770e7a7a5f");
    BOOST_CHECK_EQUAL(to_hex(op.moderator), "0e6d6f64657261746f725f6e616d65");
    BOOST_CHECK_EQUAL(to_hex(op.json_metadata), "027b7d");
    BOOST_CHECK_EQUAL(to_hex(op.start_time), "18541e57");
    BOOST_CHECK_EQUAL(to_hex(op.auto_resolve_delay_sec), "21000000");
    BOOST_CHECK_EQUAL(to_hex(op.game), "00");
    BOOST_CHECK_EQUAL(to_hex(op.markets),
                      "120001020304e803040cfe04000005060708010001000801000000090a0b0c00000cf4010ce803");

    BOOST_CHECK_EQUAL(hex, "e629f9aa6b2c46aa8fa836770e7a7a5f0e6d6f64657261746f725f6e616d65027b7d18541e57210000000012000"
                           "1020304e803040cfe04000005060708010001000801000000090a0b0c00000cf4010ce803");
}

SCORUM_TEST_CASE(create_game_binary_deserialization_test)
{
    auto hex = "e629f9aa6b2c46aa8fa836770e7a7a5f0e6d6f64657261746f725f6e616d65027b7d18541e57210000000012000"
               "1020304e803040cfe04000005060708010001000801000000090a0b0c00000cf4010ce803";

    char buffer[1000];
    fc::from_hex(hex, buffer, sizeof(buffer));
    auto obj = fc::raw::unpack<create_game_operation>(buffer, sizeof(buffer));

    validate_soccer_create_game_operation(obj);
}

BOOST_AUTO_TEST_SUITE_END()

struct update_game_markets_operation_fixture : game_serialization_test_fixture
{
    update_game_markets_operation get_update_game_markets_operation() const
    {
        update_game_markets_operation op;
        op.uuid = game_uuid;
        op.moderator = "moderator_name";
        op.markets = get_markets();

        return op;
    }

    void validate_update_game_markets_operation(const update_game_markets_operation& obj) const
    {
        BOOST_CHECK_EQUAL(obj.moderator, "moderator_name");

        validate_markets(obj.markets);
    }
};

BOOST_FIXTURE_TEST_SUITE(update_game_markets_serialization_tests, update_game_markets_operation_fixture)

SCORUM_TEST_CASE(update_game_markets_binary_serialization_test)
{
    auto op = get_update_game_markets_operation();

    auto hex = to_hex(op);

    BOOST_CHECK_EQUAL(hex, "e629f9aa6b2c46aa8fa836770e7a7a5f0e6d6f64657261746f725f6e616d65120001020304e803040cfe0400000"
                           "5060708010001000801000000090a0b0c00000cf4010ce803");
}

SCORUM_TEST_CASE(update_game_markets_binary_deserialization_test)
{
    auto hex = "e629f9aa6b2c46aa8fa836770e7a7a5f0e6d6f64657261746f725f6e616d651200010203040cfe04000004e8030"
               "5060708010000000801000100090a0b0c00000cf4010ce803";

    char buffer[1000];
    fc::from_hex(hex, buffer, sizeof(buffer));
    auto obj = fc::raw::unpack<update_game_markets_operation>(buffer, sizeof(buffer));

    validate_update_game_markets_operation(obj);
}

SCORUM_TEST_CASE(update_game_markets_json_serialization_test)
{
    auto op = get_update_game_markets_operation();

    auto json = fc::json::to_string(op);

    BOOST_CHECK_EQUAL(flatten(R"({
                                   "uuid": "e629f9aa-6b2c-46aa-8fa8-36770e7a7a5f",
                                   "moderator": "moderator_name",
                                   "markets": [
                                     [ "result_home", {} ],
                                     [ "result_draw", {} ],
                                     [ "result_away", {} ],
                                     [ "round_home", {} ],
                                     [ "handicap", { "threshold": 1000 } ],
                                     [ "handicap", { "threshold": -500 } ],
                                     [ "handicap", { "threshold": 0 } ],
                                     [ "correct_score_home", {} ],
                                     [ "correct_score_draw", {} ],
                                     [ "correct_score_away", {} ],
                                     [ "correct_score", { "home": 1, "away": 1 } ],
                                     [ "correct_score", { "home": 1, "away": 0 } ],
                                     [ "goal_home", {} ],
                                     [ "goal_both", {} ],
                                     [ "goal_away", {} ],
                                     [ "total", { "threshold": 0 } ],
                                     [ "total", { "threshold": 500 } ],
                                     [ "total", { "threshold": 1000 } ]
                                   ]
                                 })"),
                      fc::json::to_string(op));
}

SCORUM_TEST_CASE(update_game_markets_json_deserialization_test)
{
    auto json = fc::format_string(update_markets_json_tpl, fc::mutable_variant_object()("markets", markets_json));
    auto obj = fc::json::from_string(json).as<update_game_markets_operation>();

    validate_update_game_markets_operation(obj);
}

SCORUM_TEST_CASE(no_sorting_for_markets)
{
    update_game_markets_operation op;
    op.markets = { correct_score{ 1, 1 }, correct_score_home{} };

    BOOST_CHECK(op.markets.at(0).which() == market_type::tag<correct_score>::value);
    BOOST_CHECK(op.markets.at(1).which() == market_type::tag<correct_score_home>::value);

    op.markets = { correct_score_home{}, correct_score{ 1, 1 } };

    BOOST_CHECK(op.markets.at(1).which() == market_type::tag<correct_score>::value);
    BOOST_CHECK(op.markets.at(0).which() == market_type::tag<correct_score_home>::value);
}

SCORUM_TEST_CASE(allow_duplicate_markets)
{
    update_game_markets_operation op;
    op.markets = { correct_score_home{}, correct_score_home{}, correct_score_home{} };

    BOOST_REQUIRE_EQUAL(3u, op.markets.size());

    BOOST_CHECK(op.markets.at(0).which() == market_type::tag<correct_score_home>::value);
    BOOST_CHECK(op.markets.at(1).which() == market_type::tag<correct_score_home>::value);
    BOOST_CHECK(op.markets.at(2).which() == market_type::tag<correct_score_home>::value);
}

SCORUM_TEST_CASE(validate_dont_throw_exception_on_duplicate_markets)
{
    update_game_markets_operation op;
    op.moderator = "alice";
    op.markets = { correct_score_home{}, correct_score_home{} };

    BOOST_CHECK_NO_THROW(op.validate());
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE(create_game_operation_validate_tests)

SCORUM_TEST_CASE(create_game_operation_validate_throws_exception_on_duplicate_markets)
{
    create_game_operation op;
    op.uuid = { { 1 } };
    op.game = soccer_game{};
    op.markets = { correct_score_home{}, correct_score_home{} };
    op.auto_resolve_delay_sec = 33;

    BOOST_CHECK_THROW(op.validate(), fc::assert_exception);
}

BOOST_AUTO_TEST_CASE(throw_when_market_have_invalid_threshold)
{
    std::vector<market_type> invalid_markets = { total{ 0 },
                                                 total{ 100 },
                                                 total{ 600 },
                                                 total{ -500 },
                                                 handicap{ 100 },
                                                 handicap{ 600 },
                                                 handicap{ -100 },
                                                 handicap{ -600 },
                                                 total_goals_away{ 0 },
                                                 total_goals_away{ 100 },
                                                 total_goals_away{ 600 },
                                                 total_goals_away{ -500 },
                                                 total_goals_home{ 0 },
                                                 total_goals_home{ 100 },
                                                 total_goals_home{ 600 },
                                                 total_goals_home{ -500 } };

    for (const auto& m : invalid_markets)
    {
        create_game_operation op;
        op.moderator = "alice";
        op.uuid = { { 1 } };
        op.game = soccer_game{};
        op.markets = { m };
        op.auto_resolve_delay_sec = 33;

        auto wincase = create_wincases(m).first;

        const auto expected_message = fc::format_string("Wincase '${w}' is invalid", { "w", wincase });

        SCORUM_CHECK_EXCEPTION(op.validate(), fc::assert_exception, expected_message);
    }
}

SCORUM_TEST_CASE(no_throw_when_market_have_valid_threshold)
{
    std::vector<market_type> valid_markets
        = { total{ 500 },  handicap{ 500 },  total_goals_away{ 500 },  total_goals_home{ 500 },
            total{ 1000 }, handicap{ 1000 }, total_goals_away{ 1000 }, total_goals_home{ 1000 },
            total{ 6000 }, handicap{ 6000 }, total_goals_away{ 6000 }, total_goals_home{ 6000 },
            handicap{ 0 }, handicap{ -500 }, handicap{ -2000 } };

    for (const auto& m : valid_markets)
    {
        create_game_operation op;
        op.moderator = "alice";
        op.uuid = { { 1 } };
        op.game = soccer_game{};
        op.markets = { m };
        op.auto_resolve_delay_sec = 33;

        BOOST_CHECK_NO_THROW(op.validate());
    }
}

BOOST_AUTO_TEST_SUITE_END()
}
