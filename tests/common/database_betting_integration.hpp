#pragma once

#include <scorum/chain/services/dynamic_global_property.hpp>
#include <scorum/chain/services/proposal.hpp>
#include <scorum/chain/services/betting_property.hpp>
#include <scorum/chain/services/account.hpp>
#include <scorum/chain/services/game.hpp>

#include <scorum/protocol/operations.hpp>
#include <scorum/protocol/betting/wincase.hpp>

#include "database_trx_integration.hpp"
#include <functional>

namespace database_fixture {

const uint32_t start_delay_default = SCORUM_BLOCK_INTERVAL;
const uint32_t auto_resolve_delay_default = DAYS_TO_SECONDS(1);

struct database_betting_integration_fixture : public database_trx_integration_fixture
{
    database_betting_integration_fixture();

    virtual ~database_betting_integration_fixture() = default;

    void empower_moderator(const Actor& moderator);
    create_game_operation create_game(const Actor& moderator,
                                      fc::flat_set<betting::market_type> markets,
                                      uint32_t start_delay = start_delay_default,
                                      uint32_t auto_resolve_delay_sec = auto_resolve_delay_default);
    post_bet_operation create_bet(const Actor& better,
                                  const betting::wincase_type& wincase,
                                  const odds_input& odds_value,
                                  const asset& stake);
    cancel_pending_bets_operation cancel_pending_bet(const Actor& better, const fc::flat_set<int64_t>& bet_ids);
    cancel_game_operation cancel_game(const Actor& moderator);
    update_game_markets_operation update_markets(const Actor& moderator, fc::flat_set<betting::market_type> markets);
    update_game_start_time_operation update_start_time(const Actor& moderator, uint32_t start_delay);
    post_game_results_operation post_results(const Actor& moderator,
                                             const fc::flat_set<betting::wincase_type>& winners);

    proposal_id_type get_last_proposal_id();

    dynamic_global_property_service_i& dgp_service;
    betting_property_service_i& betting_property_service;
    account_service_i& account_service;
    game_service_i& game_service;
};
}
