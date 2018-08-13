#include <boost/test/unit_test.hpp>

#include "defines.hpp"

#include "database_default_integration.hpp"
#include "actor.hpp"

#include <scorum/protocol/proposal_operations.hpp>

#include <scorum/chain/services/dynamic_global_property.hpp>
#include <scorum/chain/services/proposal.hpp>
#include <scorum/chain/services/betting_property.hpp>
#include <scorum/chain/services/account.hpp>
#include <scorum/chain/services/game.hpp>

#include <scorum/protocol/betting/market_kind.hpp>

namespace bet_operations_tests {

using namespace scorum::protocol;
using namespace scorum::protocol::betting;
using namespace scorum::chain;

struct budget_operations_fixture : public database_fixture::database_default_integration_fixture
{
    budget_operations_fixture()
        : dgp_service(db.dynamic_global_property_service())
        , betting_property_service(db.betting_property_service())
        , account_service(db.account_service())
        , game_service(db.game_service())
    {
        alice.scorum(ASSET_SCR(1e+9));
        actor(initdelegate).create_account(alice);
        actor(initdelegate).give_sp(alice, 1e+9);
        actor(initdelegate).give_scr(alice, alice.scr_amount.amount.value);

        bob.scorum(ASSET_SCR(1e+9));
        actor(initdelegate).create_account(bob);
        actor(initdelegate).give_sp(bob, 1e+9);
        actor(initdelegate).give_scr(bob, bob.scr_amount.amount.value);

        actor(initdelegate).create_account(moderator);
        actor(initdelegate).give_sp(moderator, 1e+9);
        actor(initdelegate).give_scr(moderator, 1e+9);

        try
        {
            empower_moderator(moderator);
        }
        FC_CAPTURE_LOG_AND_RETHROW(())

        BOOST_REQUIRE(betting_property_service.is_exists());
        BOOST_REQUIRE_EQUAL(betting_property_service.get().moderator, moderator.name);

        try
        {
            create_game(moderator);
        }
        FC_CAPTURE_LOG_AND_RETHROW(())

        generate_block();
    }

    void empower_moderator(const Actor& moderator)
    {
        development_committee_empower_betting_moderator_operation operation;
        operation.account = moderator.name;

        {
            proposal_create_operation op;
            op.creator = initdelegate.name;
            op.operation = operation;
            op.lifetime_sec = SCORUM_PROPOSAL_LIFETIME_MIN_SECONDS;

            push_operation_only(op, initdelegate.private_key);
        }

        {
            proposal_vote_operation op;
            op.voting_account = initdelegate.name;
            op.proposal_id = get_last_proposal_id()._id;

            push_operation_only(op, initdelegate.private_key);
        }

        generate_block();
    }

    create_game_operation create_game(const Actor& moderator)
    {
        create_game_operation op;
        op.moderator = moderator.name;
        op.start = dgp_service.head_block_time() + fc::hours(1);
        op.name = "test";
        op.game = soccer_game{};
        op.markets = { { market_kind::result, { { result_home{}, result_draw_away{} } } },
                       { market_kind::total, { { total_under{ 2000 }, total_over{ 2000 } } } } };

        push_operation_only(op, moderator.private_key);

        return op;
    }

    post_bet_operation create_bet(const Actor& better,
                                  const market_kind& market_kind,
                                  const wincase_type& wincase,
                                  const std::string& odds_value,
                                  const asset& stake)
    {
        post_bet_operation op;
        op.better = better.name;
        op.game_id = game_service.get("test").id._id;
        op.market = market_kind;
        op.wincase = wincase;
        op.odds = odds_value;
        op.stake = stake;

        push_operation_only(op, better.private_key);

        return op;
    }

    cancel_pending_bets_operation cancel_pending_bet(const Actor& better, const fc::flat_set<int64_t>& bet_ids)
    {
        cancel_pending_bets_operation op;
        op.better = better.name;
        op.bet_ids = bet_ids;

        push_operation_only(op, better.private_key);

        return op;
    }

    proposal_id_type get_last_proposal_id()
    {
        auto& proposal_service = db.obtain_service<dbs_proposal>();
        std::vector<proposal_object::cref_type> proposals = proposal_service.get_proposals();

        BOOST_REQUIRE_GT(proposals.size(), static_cast<size_t>(0));

        return proposals[proposals.size() - 1].get().id;
    }

