#pragma once

#define API_BETTING "betting_api"

#include <fc/api.hpp>

#include <scorum/app/betting_api_objects.hpp>

namespace chainbase {
class database_guard;
}

namespace scorum {
namespace app {

struct api_context;

/**
 * @brief Betting API
 *
 * This api does not require plugins
 *
 * @ingroup api
 * @addtogroup betting_api Betting API
 */
class betting_api
{
public:
    betting_api(const api_context& ctx);

    ~betting_api();

    void on_api_startup();

    /// @name Public API
    /// @addtogroup betting_api
    /// @{
    ///

    /**
    * @brief Returns bets with draw status
    * @param game_uuid Game UUID
    * @return array of matched_bet_object's
    */
    std::vector<matched_bet_api_object> get_game_returns(const uuid_type& game_uuid) const;

    /**
     * @brief Returns all winners for particular game
     * @param game_uuid Game UUID
     * @return array of winner_api_object's
     */
    std::vector<winner_api_object> get_game_winners(const uuid_type& game_uuid) const;

    /**
     * @brief Returns games
     * @param filter [created, started, finished, resolved, expired, cancelled]
     * @return array of game_api_object's
     */
    std::vector<game_api_object> get_games_by_status(const fc::flat_set<chain::game_status>& filter) const;

    /**
     * @brief Returns games
     * @param UUIDs of games to return
     * @return array of game_api_object's
     */
    std::vector<game_api_object> get_games_by_uuids(const std::vector<uuid_type>& uuids) const;

    /**
     * @brief Returns games
     * @param from lower bound game id
     * @param limit query limit
     * @return array of game_api_object's
     */
    std::vector<game_api_object> lookup_games_by_id(chain::game_id_type from, uint32_t limit) const;

    /**
     * @brief Returns matched bets
     * @param from lower bound bet id
     * @param limit query limit
     * @return array of matched_bet_api_object's
     */
    std::vector<matched_bet_api_object> lookup_matched_bets(chain::matched_bet_id_type from, uint32_t limit) const;

    /**
     * @brief Return pending bets
     * @param from lower bound bet id
     * @param limit query limit
     * @return array of pending_bet_api_object's
     */
    std::vector<pending_bet_api_object> lookup_pending_bets(chain::pending_bet_id_type from, uint32_t limit) const;

    /**
     * @brief Returns matched bets
     * @param uuids  The vector of UUIDs of pending bets which form matched bets
     * @return array of matched_bet_api_object's
     */
    std::vector<matched_bet_api_object> get_matched_bets(const std::vector<uuid_type>& uuids) const;

    /**
     * @brief Return pending bets
     * @param uuids The vector of UUIDs of pending bets which should be returned
     * @return array of pending_bet_api_object's
     */
    std::vector<pending_bet_api_object> get_pending_bets(const std::vector<uuid_type>& uuids) const;

    /**
     * @brief Returns matched bets for game
     * @param uuid Game uuid
     * @return array of matched_bet_api_object's
     */
    std::vector<matched_bet_api_object> get_game_matched_bets(const uuid_type& uuid) const;

    /**
     * @brief Return pending bets for game
     * @param uuid Game uuid
     * @return array of pending_bet_api_object's
     */
    std::vector<pending_bet_api_object> get_game_pending_bets(const uuid_type& uuid) const;

    /**
     * @brief Return betting properties
     * @return betting propery api object
     */
    betting_property_api_object get_betting_properties() const;

    /// @}

    class impl;

private:
    std::unique_ptr<impl> _impl;

    std::shared_ptr<chainbase::database_guard> _guard;
};

} // namespace app
} // namespace scorum

// clang-format off
FC_API(scorum::app::betting_api, (get_game_returns)
                                 (get_game_winners)
                                 (get_games_by_status)
                                 (get_games_by_uuids)
                                 (lookup_games_by_id)
                                 (lookup_matched_bets)
                                 (lookup_pending_bets)
                                 (get_matched_bets)
                                 (get_pending_bets)
                                 (get_game_matched_bets)
                                 (get_game_pending_bets)
                                 (get_betting_properties))
// clang-format on
