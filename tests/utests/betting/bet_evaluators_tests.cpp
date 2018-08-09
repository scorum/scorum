#include <boost/test/unit_test.hpp>

#include <scorum/chain/evaluators/post_bet_evalulator.hpp>

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
        mocks.OnCall(betting_service_moc, betting_service_i::create_bet)
            .Do([&](const account_name_type& better, const game_id_type game, const wincase_type& wincase,
                    const odds& odds_value, const asset& stake) -> const bet_object& {
                return bets.create([&](bet_object& obj) {
                    obj.better = better;
                    obj.game = game;
                    obj.wincase = wincase;
                    obj.odds_value = odds_value;
                    obj.stake = stake;
                    obj.rest_stake = stake;
                });
            });

        mocks.OnCall(betting_matcher_moc, betting_matcher_i::match).Do([&](const bet_object& bet) -> void {
            pending_bets.create([&](pending_bet_object& obj) {
                obj.bet = bet.id;
                obj.game = bet.game;
            });
        });

        game_service.create([&](game_object& obj) { fc::from_string(obj.name, "test"); })
    }

    post_bet_evaluator evaluator_for_test;
};

BOOST_FIXTURE_TEST_SUITE(post_bet_evaluator_tests, post_bet_evaluator_fixture)

SCORUM_TEST_CASE(post_bet_evaluator_operation_validate_check)
{
    post_bet_operation op;

    op.better = "smit";
    op.game_id = 1;
    op.market = market_kind::correct_score;
    op.wincase = correct_score_home_yes();
    op.odds = "3/1";

    BOOST_CHECK_NO_THROW(op.validate());

    post_bet_operation old_op = op;

    op.better = "";
    BOOST_CHECK_THROW(op.validate(), fc::assert_exception);
    op = old_op;

    op.game_id = -1;
    BOOST_CHECK_THROW(op.validate(), fc::assert_exception);
    op = old_op;

    op.game_id = 0;
    BOOST_CHECK_THROW(op.validate(), fc::assert_exception);
    op = old_op;

    op.wincase = result_away();
    BOOST_CHECK_THROW(op.validate(), fc::assert_exception);
    op = old_op;

    op.odds = "11111111111111/1";
    BOOST_CHECK_THROW(op.validate(), fc::assert_exception);
    op = old_op;

    op.stake.amount = 0;
    BOOST_CHECK_THROW(op.validate(), fc::assert_exception);
    op = old_op;

    op.stake = ASSET_SP(1e+9);
    BOOST_CHECK_THROW(op.validate(), fc::assert_exception);
    op = old_op;
}

SCORUM_TEST_CASE(post_bet_evaluator_negative_check)
{
    // TODO
}

BOOST_AUTO_TEST_SUITE_END()
}
