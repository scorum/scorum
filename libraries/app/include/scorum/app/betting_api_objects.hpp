#pragma once

#include <scorum/protocol/types.hpp>
#include <scorum/chain/schema/game_object.hpp>
#include <scorum/chain/schema/bet_objects.hpp>
#include <scorum/chain/schema/betting_property_object.hpp>

#include <scorum/app/schema/api_template.hpp>

#define API_QUERY_MAX_LIMIT (100)

namespace scorum {
namespace app {

enum class game_filter : uint8_t
{
    created = (uint8_t)chain::game_status::created,
    started = (uint8_t)chain::game_status::started,
    finished = (uint8_t)chain::game_status::finished,
    not_finished = created | started,
    not_started = created | finished,
    not_created = started | finished,
    all = created | started | finished
};

struct game_api_object
{
    game_api_object() = default;

    game_api_object(const chain::game_object& obj)
        : id(obj.id)
        , uuid(obj.uuid)
        , name(fc::to_string(obj.name))
        , start_time(obj.start_time)
        , last_update(obj.last_update)
        , bets_resolve_time(obj.bets_resolve_time)
        , status(obj.status)
        , game(obj.game)
    {
        std::copy(obj.markets.begin(), obj.markets.end(), markets.begin());
        std::copy(obj.results.begin(), obj.results.end(), results.begin());
    }

    chain::game_id_type id = 0;
    uuid_type uuid;

    std::string moderator;
    std::string name;

    fc::time_point_sec start_time = fc::time_point_sec::min();
    fc::time_point_sec last_update = fc::time_point_sec::min();
    fc::time_point_sec bets_resolve_time = time_point_sec::maximum();

    chain::game_status status = chain::game_status::created;
    chain::game_type game;

    fc::flat_set<chain::market_type> markets;
    fc::flat_set<chain::wincase_type> results;
};

struct winner_api_object
{
    winner_api_object() = default;
    winner_api_object(const chain::market_type& market, const chain::bet_data& winner, const chain::bet_data& loser)
        : winner({ winner.uuid, winner.better, winner.wincase })
        , loser({ loser.uuid, loser.better, loser.wincase })
        , market(market)
        , profit(loser.stake)
        , income(winner.stake + loser.stake)
    {
    }

    /**
     * @brief Brief information about better
     */
    struct better
    {
        uuid_type uuid;
        protocol::account_name_type name;
        chain::wincase_type wincase;
    };

    better winner;
    better loser;
    chain::market_type market;

    /**
     * @brief Winner's net win
     */
    protocol::asset profit;
    /**
     * @brief Winner's net win + winner's stake
     */
    protocol::asset income;
};

using matched_bet_api_object = api_obj<chain::matched_bet_object>;
using pending_bet_api_object = api_obj<chain::pending_bet_object>;
using betting_property_api_object = api_obj<chain::betting_property_object>;

} // namespace app
} // namespace scorum

// clang-format off
FC_REFLECT_ENUM(scorum::app::game_filter,
                (all)
                (created)
                (started)
                (finished)
                (not_created)
                (not_finished)
                (not_started))

FC_REFLECT(scorum::app::game_api_object,
           (id)
           (moderator)
           (name)
           (start_time)
           (last_update)
           (bets_resolve_time)
           (status)
           (game)
           (markets)
           (results))

FC_REFLECT(scorum::app::winner_api_object::better,
          (uuid)
          (name)
          (wincase))

FC_REFLECT(scorum::app::winner_api_object,
          (winner)
          (loser)
          (market)
          (profit)
          (income))
// clang-format on

FC_REFLECT_DERIVED(scorum::app::matched_bet_api_object, (scorum::chain::matched_bet_object), BOOST_PP_SEQ_NIL)
FC_REFLECT_DERIVED(scorum::app::pending_bet_api_object, (scorum::chain::pending_bet_object), BOOST_PP_SEQ_NIL)
FC_REFLECT_DERIVED(scorum::app::betting_property_api_object, (scorum::chain::betting_property_object), BOOST_PP_SEQ_NIL)
