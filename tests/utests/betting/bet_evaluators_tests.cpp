#include <boost/test/unit_test.hpp>

#include <scorum/chain/evaluators/post_bet_evalulator.hpp>
#include <scorum/chain/evaluators/cancel_pending_bets_evaluator.hpp>

#include "betting_common.hpp"

namespace bet_evaluators_tests {

using namespace scorum::chain;
using namespace scorum::chain::betting;
using namespace scorum::protocol;
using namespace scorum::protocol::betting;

struct post_bet_evaluator_fixture : public betting_common::betting_evaluator_fixture_impl
{
    post_bet_evaluator_fixture()
        : evaluator_for_test(*dbs_services, *betting_service_moc, *betting_matcher_moc)
    {
        smit.scorum(ASSET_SCR(1e+9));
        account_service.add_actor(smit);

        const auto& game = games.create([&](game_object& obj) { fc::from_string(obj.name, "test"); });

        test_op.better = smit.name;
        test_op.game_id = game.id._id;
        test_op.market = market_kind::correct_score;
        test_op.wincase = correct_score_home_yes();
        test_op.odds = "3/1";
        test_op.stake = smit.scr_amount;
    }

    bet_object create_bet()
    {
        return create_object<bet_object>(shm, [&](bet_object& obj) {
            obj.better = test_op.better;
            obj.game = test_op.game_id;
            obj.wincase = test_op.wincase;
            obj.odds_value = odds::from_string(test_op.odds);
            obj.stake = test_op.stake;
            obj.rest_stake = obj.stake;
        });
    }

    post_bet_evaluator evaluator_for_test;

    post_bet_operation test_op;

    Actor smit = "smit";
};

BOOST_FIXTURE_TEST_SUITE(post_bet_evaluator_tests, post_bet_evaluator_fixture)

SCORUM_TEST_CASE(post_bet_evaluator_operation_validate_check)
{
    post_bet_operation op = test_op;

    BOOST_CHECK_NO_THROW(op.validate());

    op.better = "";
    BOOST_CHECK_THROW(op.validate(), fc::assert_exception);
    op = test_op;

    op.game_id = -1;
    BOOST_CHECK_THROW(op.validate(), fc::assert_exception);
    op = test_op;

    op.game_id = 0;
    BOOST_CHECK_THROW(op.validate(), fc::assert_exception);
    op = test_op;

    op.wincase = result_away();
    BOOST_CHECK_THROW(op.validate(), fc::assert_exception);
    op = test_op;

    op.odds = "11111111111111/1";
    BOOST_CHECK_THROW(op.validate(), fc::assert_exception);
    op = test_op;

    op.stake.amount = 0;
    BOOST_CHECK_THROW(op.validate(), fc::assert_exception);
    op = test_op;

    op.stake = ASSET_SP(1e+9);
    BOOST_CHECK_THROW(op.validate(), fc::assert_exception);
    op = test_op;
}

SCORUM_TEST_CASE(post_bet_evaluator_negative_check)
{
    post_bet_operation op = test_op;

    op.better = "mercury";
    BOOST_CHECK_THROW(evaluator_for_test.do_apply(op), fc::assert_exception);
    op = test_op;

    op.stake = smit.scr_amount + smit.scr_amount;
    BOOST_CHECK_THROW(evaluator_for_test.do_apply(op), fc::assert_exception);
    op = test_op;
}

SCORUM_TEST_CASE(post_bet_evaluator_positive_check)
{
    auto bet = create_bet();

    mocks.ExpectCall(betting_service_moc, betting_service_i::create_bet).ReturnByRef(bet);

    mocks.ExpectCall(betting_matcher_moc, betting_matcher_i::match).With(_);

    BOOST_CHECK_NO_THROW(evaluator_for_test.do_apply(test_op));
}

BOOST_AUTO_TEST_SUITE_END()

struct cancel_pending_bets_evaluator_fixture : public betting_common::betting_evaluator_fixture_impl
{
    cancel_pending_bets_evaluator_fixture()
        : evaluator_for_test(*dbs_services, *betting_service_moc)
    {
        smit.scorum(ASSET_SCR(1e+9));
        account_service.add_actor(smit);

        account_service.service().decrease_balance(account_service.service().get_account(smit.name), smit.scr_amount);

        const auto& game = games.create([&](game_object& obj) { fc::from_string(obj.name, "test"); });
        const auto& bet1 = bets.create([&](bet_object& obj) {
            obj.better = smit.name;
            obj.game = game.id;
            obj.wincase = correct_score_home_yes();
            obj.odds_value = odds(10, 3);
            obj.stake = asset(smit.scr_amount.amount / 2, SCORUM_SYMBOL);
            obj.rest_stake = obj.stake;
        });
        const auto& bet2 = bets.create([&](bet_object& obj) {
            obj.better = smit.name;
            obj.game = game.id;
            obj.wincase = correct_score_draw_yes();
            obj.odds_value = odds(10, 3);
            obj.stake = asset(smit.scr_amount.amount / 2, SCORUM_SYMBOL);
            obj.rest_stake = asset(obj.stake.amount / 2, SCORUM_SYMBOL);
        });

        const auto& pending_bet1 = pending_bets.create([&](pending_bet_object& obj) { obj.bet = bet1.id; });
        const auto& pending_bet2 = pending_bets.create([&](pending_bet_object& obj) { obj.bet = bet2.id; });

        test_op.better = smit.name;
        bet1_id = bet1.id;
        bet2_id = bet2.id;
        test_op.bet_ids = { bet1.id._id, bet2.id._id };
        pending_bet1_id = pending_bet1.id;
        pending_bet2_id = pending_bet2.id;
    }

