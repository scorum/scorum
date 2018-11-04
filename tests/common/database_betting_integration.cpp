#include "database_betting_integration.hpp"

#include <fc/time.hpp>

#include <scorum/protocol/betting/wincase.hpp>
#include <scorum/protocol/betting/market.hpp>
#include <scorum/protocol/betting/game.hpp>

#include <scorum/chain/dba/db_accessor.hpp>
#include <scorum/chain/schema/game_object.hpp>
#include <scorum/chain/schema/betting_property_object.hpp>
#include <scorum/chain/schema/dynamic_global_property_object.hpp>
#include <scorum/chain/schema/proposal_object.hpp>

#include <boost/test/unit_test.hpp>

#include "detail.hpp"

namespace database_fixture {

using namespace scorum::protocol;
using namespace scorum::chain;

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

create_game_operation database_betting_integration_fixture::create_game(const scorum::uuid_type& uuid,
                                                                        const Actor& moderator,
                                                                        std::vector<market_type> markets,
                                                                        uint32_t start_delay,
                                                                        uint32_t auto_resolve_delay_sec)
{
    try
    {
        create_game_operation op;
        op.uuid = uuid;
        op.moderator = moderator.name;
        op.start_time = head_block_time() + start_delay;
        op.auto_resolve_delay_sec = auto_resolve_delay_sec;
        op.json_metadata = "{}";
        op.game = soccer_game{};
        op.markets = markets;

        push_operation_only(op, moderator.private_key);

        return op;
    }
    FC_CAPTURE_LOG_AND_RETHROW(())
}

create_game_operation database_betting_integration_fixture::create_game(const Actor& moderator,
                                                                        std::vector<market_type> markets,
                                                                        uint32_t start_delay,
                                                                        uint32_t auto_resolve_delay_sec)
{
    return create_game(gen_uuid("test"), moderator, markets, start_delay, auto_resolve_delay_sec);
}

post_bet_operation database_betting_integration_fixture::create_bet(const scorum::uuid_type& uuid,
                                                                    const Actor& better,
                                                                    const wincase_type& wincase,
                                                                    const odds_input& odds_value,
                                                                    const asset& stake,
                                                                    bool is_live)
{
    try
    {
        post_bet_operation op;
        op.uuid = uuid;
        op.better = better.name;
        op.game_uuid = dba::db_accessor<game_object>(db).get().uuid;
        op.wincase = wincase;
        op.odds = odds_value;
        op.stake = stake;
        op.live = is_live;

        push_operation_only(op, better.private_key);

        return op;
    }
    FC_CAPTURE_LOG_AND_RETHROW(())
}

cancel_pending_bets_operation
database_betting_integration_fixture::cancel_pending_bet(const Actor& better,
                                                         const std::vector<scorum::uuid_type>& bet_uuids)
{
    try
    {
        cancel_pending_bets_operation op;
        op.better = better.name;
        op.bet_uuids = bet_uuids;

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
        op.uuid = dba::db_accessor<game_object>(db).get().uuid;

        push_operation_only(op, moderator.private_key);

        return op;
    }
    FC_CAPTURE_LOG_AND_RETHROW(())
}

update_game_markets_operation database_betting_integration_fixture::update_markets(const Actor& moderator,
                                                                                   std::vector<market_type> markets)
{
    try
    {
        update_game_markets_operation op;
        op.moderator = moderator.name;
        op.uuid = dba::db_accessor<game_object>(db).get().uuid;
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
        op.uuid = dba::db_accessor<game_object>(db).get().uuid;
        op.start_time = head_block_time() + start_delay;

        push_operation_only(op, moderator.private_key);

        return op;
    }
    FC_CAPTURE_LOG_AND_RETHROW(())
}

post_game_results_operation database_betting_integration_fixture::post_results(const Actor& moderator,
                                                                               const std::vector<wincase_type>& winners)
{
    try
    {
        post_game_results_operation op;
        op.moderator = moderator.name;
        op.uuid = dba::db_accessor<game_object>(db).get().uuid;
        op.wincases = winners;

        push_operation_only(op, moderator.private_key);

        return op;
    }
    FC_CAPTURE_LOG_AND_RETHROW(())
}

proposal_id_type database_betting_integration_fixture::get_last_proposal_id()
{
    const auto proposals = dba::db_accessor<proposal_object>(db).get_all_by<by_id>();

    BOOST_REQUIRE_GT(boost::size(proposals), static_cast<size_t>(0));

    return proposals.back().id;
}

fc::time_point_sec database_betting_integration_fixture::head_block_time() const
{
    return dba::db_accessor<dynamic_global_property_object>(db).get().time;
}

account_name_type database_betting_integration_fixture::betting_moderator() const
{
    auto betting_prop_dba = dba::db_accessor<betting_property_object>(db);

    BOOST_REQUIRE(!betting_prop_dba.is_empty());

    return betting_prop_dba.get().moderator;
}
}
