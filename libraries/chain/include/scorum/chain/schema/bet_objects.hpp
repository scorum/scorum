#pragma once

#include <scorum/chain/schema/scorum_object_types.hpp>

#include <scorum/protocol/odds.hpp>

namespace scorum {
namespace chain {

using scorum::protocol::asset;
using scorum::protocol::odds;

using game_id_type = int16_t;

struct wincase1
{
};
struct wincase2
{
};
struct wincase3
{
};

using wincase_type = fc::static_variant<wincase1, wincase2, wincase3>;

class bet_object : public object<bet_object_type, bet_object>
{
public:
    /// \cond DO_NOT_DOCUMENT
    CHAINBASE_DEFAULT_CONSTRUCTOR(bet_object)

    id_type id;

    fc::time_point_sec created;

    account_name_type better;

    odds value;

    asset stake = asset(0, SCORUM_SYMBOL);

    asset matched_stake = asset(0, SCORUM_SYMBOL);

    asset potential_return = asset(0, SCORUM_SYMBOL);

    game_id_type game;

    wincase_type wincase;
};

class pending_bet_object : public object<pending_bet_object_type, pending_bet_object>
{
public:
    /// \cond DO_NOT_DOCUMENT
    CHAINBASE_DEFAULT_CONSTRUCTOR(pending_bet_object)

    id_type id;

    bet_id_type bet;

    asset stake = asset(0, SCORUM_SYMBOL);
};

class matched_bet_object : public object<matched_bet_object_type, matched_bet_object>
{
public:
    /// \cond DO_NOT_DOCUMENT
    CHAINBASE_DEFAULT_CONSTRUCTOR(matched_bet_object)

    id_type id;

    fc::time_point_sec matched;

    bet_id_type bet1;

    bet_id_type bet2;

    asset stake = asset(0, SCORUM_SYMBOL);
};

typedef shared_multi_index_container<bet_object,
                                     indexed_by<ordered_unique<tag<by_id>,
                                                               member<bet_object, bet_id_type, &bet_object::id>>>>
    bet_index;

typedef shared_multi_index_container<pending_bet_object,
                                     indexed_by<ordered_unique<tag<by_id>,
                                                               member<pending_bet_object,
                                                                      pending_bet_id_type,
                                                                      &pending_bet_object::id>>>>
    pending_bet_index;

typedef shared_multi_index_container<matched_bet_object,
                                     indexed_by<ordered_unique<tag<by_id>,
                                                               member<matched_bet_object,
                                                                      matched_bet_id_type,
                                                                      &matched_bet_object::id>>>>
    matched_bet_index;
}
}

// clang-format off
FC_REFLECT(scorum::chain::bet_object,
           (id)
           (created)
           (better)
           (value)
           (stake)
           (matched_stake)
           (potential_return)
           (game)
           (wincase))

CHAINBASE_SET_INDEX_TYPE(scorum::chain::bet_object, scorum::chain::bet_index)

FC_REFLECT(scorum::chain::pending_bet_object,
           (id)
           (stake)
           (bet))

CHAINBASE_SET_INDEX_TYPE(scorum::chain::pending_bet_object, scorum::chain::pending_bet_index)

FC_REFLECT(scorum::chain::matched_bet_object,
           (id)
           (matched)
           (stake)
           (bet1)
           (bet2))

CHAINBASE_SET_INDEX_TYPE(scorum::chain::matched_bet_object, scorum::chain::matched_bet_index)
// clang-format on
