#pragma once

#include <scorum/chain/schema/scorum_object_types.hpp>

#include <scorum/protocol/odds.hpp>

#include <boost/multi_index/composite_key.hpp>

namespace scorum {
namespace chain {

using scorum::protocol::asset;
using scorum::protocol::odds;

#if 1
using game_id_type = int16_t;

struct wincase11;
struct wincase12;
struct wincase21;
struct wincase22;

struct wincase11
{
    using opposite_type = wincase12;
};
struct wincase12
{
    using opposite_type = wincase11;
};
struct wincase21
{
    using opposite_type = wincase22;
};
struct wincase22
{
    using opposite_type = wincase21;
};

using wincase_type = fc::static_variant<wincase11, wincase12, wincase21, wincase22>;

inline bool match_wincases(const wincase_type&, const wincase_type&)
{
    // stub
    return true;
}
#endif

class bet_object : public object<bet_object_type, bet_object>
{
public:
    /// \cond DO_NOT_DOCUMENT
    CHAINBASE_DEFAULT_CONSTRUCTOR(bet_object)

    id_type id;

    fc::time_point_sec created;

    account_name_type better;

    game_id_type game;

    wincase_type wincase;

    odds value;

    asset stake = asset(0, SCORUM_SYMBOL);

    asset rest_stake = asset(0, SCORUM_SYMBOL); // not participated (not matched) stake

    asset potential_gain
        = asset(0, SCORUM_SYMBOL); // can calculated each time when we need but saved to improve productivity

    asset gain = asset(0, SCORUM_SYMBOL); // actual gain to control matched stake calculation accuracy lag
};

class pending_bet_object : public object<pending_bet_object_type, pending_bet_object>
{
public:
    /// \cond DO_NOT_DOCUMENT
    CHAINBASE_DEFAULT_CONSTRUCTOR(pending_bet_object)

    id_type id;

    game_id_type game;

    bet_id_type bet;
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
};

// struct by_better; --TODO
struct by_game_id;

typedef shared_multi_index_container<bet_object,
                                     indexed_by<ordered_unique<tag<by_id>,
                                                               member<bet_object, bet_id_type, &bet_object::id>>>>
    bet_index;

typedef shared_multi_index_container<pending_bet_object,
                                     indexed_by<ordered_unique<tag<by_id>,
                                                               member<pending_bet_object,
                                                                      pending_bet_id_type,
                                                                      &pending_bet_object::id>>,
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
                                                                                     std::less<bet_id_type>>>>>
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
           (value)
           (stake)
           (rest_stake)
           (potential_gain)
           (gain))

CHAINBASE_SET_INDEX_TYPE(scorum::chain::bet_object, scorum::chain::bet_index)

FC_REFLECT(scorum::chain::pending_bet_object,
           (id)
           (game)
           (bet))

CHAINBASE_SET_INDEX_TYPE(scorum::chain::pending_bet_object, scorum::chain::pending_bet_index)

FC_REFLECT(scorum::chain::matched_bet_object,
           (id)
           (matched)
           (bet1)
           (bet2))

CHAINBASE_SET_INDEX_TYPE(scorum::chain::matched_bet_object, scorum::chain::matched_bet_index)
// clang-format on
