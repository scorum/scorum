#pragma once

#include <scorum/chain/schema/scorum_object_types.hpp>

#include <scorum/protocol/odds.hpp>

#include <boost/multi_index/composite_key.hpp>

#include <scorum/protocol/betting/market.hpp>

namespace scorum {
namespace chain {

using scorum::protocol::asset;
using scorum::protocol::odds;
using scorum::protocol::wincase_type;
using scorum::protocol::market_type;

enum class pending_bet_kind : uint8_t
{
    live = 0b01,
    non_live = 0b10
};

class pending_bet_object : public object<pending_bet_object_type, pending_bet_object>
{
public:
    /// @cond DO_NOT_DOCUMENT
    CHAINBASE_DEFAULT_CONSTRUCTOR(pending_bet_object)
    /// @endcond

    id_type id;
    game_id_type game;
    market_type market;
    fc::time_point_sec created;

    account_name_type better;
    wincase_type wincase;
    asset stake = asset(0, SCORUM_SYMBOL);

    odds odds_value;
    pending_bet_kind kind = pending_bet_kind::live;
};

class matched_bet_object : public object<matched_bet_object_type, matched_bet_object>
{
public:
    /// @cond DO_NOT_DOCUMENT
    CHAINBASE_DEFAULT_CONSTRUCTOR(matched_bet_object)
    /// @endcond

    id_type id;
    game_id_type game;
    market_type market;
    fc::time_point_sec created;

    account_name_type better1;
    wincase_type wincase1;
    asset stake1 = asset(0, SCORUM_SYMBOL);

    account_name_type better2;
    wincase_type wincase2;
    asset stake2 = asset(0, SCORUM_SYMBOL);
};

struct by_game_id_kind;
struct by_game_id_market;

typedef shared_multi_index_container<pending_bet_object,
                                     indexed_by<ordered_unique<tag<by_id>,
                                                               member<pending_bet_object,
                                                                      pending_bet_id_type,
                                                                      &pending_bet_object::id>>,
                                                ordered_non_unique<tag<by_game_id_kind>,
                                                                   composite_key<pending_bet_object,
                                                                                 member<pending_bet_object,
                                                                                        game_id_type,
                                                                                        &pending_bet_object::game>,
                                                                                 member<pending_bet_object,
                                                                                        pending_bet_kind,
                                                                                        &pending_bet_object::kind>>>,
                                                ordered_non_unique<tag<by_game_id_market>,
                                                                   composite_key<pending_bet_object,
                                                                                 member<pending_bet_object,
                                                                                        game_id_type,
                                                                                        &pending_bet_object::game>,
                                                                                 member<pending_bet_object,
                                                                                        market_type,
                                                                                        &pending_bet_object::market>>>>>
    pending_bet_index;

typedef shared_multi_index_container<matched_bet_object,
                                     indexed_by<ordered_unique<tag<by_id>,
                                                               member<matched_bet_object,
                                                                      matched_bet_id_type,
                                                                      &matched_bet_object::id>>,
                                                ordered_non_unique<tag<by_game_id_market>,
                                                                   composite_key<matched_bet_object,
                                                                                 member<matched_bet_object,
                                                                                        game_id_type,
                                                                                        &matched_bet_object::game>,
                                                                                 member<matched_bet_object,
                                                                                        market_type,
                                                                                        &matched_bet_object::market>>>>>
    matched_bet_index;
}
}

// clang-format off
FC_REFLECT_ENUM(scorum::chain::pending_bet_kind,
           (live)
           (non_live))

FC_REFLECT(scorum::chain::pending_bet_object,
           (id)
           (game)
           (market)
           (created)
           (better)
           (wincase)
           (stake)
           (odds_value)
           (kind))

CHAINBASE_SET_INDEX_TYPE(scorum::chain::pending_bet_object, scorum::chain::pending_bet_index)

FC_REFLECT(scorum::chain::matched_bet_object,
           (id)
           (game)
           (market)
           (created)
           (better1)
           (better2)
           (wincase1)
           (wincase2)
           (stake1)
           (stake2))

CHAINBASE_SET_INDEX_TYPE(scorum::chain::matched_bet_object, scorum::chain::matched_bet_index)
// clang-format on
