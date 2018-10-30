#pragma once

#include <scorum/protocol/betting/market.hpp>
#include <scorum/chain/schema/scorum_object_types.hpp>
#include <scorum/protocol/odds.hpp>

#include <boost/multi_index/hashed_index.hpp>
#include <boost/multi_index/composite_key.hpp>

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

struct bet_data
{
    uuid_type uuid;
    fc::time_point_sec created;

    account_name_type better;
    wincase_type wincase;
    asset stake = asset(0, SCORUM_SYMBOL);

    class odds odds;
    pending_bet_kind kind = pending_bet_kind::live;
};

class bet_uuid_history_object : public object<bet_uuid_history_object_type, bet_uuid_history_object>
{
public:
    /// @cond DO_NOT_DOCUMENT
    CHAINBASE_DEFAULT_CONSTRUCTOR(bet_uuid_history_object)
    /// @endcond

    id_type id;
    uuid_type uuid;
};

class pending_bet_object : public object<pending_bet_object_type, pending_bet_object>
{
public:
    /// @cond DO_NOT_DOCUMENT
    CHAINBASE_DEFAULT_CONSTRUCTOR(pending_bet_object)
    /// @endcond

    id_type id;
    uuid_type game_uuid;
    market_type market;

    bet_data data;

    // clang-format off
    fc::time_point_sec get_created() const { return data.created; }
    account_name_type get_better() const { return data.better; }
    pending_bet_kind get_kind() const { return data.kind; }
    uuid_type get_uuid() const { return data.uuid; }
    wincase_type get_wincase() const { return data.wincase; }

    // clang-format on
};

class matched_bet_object : public object<matched_bet_object_type, matched_bet_object>
{
public:
    /// @cond DO_NOT_DOCUMENT
    CHAINBASE_DEFAULT_CONSTRUCTOR(matched_bet_object)
    /// @endcond

    id_type id;
    uuid_type game_uuid;
    market_type market;
    fc::time_point_sec created;

    bet_data bet1_data;
    bet_data bet2_data;

    // clang-format off
    uuid_type get_bet1_uuid() const { return bet1_data.uuid; }
    uuid_type get_bet2_uuid() const { return bet2_data.uuid; }
    // clang-format on
};

struct by_uuid;
struct by_game_uuid_kind;
struct by_game_uuid_market;
struct by_game_uuid_better;
struct by_game_uuid_created;
struct by_game_uuid_wincase;

typedef shared_multi_index_container<bet_uuid_history_object,
                                     indexed_by<ordered_unique<tag<by_id>,
                                                               member<bet_uuid_history_object,
                                                                      bet_uuid_history_id_type,
                                                                      &bet_uuid_history_object::id>>,
                                                hashed_unique<tag<by_uuid>,
                                                              member<bet_uuid_history_object,
                                                                     uuid_type,
                                                                     &bet_uuid_history_object::uuid>>>>
    bet_uuid_history_index;