    Actor alice = "alice";
    Actor bob = "bob";
    Actor moderator = "smit";

    dynamic_global_property_service_i& dgp_service;
    betting_property_service_i& betting_property_service;
    account_service_i& account_service;
    game_service_i& game_service;
};

BOOST_FIXTURE_TEST_SUITE(budget_operations_tests, budget_operations_fixture)

SCORUM_TEST_CASE(post_bet_operation_check)
{
    generate_block();

    try
    {
        create_bet(alice, market_kind::result, result_home{}, "10/2",
                   asset(alice.scr_amount.amount / 2, SCORUM_SYMBOL));
    }
    FC_CAPTURE_LOG_AND_RETHROW(())

    generate_block();

    try
    {
        create_bet(bob, market_kind::result, result_home{}, "10/8", asset(bob.scr_amount.amount / 2, SCORUM_SYMBOL));
    }
    FC_CAPTURE_LOG_AND_RETHROW(())

    generate_block();
}

SCORUM_TEST_CASE(cancel_pending_single_bet_by_better_operation_check)
{
    generate_block();

    try
    {
        create_bet(alice, market_kind::result, result_home{}, "10/2",
                   asset(alice.scr_amount.amount / 2, SCORUM_SYMBOL));
    }
    FC_CAPTURE_LOG_AND_RETHROW(())

    generate_block();

    try
    {
        cancel_pending_bet(alice, { 0 });
    }
    FC_CAPTURE_LOG_AND_RETHROW(())

    generate_block();
}

SCORUM_TEST_CASE(cancel_pending_single_bet_by_moderator_operation_check)
{
    generate_block();

    BOOST_REQUIRE_NO_THROW(create_bet(alice, market_kind::result, result_home{}, "10/2",
                                      asset(alice.scr_amount.amount / 2, SCORUM_SYMBOL)));

    generate_block();

    BOOST_REQUIRE_NO_THROW(cancel_pending_bet(moderator, { 0 }));

    generate_block();
}

SCORUM_TEST_CASE(cancel_pending_some_bets_by_better_operation_check)
{
    generate_block();

    try
    {
        create_bet(alice, market_kind::total, total_under{ 2000 }, "10/2",
                   asset(alice.scr_amount.amount / 2, SCORUM_SYMBOL));
    }
    FC_CAPTURE_LOG_AND_RETHROW(())

    generate_block();

    try
    {
        create_bet(alice, market_kind::result, result_home(), "10/5",
                   asset(alice.scr_amount.amount / 2, SCORUM_SYMBOL));
    }
    FC_CAPTURE_LOG_AND_RETHROW(())

    generate_block();

    try
    {
        create_bet(bob, market_kind::total, total_over{ 2000 }, "10/8",
                   asset(bob.scr_amount.amount / 2, SCORUM_SYMBOL));
    }
    FC_CAPTURE_LOG_AND_RETHROW(())

    generate_block();

    try
    {
        cancel_pending_bet(alice, { 0, 1 });
    }
    FC_CAPTURE_LOG_AND_RETHROW(())

    generate_block();
}

SCORUM_TEST_CASE(cancel_pending_some_bets_by_moderator_operation_check)
{
    generate_block();

    BOOST_REQUIRE_NO_THROW(create_bet(alice, market_kind::total, total_under{ 2000 }, "10/2",
                                      asset(alice.scr_amount.amount / 2, SCORUM_SYMBOL)));

    generate_block();

    BOOST_REQUIRE_NO_THROW(create_bet(alice, market_kind::result, result_home(), "10/5",
                                      asset(alice.scr_amount.amount / 2, SCORUM_SYMBOL)));

    generate_block();

    BOOST_REQUIRE_NO_THROW(create_bet(bob, market_kind::total, total_over{ 2000 }, "10/8",
                                      asset(bob.scr_amount.amount / 2, SCORUM_SYMBOL)));

    generate_block();

    BOOST_REQUIRE_NO_THROW(cancel_pending_bet(moderator, { 0, 1 }));

    generate_block();
}

BOOST_AUTO_TEST_SUITE_END()
}