    cancel_pending_bets_evaluator evaluator_for_test;

    cancel_pending_bets_operation test_op;

    bet_id_type bet1_id;
    bet_id_type bet2_id;

    pending_bet_id_type pending_bet1_id;
    pending_bet_id_type pending_bet2_id;

    Actor smit = "smit";
};

BOOST_FIXTURE_TEST_SUITE(cancel_pending_bets_evaluator_tests, cancel_pending_bets_evaluator_fixture)

SCORUM_TEST_CASE(cancel_pending_bets_operation_validate_check)
{
    cancel_pending_bets_operation op = test_op;

    BOOST_CHECK_NO_THROW(op.validate());

    op.better = "";
    BOOST_CHECK_THROW(op.validate(), fc::assert_exception);
    op = test_op;

    op.bet_ids = { 0 };
    BOOST_CHECK_THROW(op.validate(), fc::assert_exception);
    op = test_op;

    op.bet_ids = { -1 };
    BOOST_CHECK_THROW(op.validate(), fc::assert_exception);
    op = test_op;
}

SCORUM_TEST_CASE(cancel_pending_bets_evaluator_negative_check)
{
    cancel_pending_bets_operation op = test_op;

    op.better = "mercury";
    BOOST_CHECK_THROW(evaluator_for_test.do_apply(op), fc::assert_exception);
    op = test_op;

    op.bet_ids = { 999 };
    BOOST_CHECK_THROW(evaluator_for_test.do_apply(op), fc::assert_exception);
    op = test_op;
}

SCORUM_TEST_CASE(cancel_pending_bets_evaluator_positive_check)
{
    mocks.ExpectCall(betting_service_moc, betting_service_i::is_betting_moderator).With(smit.name).Return(true);

    mocks.ExpectCall(betting_service_moc, betting_service_i::is_bet_matched).Do([&](const bet_object& obj) -> bool {
        return obj.rest_stake != obj.stake;
    });

    BOOST_REQUIRE_EQUAL(bets.service().get_bet(bet1_id).rest_stake, asset(smit.scr_amount.amount / 2, SCORUM_SYMBOL));
    BOOST_REQUIRE_EQUAL(bets.service().get_bet(bet2_id).rest_stake, asset(smit.scr_amount.amount / 4, SCORUM_SYMBOL));

    BOOST_REQUIRE(bets.is_exists(bet1_id));
    BOOST_REQUIRE(bets.is_exists(bet2_id));
    BOOST_REQUIRE(pending_bets.is_exists(pending_bet1_id));
    BOOST_REQUIRE(pending_bets.is_exists(pending_bet2_id));

    BOOST_CHECK_EQUAL(account_service.service().get_account(smit.name).balance, ASSET_NULL_SCR);

    BOOST_CHECK_THROW(evaluator_for_test.do_apply(test_op), fc::assert_exception);

    BOOST_CHECK_EQUAL(account_service.service().get_account(smit.name).balance,
                      asset(smit.scr_amount.amount / 2, SCORUM_SYMBOL));

    BOOST_CHECK_EQUAL(bets.service().get_bet(bet1_id).rest_stake, ASSET_NULL_SCR);
    BOOST_CHECK_EQUAL(bets.service().get_bet(bet2_id).rest_stake, ASSET_NULL_SCR);

    BOOST_CHECK(!bets.is_exists(bet1_id));
    BOOST_CHECK(bets.is_exists(bet2_id));
    BOOST_REQUIRE(!pending_bets.is_exists(pending_bet1_id));
    BOOST_REQUIRE(!pending_bets.is_exists(pending_bet2_id));
}

BOOST_AUTO_TEST_SUITE_END()
}