typedef shared_multi_index_container<pending_bet_object,
                                     indexed_by<ordered_unique<tag<by_id>,
                                                               member<pending_bet_object,
                                                                      pending_bet_id_type,
                                                                      &pending_bet_object::id>>,
                                                hashed_unique<tag<by_uuid>,
                                                              const_mem_fun<pending_bet_object,
                                                                            uuid_type,
                                                                            &pending_bet_object::get_uuid>>,

                                                ordered_non_unique<tag<by_game_uuid_wincase>,
                                                                   composite_key<pending_bet_object,
                                                                                 member<pending_bet_object,
                                                                                        uuid_type,
                                                                                        &pending_bet_object::game_uuid>,
                                                                                 const_mem_fun<pending_bet_object,
                                                                                               wincase_type,
                                                                                               &pending_bet_object::
                                                                                                   get_wincase>>>,

                                                ordered_non_unique<tag<by_game_uuid_kind>,
                                                                   composite_key<pending_bet_object,
                                                                                 member<pending_bet_object,
                                                                                        uuid_type,
                                                                                        &pending_bet_object::game_uuid>,
                                                                                 const_mem_fun<pending_bet_object,
                                                                                               pending_bet_kind,
                                                                                               &pending_bet_object::
                                                                                                   get_kind>>>,
                                                ordered_non_unique<tag<by_game_uuid_market>,
                                                                   composite_key<pending_bet_object,
                                                                                 member<pending_bet_object,
                                                                                        uuid_type,
                                                                                        &pending_bet_object::game_uuid>,
                                                                                 member<pending_bet_object,
                                                                                        market_type,
                                                                                        &pending_bet_object::market>>>,
                                                ordered_non_unique<tag<by_game_uuid_better>,
                                                                   composite_key<pending_bet_object,
                                                                                 member<pending_bet_object,
                                                                                        uuid_type,
                                                                                        &pending_bet_object::game_uuid>,
                                                                                 const_mem_fun<pending_bet_object,
                                                                                               account_name_type,
                                                                                               &pending_bet_object::
                                                                                                   get_better>>>,
                                                ordered_non_unique<tag<by_game_uuid_created>,
                                                                   composite_key<pending_bet_object,
                                                                                 member<pending_bet_object,
                                                                                        uuid_type,
                                                                                        &pending_bet_object::game_uuid>,
                                                                                 const_mem_fun<pending_bet_object,
                                                                                               fc::time_point_sec,
                                                                                               &pending_bet_object::
                                                                                                   get_created>>>>>
    pending_bet_index;

struct by_bet1_uuid;
struct by_bet2_uuid;

typedef shared_multi_index_container<matched_bet_object,
                                     indexed_by<ordered_unique<tag<by_id>,
                                                               member<matched_bet_object,
                                                                      matched_bet_id_type,
                                                                      &matched_bet_object::id>>,
                                                ordered_non_unique<tag<by_bet1_uuid>,
                                                                   const_mem_fun<matched_bet_object,
                                                                                 uuid_type,
                                                                                 &matched_bet_object::get_bet1_uuid>>,
                                                ordered_non_unique<tag<by_bet2_uuid>,
                                                                   const_mem_fun<matched_bet_object,
                                                                                 uuid_type,
                                                                                 &matched_bet_object::get_bet2_uuid>>,
                                                ordered_non_unique<tag<by_game_uuid_market>,
                                                                   composite_key<matched_bet_object,
                                                                                 member<matched_bet_object,
                                                                                        uuid_type,
                                                                                        &matched_bet_object::game_uuid>,
                                                                                 member<matched_bet_object,
                                                                                        market_type,
                                                                                        &matched_bet_object::market>>>,
                                                ordered_non_unique<tag<by_game_uuid_created>,
                                                                   composite_key<matched_bet_object,
                                                                                 member<matched_bet_object,
                                                                                        uuid_type,
                                                                                        &matched_bet_object::game_uuid>,
                                                                                 member<matched_bet_object,
                                                                                        fc::time_point_sec,
                                                                                        &matched_bet_object::
                                                                                            created>>>>>
    matched_bet_index;
}
}

// clang-format off

FC_REFLECT(scorum::chain::bet_uuid_history_object,
           (id)
           (uuid)
           )

CHAINBASE_SET_INDEX_TYPE(scorum::chain::bet_uuid_history_object, scorum::chain::bet_uuid_history_index)

FC_REFLECT_ENUM(scorum::chain::pending_bet_kind,
                (live)
                (non_live))

FC_REFLECT(scorum::chain::bet_data,
           (uuid)
           (created)
           (better)
           (wincase)
           (stake)
           (odds)
           (kind))

FC_REFLECT(scorum::chain::pending_bet_object,
           (id)
           (game_uuid)
           (market)
           (data)
           )

CHAINBASE_SET_INDEX_TYPE(scorum::chain::pending_bet_object, scorum::chain::pending_bet_index)

FC_REFLECT(scorum::chain::matched_bet_object,
           (id)
           (game_uuid)
           (market)
           (created)
           (bet1_data)
           (bet2_data)
           )

CHAINBASE_SET_INDEX_TYPE(scorum::chain::matched_bet_object, scorum::chain::matched_bet_index)
// clang-format on
