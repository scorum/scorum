#include "database_betting_integration.hpp"

#include <scorum/protocol/betting/wincase.hpp>
#include <scorum/protocol/betting/market.hpp>
#include <scorum/protocol/betting/game.hpp>
#include <fc/time.hpp>

#include <boost/test/unit_test.hpp>

namespace database_fixture {

using namespace scorum::protocol::betting;

database_betting_integration_fixture::database_betting_integration_fixture()
    : dgp_service(db.dynamic_global_property_service())
    , betting_property_service(db.betting_property_service())
    , account_service(db.account_service())
    , game_service(db.game_service())
{
}

void database_betting_integration_fixture::empower_moderator(const Actor& moderator)
{
    try
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
    FC_CAPTURE_LOG_AND_RETHROW(())
}

create_game_operation database_betting_integration_fixture::create_game(const Actor& moderator,
                                                                        fc::flat_set<betting::market_type> markets,
                                                                        uint32_t start_delay,
                                                                        uint32_t auto_resolve_delay_sec)
{
    try
    {
        create_game_operation op;
        op.moderator = moderator.name;
        op.start_time = dgp_service.head_block_time() + start_delay;
        op.auto_resolve_delay_sec = auto_resolve_delay_sec;
        op.name = "test";
        op.game = soccer_game{};
        op.markets = markets;

        push_operation_only(op, moderator.private_key);

        return op;
    }
    FC_CAPTURE_LOG_AND_RETHROW(())
}

post_bet_operation database_betting_integration_fixture::create_bet(const Actor& better,
                                                                    const betting::wincase_type& wincase,
                                                                    const odds_input& odds_value,
                                                                    const asset& stake)
{
    try
    {
        post_bet_operation op;
        op.better = better.name;
        op.game_id = game_service.get_game("test").id._id;
        op.wincase = wincase;
        op.odds = odds_value;
        op.stake = stake;

        push_operation_only(op, better.private_key);

        return op;
    }
    FC_CAPTURE_LOG_AND_RETHROW(())
}

cancel_pending_bets_operation
database_betting_integration_fixture::cancel_pending_bet(const Actor& better, const fc::flat_set<int64_t>& bet_ids)
{
    try
    {
        cancel_pending_bets_operation op;
        op.better = better.name;
        op.bet_ids = bet_ids;

        push_operation_only(op, better.private_key);

        return op;
    }
    FC_CAPTURE_LOG_AND_RETHROW(())
}

cancel_game_operation database_betting_integration_fixture::cancel_game(const Actor& moderator)
{
    try
    {
        cancel_game_operation op;
        op.moderator = moderator.name;
        op.game_id = game_service.get_game("test").id._id;

        push_operation_only(op, moderator.private_key);

        return op;
    }
    FC_CAPTURE_LOG_AND_RETHROW(())
}

update_game_markets_operation
database_betting_integration_fixture::update_markets(const Actor& moderator, fc::flat_set<betting::market_type> markets)
{
    try
    {
        update_game_markets_operation op;
        op.moderator = moderator.name;
        op.game_id = game_service.get_game("test").id._id;
        op.markets = markets;

        push_operation_only(op, moderator.private_key);

        return op;
    }
    FC_CAPTURE_LOG_AND_RETHROW(())
}

update_game_start_time_operation database_betting_integration_fixture::update_start_time(const Actor& moderator,
                                                                                         uint32_t start_delay)
{
    try
    {
        update_game_start_time_operation op;
        op.moderator = moderator.name;
        op.game_id = game_service.get_game("test").id._id;
        op.start_time = dgp_service.head_block_time() + start_delay;

        push_operation_only(op, moderator.private_key);

        return op;
    }
    FC_CAPTURE_LOG_AND_RETHROW(())
}

post_game_results_operation
database_betting_integration_fixture::post_results(const Actor& moderator,
                                                   const fc::flat_set<betting::wincase_type>& winners)
{
    try
    {
        post_game_results_operation op;
        op.moderator = moderator.name;
        op.game_id = game_service.get_game("test").id._id;
        op.wincases = winners;

        push_operation_only(op, moderator.private_key);

        return op;
    }
    FC_CAPTURE_LOG_AND_RETHROW(())
}

proposal_id_type database_betting_integration_fixture::get_last_proposal_id()
{
    auto& proposal_service = db.obtain_service<dbs_proposal>();
    std::vector<proposal_object::cref_type> proposals = proposal_service.get_proposals();

    BOOST_REQUIRE_GT(proposals.size(), static_cast<size_t>(0));

    return proposals[proposals.size() - 1].get().id;
}
}
