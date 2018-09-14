#include <boost/test/unit_test.hpp>

#include <scorum/chain/betting/betting_matcher.hpp>

#include "betting_common.hpp"

namespace betting_matcher_tests {

using namespace scorum::chain;
using namespace scorum::protocol;

using namespace service_wrappers;

struct betting_matcher_fixture : public betting_common::betting_service_fixture_impl
{
    using cancel_ptr = void (betting_service_i::*)(const betting_service_i::pending_bet_crefs_type&);

    betting_matcher_fixture()
        : matcher(*dbs_services, *virt_op_emitter, *betting_svc)
    {
    }

    betting_matcher matcher;
};

BOOST_FIXTURE_TEST_SUITE(betting_matcher_tests, betting_matcher_fixture)

SCORUM_TEST_CASE(matching_not_found_and_created_pending_check)
{
    mocks.OnCallOverload(betting_svc, (cancel_ptr)&betting_service_i::cancel_pending_bets);
    const auto& new_bet = create_bet();

    matcher.match(new_bet);

    BOOST_CHECK(!pending_bets.empty());
    BOOST_CHECK(matched_bets.empty());
}

SCORUM_TEST_CASE(matched_for_full_stake_check)
{
    mocks.OnCallOverload(betting_svc, (cancel_ptr)&betting_service_i::cancel_pending_bets);

    const auto& bet1 = create_bet("alice", test_bet_game, goal_home::yes(), "10/1", ASSET_SCR(1e+9));

    matcher.match(bet1);

    const auto& bet2 = create_bet("bob", test_bet_game, goal_home::no(), "10/9", ASSET_SCR(9e+9));

    mocks.OnCall(virt_op_emitter, database_virtual_operations_emmiter_i::push_virtual_operation);
    mocks.ExpectCallOverload(betting_svc, (cancel_ptr)&betting_service_i::cancel_pending_bets)
        .Do([](const betting_service_i::pending_bet_crefs_type& bets) {
            BOOST_REQUIRE_EQUAL(bets.size(), 2u);
            BOOST_CHECK_EQUAL(bets[0].get().stake.amount, 0u);
            BOOST_CHECK_EQUAL(bets[1].get().stake.amount, 0u);
        });

    matcher.match(bet2);

    BOOST_CHECK_EQUAL(matched_bets.get().stake1.amount, 1e+9);
    BOOST_CHECK_EQUAL(matched_bets.get().stake2.amount, 9e+9);
}

SCORUM_TEST_CASE(matched_for_part_stake_check)
{
    mocks.OnCallOverload(betting_svc, (cancel_ptr)&betting_service_i::cancel_pending_bets);

    const auto& bet1 = create_bet("alice", test_bet_game, goal_home::yes(), "10/1", ASSET_SCR(1000));

    matcher.match(bet1);

    // set not enough stake to pay gain 'alice'
    const auto& bet2 = create_bet("bob", test_bet_game, goal_home::no(), "10/9", ASSET_SCR(8000));

    mocks.ExpectCallOverload(betting_svc, (cancel_ptr)&betting_service_i::cancel_pending_bets)
        .Do([](const betting_service_i::pending_bet_crefs_type& bets) {
            BOOST_REQUIRE_EQUAL(bets.size(), 1u);
            BOOST_CHECK_EQUAL(bets[0].get().better, "bob");
            BOOST_CHECK_EQUAL(bets[0].get().stake.amount, 0);
        });

    matcher.match(bet2);

    BOOST_CHECK_EQUAL(matched_bets.get().better1, "alice");
    BOOST_CHECK_EQUAL(matched_bets.get().stake1.amount, 888);
    BOOST_CHECK_EQUAL(matched_bets.get().better2, "bob");
    BOOST_CHECK_EQUAL(matched_bets.get().stake2.amount, 8000);
    BOOST_CHECK_EQUAL(pending_bets.get().better, "alice");
    BOOST_CHECK_EQUAL(pending_bets.get().stake.amount, 112);
}

SCORUM_TEST_CASE(matched_for_full_stake_with_more_than_one_matching_check)
{
    mocks.OnCallOverload(betting_svc, (cancel_ptr)&betting_service_i::cancel_pending_bets);

    const auto& bet1 = create_bet("alice", test_bet_game, goal_home::yes(), "10/1", ASSET_SCR(1000));

    matcher.match(bet1);

    const auto& bet2 = create_bet("bob", test_bet_game, goal_home::no(), "10/9", ASSET_SCR(8000));

    mocks.ExpectCallOverload(betting_svc, (cancel_ptr)&betting_service_i::cancel_pending_bets)
        .Do([](const betting_service_i::pending_bet_crefs_type& bets) {
            BOOST_REQUIRE_EQUAL(bets.size(), 1u);
            BOOST_CHECK_EQUAL(bets[0].get().better, "bob");
            BOOST_CHECK_EQUAL(bets[0].get().stake.amount, 0);
        });

    matcher.match(bet2);

    BOOST_CHECK_EQUAL(matched_bets.get().stake1.amount, 888);
    BOOST_CHECK_EQUAL(matched_bets.get().stake2.amount, 8000);
    BOOST_CHECK_EQUAL(pending_bets.get(1).stake.amount, 112);

    // in order to cover 112 bet with 10/1 odds we need to bet at least 1008:
    // 112  | 10/1: r1 = 1120
    //              p1 = 1008
    // 1008 | 10/9: r2 = 1120               => r2 == r1 => b1 = 112
    //              p2 = 112                               b2 = 1008
    // 1009 | 10/9: r2 = 1121.(1) = 1121    => r2 > r1 => b1 = 112
    //              p2 = 112.(1) = 112                    b2 = p1 = 1008
    const auto& bet3 = create_bet("sam", test_bet_game, goal_home::no(), "10/9", ASSET_SCR(1009));

    mocks.ExpectCallOverload(betting_svc, (cancel_ptr)&betting_service_i::cancel_pending_bets)
        .Do([](const betting_service_i::pending_bet_crefs_type& bets) {
            BOOST_REQUIRE_EQUAL(bets.size(), 2u);
            BOOST_CHECK_EQUAL(bets[0].get().better, "alice");
            BOOST_CHECK_EQUAL(bets[0].get().stake.amount, 0);
            BOOST_CHECK_EQUAL(bets[1].get().better, "sam");
            BOOST_CHECK_EQUAL(bets[1].get().stake.amount, 1);
        });

    matcher.match(bet3);

    BOOST_CHECK_EQUAL(matched_bets.get(2).better1, "alice");
    BOOST_CHECK_EQUAL(matched_bets.get(2).stake1.amount, 112);
    BOOST_CHECK_EQUAL(matched_bets.get(2).better2, "sam");
    BOOST_CHECK_EQUAL(matched_bets.get(2).stake2.amount, 1008);
}

SCORUM_TEST_CASE(matched_from_larger_potential_result_check)
{
    mocks.OnCallOverload(betting_svc, (cancel_ptr)&betting_service_i::cancel_pending_bets);

    const auto& bet1 = create_bet("bob", test_bet_game, goal_home::no(), "10/9", ASSET_SCR(8000));

    matcher.match(bet1);

    const auto& bet2 = create_bet("sam", test_bet_game, goal_home::no(), "10/9", ASSET_SCR(1000));

    matcher.match(bet2);

    const auto& bet3 = create_bet("alice", test_bet_game, goal_home::yes(), "10/1", ASSET_SCR(1000));

    mocks.ExpectCallOverload(betting_svc, (cancel_ptr)&betting_service_i::cancel_pending_bets)
        .Do([](const betting_service_i::pending_bet_crefs_type& bets) {
            BOOST_REQUIRE_EQUAL(bets.size(), 2u);
            BOOST_CHECK_EQUAL(bets[0].get().better, "bob");
            BOOST_CHECK_EQUAL(bets[0].get().stake.amount, 0);
            BOOST_CHECK_EQUAL(bets[1].get().better, "sam");
            BOOST_CHECK_EQUAL(bets[1].get().stake.amount, 0);
        });

    matcher.match(bet3);

    BOOST_CHECK_EQUAL(matched_bets.get(1).better1, "bob");
    BOOST_CHECK_EQUAL(matched_bets.get(1).stake1.amount, 8000);
    BOOST_CHECK_EQUAL(matched_bets.get(1).better2, "alice");
    BOOST_CHECK_EQUAL(matched_bets.get(1).stake2.amount, 888);
    BOOST_CHECK_EQUAL(matched_bets.get(2).better1, "sam");
    BOOST_CHECK_EQUAL(matched_bets.get(2).stake1.amount, 1000);
    BOOST_CHECK_EQUAL(matched_bets.get(2).better2, "alice");
    BOOST_CHECK_EQUAL(matched_bets.get(2).stake2.amount, 111);

    // as we do not remove pending bets in tests, alice's pending bet is at index 3
    BOOST_CHECK_EQUAL(pending_bets.get(3).stake.amount, 1);
}

SCORUM_TEST_CASE(virt_operation_should_be_emitted_check)
{
    mocks.OnCallOverload(betting_svc, (cancel_ptr)&betting_service_i::cancel_pending_bets);
    mocks.ExpectCall(virt_op_emitter, database_virtual_operations_emmiter_i::push_virtual_operation)
        .Do([](const operation& op) {
            auto& typed_op = op.get<bets_matched_operation>();
            BOOST_CHECK_EQUAL(typed_op.better1, "alice");
            BOOST_CHECK_EQUAL(typed_op.better2, "bob");
            BOOST_CHECK_EQUAL(typed_op.matched_stake1.amount, 888);
            BOOST_CHECK_EQUAL(typed_op.matched_stake2.amount, 8000);
        });

    const auto& bet1 = create_bet("alice", test_bet_game, goal_home::yes(), "10/1", ASSET_SCR(1000));
    matcher.match(bet1);

    // set not enough stake to pay gain 'alice'
    const auto& bet2 = create_bet("bob", test_bet_game, goal_home::no(), "10/9", ASSET_SCR(8000));
    matcher.match(bet2);
}

SCORUM_TEST_CASE(extra_large_coefficient_big_gain_mismatch_test)
{
    mocks.OnCallOverload(betting_svc, (cancel_ptr)&betting_service_i::cancel_pending_bets);

    const auto& bet1 = create_bet("alice", test_bet_game, goal_home::yes(), "10000/1", ASSET_SCR(10000));

    matcher.match(bet1);

    const auto& bet2 = create_bet("bob", test_bet_game, goal_home::no(), "10000/9999", ASSET_SCR(90'000'000));

    mocks.ExpectCallOverload(betting_svc, (cancel_ptr)&betting_service_i::cancel_pending_bets)
        .Do([](const betting_service_i::pending_bet_crefs_type& bets) {
            BOOST_REQUIRE_EQUAL(bets.size(), 1u);
            BOOST_CHECK_EQUAL(bets[0].get().better, "bob");
            BOOST_CHECK_EQUAL(bets[0].get().stake.amount, 0u);
        });

    matcher.match(bet2);

    BOOST_CHECK_EQUAL(matched_bets.get().stake1.amount, 9000);
    BOOST_CHECK_EQUAL(matched_bets.get().stake2.amount, 90'000'000);

    // NOTE:
    // alice potential gain: 90'000'000; gain: 90'009'000 (gain extra 9'000 SCR)
    // bob   potential gain: 90'009'000; gain: 90'009'000 [OK]
}

SCORUM_TEST_CASE(extra_small_bet_which_cannot_be_matched_test)
{
    mocks.OnCallOverload(betting_svc, (cancel_ptr)&betting_service_i::cancel_pending_bets);

    const auto& bet1 = create_bet("alice", test_bet_game, goal_home::yes(), "10/8", ASSET_SCR(2));

    matcher.match(bet1);

    const auto& bet2 = create_bet("bob", test_bet_game, goal_home::no(), "10/2", ASSET_SCR(1000));

    mocks.ExpectCallOverload(betting_svc, (cancel_ptr)&betting_service_i::cancel_pending_bets)
        .Do([](const betting_service_i::pending_bet_crefs_type& bets) {
            BOOST_REQUIRE_EQUAL(bets.size(), 1u);
            BOOST_CHECK_EQUAL(bets[0].get().better, "alice");
            BOOST_CHECK_EQUAL(bets[0].get().stake.amount, 2u);
        });

    matcher.match(bet2);

    BOOST_CHECK(matched_bets.empty());
}

BOOST_AUTO_TEST_SUITE_END()
}
