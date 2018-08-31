#pragma once

#include <scorum/chain/schema/scorum_object_types.hpp>

#include <scorum/protocol/odds.hpp>

#include <boost/multi_index/composite_key.hpp>

#include <scorum/protocol/betting/wincase.hpp>

namespace scorum {
namespace chain {

using scorum::protocol::asset;
using scorum::protocol::odds;
using scorum::protocol::betting::wincase_type;

class bet_object : public object<bet_object_type, bet_object>
{
public:
    /// @cond DO_NOT_DOCUMENT
    CHAINBASE_DEFAULT_CONSTRUCTOR(bet_object)
    /// @endcond

    id_type id;

    fc::time_point_sec created;

    account_name_type better;

    game_id_type game;

    wincase_type wincase;

    odds odds_value;

    asset stake = asset(0, SCORUM_SYMBOL);

    /// Not matched part of stake
    asset rest_stake = asset(0, SCORUM_SYMBOL);
};

class pending_bet_object : public object<pending_bet_object_type, pending_bet_object>
{
public:
    /// @cond DO_NOT_DOCUMENT
    CHAINBASE_DEFAULT_CONSTRUCTOR(pending_bet_object)
    /// @endcond

    id_type id;

    game_id_type game;

    bet_id_type bet;
};

class matched_bet_object : public object<matched_bet_object_type, matched_bet_object>
{
public:
    /// @cond DO_NOT_DOCUMENT
    CHAINBASE_DEFAULT_CONSTRUCTOR(matched_bet_object)
    /// @endcond

    id_type id;

    fc::time_point_sec when_matched;

    bet_id_type bet1;

    bet_id_type bet2;

    /// what part of bet1 stake is matched (subtracted from bet.rest_stake)
    asset matched_bet1_stake = asset(0, SCORUM_SYMBOL);

    /// what part of bet2 stake is matched (subtracted from bet.rest_stake)
    asset matched_bet2_stake = asset(0, SCORUM_SYMBOL);
};

struct by_game_id;

typedef shared_multi_index_container<bet_object,
                                     indexed_by<ordered_unique<tag<by_id>,
                                                               member<bet_object, bet_id_type, &bet_object::id>>>>
    bet_index;

struct by_bet_id;

typedef shared_multi_index_container<pending_bet_object,
                                     indexed_by<ordered_unique<tag<by_id>,
                                                               member<pending_bet_object,
                                                                      pending_bet_id_type,
                                                                      &pending_bet_object::id>>,
                                                ordered_unique<tag<by_bet_id>,
                                                               member<pending_bet_object,
                                                                      bet_id_type,
                                                                      &pending_bet_object::bet>>,
                                                ordered_unique<tag<by_game_id>,
                                                               composite_key<pending_bet_object,
                                                                             member<pending_bet_object,
                                                                                    game_id_type,
                                                                                    &pending_bet_object::game>,
                                                                             member<pending_bet_object,
                                                                                    bet_id_type,
                                                                                    &pending_bet_object::bet>>,
                                                               composite_key_compare<std::less<game_id_type>,
                                                                                     std::less<bet_id_type>>>>>
    pending_bet_index;

struct by_matched_bets_id;
struct by_matched_bet1_id;
struct by_matched_bet2_id;

typedef shared_multi_index_container<matched_bet_object,
                                     indexed_by<ordered_unique<tag<by_id>,
                                                               member<matched_bet_object,
                                                                      matched_bet_id_type,
                                                                      &matched_bet_object::id>>,
                                                ordered_unique<tag<by_matched_bets_id>,
                                                               composite_key<matched_bet_object,
                                                                             member<matched_bet_object,
                                                                                    bet_id_type,
                                                                                    &matched_bet_object::bet1>,
                                                                             member<matched_bet_object,
                                                                                    bet_id_type,
                                                                                    &matched_bet_object::bet2>>,
                                                               composite_key_compare<std::less<bet_id_type>,
                                                                                     std::less<bet_id_type>>>,
                                                ordered_non_unique<tag<by_matched_bet1_id>,
                                                                   member<matched_bet_object,
                                                                          bet_id_type,
                                                                          &matched_bet_object::bet1>>,
                                                ordered_non_unique<tag<by_matched_bet2_id>,
                                                                   member<matched_bet_object,
                                                                          bet_id_type,
                                                                          &matched_bet_object::bet2>>>>
    matched_bet_index;
}
}

// clang-format off
FC_REFLECT(scorum::chain::bet_object,
           (id)
           (created)
           (better)
           (game)
           (wincase)
           (odds_value)
           (stake)
           (rest_stake))

CHAINBASE_SET_INDEX_TYPE(scorum::chain::bet_object, scorum::chain::bet_index)

FC_REFLECT(scorum::chain::pending_bet_object,
           (id)
           (game)
           (bet))

CHAINBASE_SET_INDEX_TYPE(scorum::chain::pending_bet_object, scorum::chain::pending_bet_index)

FC_REFLECT(scorum::chain::matched_bet_object,
           (id)
           (when_matched)
           (bet1)
           (bet2)
           (matched_bet1_stake)
           (matched_bet2_stake))

CHAINBASE_SET_INDEX_TYPE(scorum::chain::matched_bet_object, scorum::chain::matched_bet_index)
// clang-format on
