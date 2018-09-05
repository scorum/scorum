#include <boost/test/unit_test.hpp>

#include <scorum/chain/betting/betting_matcher.hpp>

#include "betting_common.hpp"

namespace betting_matcher_tests {

using namespace scorum::chain;
using namespace scorum::chain::betting;
using namespace scorum::protocol;
using namespace scorum::protocol::betting;

using namespace service_wrappers;

struct betting_matcher_fixture : public betting_common::betting_service_fixture_impl
{
    betting_matcher_fixture()
        : service(*dbs_services, *virt_op_emitter)
    {
    }

    betting_matcher service;

    template <typename... Args> asset total_stake(Args... args)
    {
        std::array<bet_service_i::object_cref_type, sizeof...(args)> list = { args... };
        asset result = ASSET_NULL_SCR;
        for (const bet_service_i::object_type& b : list)
        {
            result += b.stake;
        }
        return result;
    }

    template <typename... Args> asset total_stake_rest(Args... args)
    {
        std::array<bet_service_i::object_cref_type, sizeof...(args)> list = { args... };
        asset result = ASSET_NULL_SCR;
        for (const bet_service_i::object_type& b : list)
        {
            result += b.rest_stake;
        }
        return result;
    }

    template <typename... Args> asset total_matched(Args... args)
    {
        std::array<matched_bet_service_i::object_cref_type, sizeof...(args)> list = { args... };
        asset result = ASSET_NULL_SCR;
        for (const matched_bet_service_i::object_type& mb : list)
        {
            result += mb.matched_bet1_stake;
            result += mb.matched_bet2_stake;
        }
        return result;
    }
};

BOOST_FIXTURE_TEST_SUITE(betting_matcher_tests, betting_matcher_fixture)

SCORUM_TEST_CASE(matching_not_found_and_created_pending_check)
{
    const auto& new_bet = create_bet();

    service.match(new_bet);

    BOOST_CHECK(!pending_bets.empty());
    BOOST_CHECK(matched_bets.empty());
}

SCORUM_TEST_CASE(matched_for_full_stake_check)
{
    const auto& bet1 = create_bet("alice", test_bet_game, goal_home_yes(), "10/1", ASSET_SCR(1e+9));

    service.match(bet1); // call this one bacause every bet creation followed by 'match' in evaluators

    const auto& bet2 = create_bet("bob", test_bet_game, goal_home_no(), "10/9", ASSET_SCR(9e+9));

    service.match(bet2);

    BOOST_CHECK(pending_bets.empty());
    BOOST_CHECK(!matched_bets.empty());

    BOOST_CHECK_EQUAL(matched_bets.get().bet1._id, bet2.id._id);
    BOOST_CHECK_EQUAL(matched_bets.get().bet2._id, bet1.id._id);

    BOOST_CHECK_EQUAL(matched_bets.get().matched_bet1_stake, bet2.stake);
    BOOST_CHECK_EQUAL(matched_bets.get().matched_bet2_stake, bet1.stake);

    BOOST_CHECK_EQUAL(bet1.rest_stake, ASSET_NULL_SCR);
    BOOST_CHECK_EQUAL(bet2.rest_stake, ASSET_NULL_SCR);

    asset total_start = total_stake(bet1, bet2);
    asset total_result = total_matched(matched_bets.get(1));
    total_result += total_stake_rest(bet1, bet2);
    BOOST_REQUIRE_EQUAL(total_start, total_result);
}

SCORUM_TEST_CASE(matched_for_part_stake_check)
{
    const auto& bet1 = create_bet("alice", test_bet_game, goal_home_yes(), "10/1", ASSET_SCR(1e+9)); // 1 SCR

    service.match(bet1);

    // set not enough stake to pay gain 'alice'
    const auto& bet2 = create_bet("bob", test_bet_game, goal_home_no(), "10/9", ASSET_SCR(8e+9));

    service.match(bet2);

    BOOST_CHECK(!pending_bets.empty());
    BOOST_CHECK(!matched_bets.empty());

    BOOST_CHECK_EQUAL(matched_bets.get().bet1._id, bet2.id._id);
    BOOST_CHECK_EQUAL(matched_bets.get().bet2._id, bet1.id._id);

    BOOST_CHECK_EQUAL(bet1.rest_stake, ASSET_SCR(111'111'112));
    // <- 0.(1) SCR period give accuracy lag!
    BOOST_CHECK_EQUAL(bet2.rest_stake, ASSET_NULL_SCR);

    BOOST_CHECK_EQUAL(matched_bets.get().matched_bet1_stake, bet2.stake);
    BOOST_CHECK_EQUAL(matched_bets.get().matched_bet2_stake, bet1.stake - bet1.rest_stake);

    BOOST_CHECK_EQUAL(pending_bets.get().bet._id, bet1.id._id);

    asset total_start = total_stake(bet1, bet2);
    asset total_result = total_matched(matched_bets.get(1));
    total_result += total_stake_rest(bet1, bet2);
    BOOST_REQUIRE_EQUAL(total_start, total_result);
}

SCORUM_TEST_CASE(matched_for_full_stake_with_more_than_one_matching_check)
{
    const auto& bet1 = create_bet("alice", test_bet_game, goal_home_yes(), "10/1", ASSET_SCR(1e+9)); // 1 SCR

    service.match(bet1);

    const auto& bet2 = create_bet("bob", test_bet_game, goal_home_no(), "10/9", ASSET_SCR(8e+9)); // 8 SCR

    service.match(bet2);

    BOOST_CHECK(!pending_bets.empty());
    BOOST_CHECK(!matched_bets.empty());

    BOOST_CHECK_EQUAL(matched_bets.get(1).bet1._id, bet2.id._id);
    BOOST_CHECK_EQUAL(matched_bets.get(1).bet2._id, bet1.id._id);

    BOOST_CHECK_EQUAL(matched_bets.get(1).matched_bet1_stake, bet2.stake);
    BOOST_CHECK_EQUAL(matched_bets.get(1).matched_bet2_stake, bet1.stake - bet1.rest_stake);

    auto old_bet1_rest_stake = bet1.rest_stake;

    const auto& bet3 = create_bet("sam", test_bet_game, goal_home_no(), "10/9", ASSET_SCR(1e+9)); // 1 SCR

    service.match(bet3);

    BOOST_CHECK(pending_bets.empty());
    BOOST_CHECK(!matched_bets.empty());

    BOOST_CHECK_EQUAL(matched_bets.get(2).bet1._id, bet3.id._id);
    BOOST_CHECK_EQUAL(matched_bets.get(2).bet2._id, bet1.id._id);

    BOOST_CHECK_EQUAL(matched_bets.get(2).matched_bet1_stake, bet3.stake);
    BOOST_CHECK_EQUAL(matched_bets.get(2).matched_bet2_stake, old_bet1_rest_stake - bet1.rest_stake);

    asset total_start = total_stake(bet1, bet2, bet3);
    asset total_result = total_matched(matched_bets.get(1), matched_bets.get(2));
    total_result += total_stake_rest(bet1, bet2, bet3);
    BOOST_REQUIRE_EQUAL(total_start, total_result);
}

SCORUM_TEST_CASE(matched_from_larger_potential_result_check)
{
    const auto& bet1 = create_bet("bob", test_bet_game, goal_home_no(), "10/9", ASSET_SCR(8e+9)); // 8 SCR

    service.match(bet1);

    const auto& bet2 = create_bet("sam", test_bet_game, goal_home_no(), "10/9", ASSET_SCR(1e+9)); // 1 SCR

    service.match(bet2);

    const auto& bet3 = create_bet("alice", test_bet_game, goal_home_yes(), "10/1", ASSET_SCR(1e+9)); // 1 SCR

    service.match(bet3);

    BOOST_CHECK(pending_bets.empty());
    BOOST_CHECK(!matched_bets.empty());

    BOOST_CHECK_EQUAL(matched_bets.get(1).bet1._id, bet3.id._id);
    BOOST_CHECK_EQUAL(matched_bets.get(1).bet2._id, bet1.id._id);

    BOOST_CHECK_EQUAL(matched_bets.get(1).matched_bet2_stake, bet1.stake - bet1.rest_stake);

    BOOST_CHECK_EQUAL(matched_bets.get(2).bet1._id, bet3.id._id);
    BOOST_CHECK_EQUAL(matched_bets.get(2).bet2._id, bet2.id._id);

    BOOST_CHECK_EQUAL(matched_bets.get(2).matched_bet2_stake, bet3.stake);

    asset total_start = total_stake(bet1, bet2, bet3);
    asset total_result = total_matched(matched_bets.get(1), matched_bets.get(2));
    total_result += total_stake_rest(bet1, bet2, bet3);
    BOOST_REQUIRE_EQUAL(total_start, total_result);
}

SCORUM_TEST_CASE(virt_operation_should_be_emitted_check)
{
    mocks.ExpectCall(virt_op_emitter, database_virtual_operations_emmiter_i::push_virtual_operation)
        .Do([](const operation& op) {
            auto& typed_op = op.get<bets_matched_operation>();
            BOOST_CHECK_EQUAL(typed_op.better1, "bob");
            BOOST_CHECK_EQUAL(typed_op.better2, "alice");
            BOOST_CHECK_EQUAL(typed_op.matched_stake1.amount, 8e9);
            BOOST_CHECK_EQUAL(typed_op.matched_stake2.amount, 1e9 - 111'111'112);
        });

    const auto& bet1 = create_bet("alice", test_bet_game, goal_home_yes(), "10/1", ASSET_SCR(1e+9));
    service.match(bet1);

    // set not enough stake to pay gain 'alice'
    const auto& bet2 = create_bet("bob", test_bet_game, goal_home_no(), "10/9", ASSET_SCR(8e+9));
    service.match(bet2);
}

BOOST_AUTO_TEST_SUITE_END()
}
