#pragma once

#include <scorum/protocol/operations.hpp>
#include <scorum/protocol/betting/wincase.hpp>

#include <boost/uuid/uuid_generators.hpp>

#include "database_trx_integration.hpp"
#include <functional>

namespace database_fixture {

const uint32_t start_delay_default = SCORUM_BLOCK_INTERVAL;
const uint32_t auto_resolve_delay_default = DAYS_TO_SECONDS(1);

struct database_betting_integration_fixture : public database_trx_integration_fixture
{
    database_betting_integration_fixture() = default;

    virtual ~database_betting_integration_fixture() = default;

    void empower_moderator(const Actor& moderator);

    create_game_operation create_game(const scorum::uuid_type& uuid,
                                      const Actor& moderator,
                                      std::vector<market_type> markets,
                                      uint32_t start_delay = start_delay_default,
                                      uint32_t auto_resolve_delay_sec = auto_resolve_delay_default);

    create_game_operation create_game(const Actor& moderator,
                                      std::vector<market_type> markets,
                                      uint32_t start_delay = start_delay_default,
                                      uint32_t auto_resolve_delay_sec = auto_resolve_delay_default);

    post_bet_operation create_bet(const scorum::uuid_type& uuid,
                                  const Actor& better,
                                  const wincase_type& wincase,
                                  const odds_input& odds_value,
                                  const asset& stake,
                                  bool is_live = true);

    cancel_pending_bets_operation cancel_pending_bet(const Actor& better,
                                                     const std::vector<scorum::uuid_type>& bet_uuids);

    cancel_game_operation cancel_game(const Actor& moderator);
    update_game_markets_operation update_markets(const Actor& moderator, std::vector<market_type> markets);
    update_game_start_time_operation update_start_time(const Actor& moderator, uint32_t start_delay);
    post_game_results_operation post_results(const Actor& moderator, const std::vector<wincase_type>& winners);

    proposal_id_type get_last_proposal_id();

    fc::time_point_sec head_block_time() const;

    account_name_type betting_moderator() const;
};
}
