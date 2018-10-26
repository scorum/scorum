#include <boost/test/unit_test.hpp>

#include <scorum/protocol/operations.hpp>
#include <scorum/protocol/betting/game.hpp>
#include <scorum/protocol/betting/market.hpp>

#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>

#include <defines.hpp>
#include <iostream>

namespace {
using namespace scorum;
using namespace scorum::protocol;

struct post_game_results_serialization_test_fixture
{
    uuid_type game_uuid = boost::uuids::string_generator()("e629f9aa-6b2c-46aa-8fa8-36770e7a7a5f");

    post_game_results_operation create_post_game_results_operation() const
    {
        post_game_results_operation op;
        op.uuid = game_uuid;
        op.moderator = "homer";
        op.wincases = wincases;

        return op;
    }

    void validate_post_game_results_operation(const post_game_results_operation& op) const
    {
        BOOST_CHECK_EQUAL(op.moderator, "homer");
        BOOST_CHECK_EQUAL(op.uuid, game_uuid);

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

    const std::string post_results_json_tpl = R"({ "uuid":"e629f9aa-6b2c-46aa-8fa8-36770e7a7a5f",
                                                   "moderator": "homer",
                                                   "wincases": ${wincases}
                                                 })";
};

BOOST_FIXTURE_TEST_SUITE(post_game_results_serialization_tests, post_game_results_serialization_test_fixture)

SCORUM_TEST_CASE(serialize_wincases)
{
    BOOST_CHECK_EQUAL(fc::to_hex(fc::raw::pack(wincase_type(result_home::yes()))), "00");
    BOOST_CHECK_EQUAL(fc::to_hex(fc::raw::pack(wincase_type(result_home::no()))), "01");
    BOOST_CHECK_EQUAL(fc::to_hex(fc::raw::pack(wincase_type(result_draw::yes()))), "02");
    BOOST_CHECK_EQUAL(fc::to_hex(fc::raw::pack(wincase_type(result_draw::no()))), "03");
    BOOST_CHECK_EQUAL(fc::to_hex(fc::raw::pack(wincase_type(result_away::yes()))), "04");
    BOOST_CHECK_EQUAL(fc::to_hex(fc::raw::pack(wincase_type(result_away::no()))), "05");
    BOOST_CHECK_EQUAL(fc::to_hex(fc::raw::pack(wincase_type(round_home::yes()))), "06");
    BOOST_CHECK_EQUAL(fc::to_hex(fc::raw::pack(wincase_type(round_home::no()))), "07");
    BOOST_CHECK_EQUAL(fc::to_hex(fc::raw::pack(wincase_type(handicap::over()))), "080000");
    BOOST_CHECK_EQUAL(fc::to_hex(fc::raw::pack(wincase_type(handicap::under()))), "090000");
    BOOST_CHECK_EQUAL(fc::to_hex(fc::raw::pack(wincase_type(correct_score_home::yes()))), "0a");
    BOOST_CHECK_EQUAL(fc::to_hex(fc::raw::pack(wincase_type(correct_score_home::no()))), "0b");
    BOOST_CHECK_EQUAL(fc::to_hex(fc::raw::pack(wincase_type(correct_score_draw::yes()))), "0c");
    BOOST_CHECK_EQUAL(fc::to_hex(fc::raw::pack(wincase_type(correct_score_draw::no()))), "0d");
    BOOST_CHECK_EQUAL(fc::to_hex(fc::raw::pack(wincase_type(correct_score_away::yes()))), "0e");
    BOOST_CHECK_EQUAL(fc::to_hex(fc::raw::pack(wincase_type(correct_score_away::no()))), "0f");
    BOOST_CHECK_EQUAL(fc::to_hex(fc::raw::pack(wincase_type(correct_score::yes()))), "1000000000");
    BOOST_CHECK_EQUAL(fc::to_hex(fc::raw::pack(wincase_type(correct_score::no()))), "1100000000");
    BOOST_CHECK_EQUAL(fc::to_hex(fc::raw::pack(wincase_type(goal_home::yes()))), "12");
    BOOST_CHECK_EQUAL(fc::to_hex(fc::raw::pack(wincase_type(goal_home::no()))), "13");
    BOOST_CHECK_EQUAL(fc::to_hex(fc::raw::pack(wincase_type(goal_both::yes()))), "14");
    BOOST_CHECK_EQUAL(fc::to_hex(fc::raw::pack(wincase_type(goal_both::no()))), "15");
    BOOST_CHECK_EQUAL(fc::to_hex(fc::raw::pack(wincase_type(goal_away::yes()))), "16");
    BOOST_CHECK_EQUAL(fc::to_hex(fc::raw::pack(wincase_type(goal_away::no()))), "17");
    BOOST_CHECK_EQUAL(fc::to_hex(fc::raw::pack(wincase_type(total::over()))), "180000");
    BOOST_CHECK_EQUAL(fc::to_hex(fc::raw::pack(wincase_type(total::under()))), "190000");
    BOOST_CHECK_EQUAL(fc::to_hex(fc::raw::pack(wincase_type(total_goals_home::over()))), "1a0000");
    BOOST_CHECK_EQUAL(fc::to_hex(fc::raw::pack(wincase_type(total_goals_home::under()))), "1b0000");
    BOOST_CHECK_EQUAL(fc::to_hex(fc::raw::pack(wincase_type(total_goals_away::over()))), "1c0000");
    BOOST_CHECK_EQUAL(fc::to_hex(fc::raw::pack(wincase_type(total_goals_away::under()))), "1d0000");
}

SCORUM_TEST_CASE(post_game_results_json_serialization_test)
{
    auto op = create_post_game_results_operation();

    auto json = fc::json::to_string(op);

    auto json_expected
        = fc::format_string(post_results_json_tpl, fc::mutable_variant_object()("wincases", wincases_json));
    json_expected = fc::json::to_string(fc::json::from_string(json_expected));

    BOOST_CHECK_EQUAL(json, json_expected);
}

SCORUM_TEST_CASE(post_game_results_json_deserialization_test)
{
    auto json = fc::format_string(post_results_json_tpl, fc::mutable_variant_object()("wincases", wincases_json));
    auto obj = fc::json::from_string(json).as<post_game_results_operation>();

    validate_post_game_results_operation(obj);
}

SCORUM_TEST_CASE(post_game_results_binary_serialization_test)
{
    auto op = create_post_game_results_operation();

    auto hex = fc::to_hex(fc::raw::pack(op));

    BOOST_CHECK_EQUAL(hex, "e629f9aa6b2c46aa8fa836770e7a7a5f05686f6d6572110003040708e803090cfe0900000a0d0f1001000200110"
                           "300020012151618000019e803");
}

SCORUM_TEST_CASE(post_game_results_binary_deserialization_test)
{
    auto hex = "e629f9aa6b2c46aa8fa836770e7a7a5f05686f6d6572110003040708e803090cfe0900000a0d0f1001000200110"
               "300020012151618000019e803";

    char buffer[1000];
    fc::from_hex(hex, buffer, sizeof(buffer));
    auto obj = fc::raw::unpack<post_game_results_operation>(buffer, sizeof(buffer));

    validate_post_game_results_operation(obj);
}

BOOST_AUTO_TEST_SUITE_END()
}
