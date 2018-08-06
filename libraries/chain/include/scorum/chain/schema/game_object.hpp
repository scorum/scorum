#pragma once
#include <fc/fixed_string.hpp>
#include <fc/shared_containers.hpp>
#include <scorum/protocol/betting/game.hpp>
#include <scorum/protocol/betting/market.hpp>
#include <scorum/chain/schema/scorum_object_types.hpp>

namespace scorum {
namespace chain {

using namespace scorum::protocol;

enum class game_status : uint8_t
{
    created,
    started,
    finished
};

struct by_name;

class game_object : public object<game_object_type, game_object>
{
public:
    /// @cond DO_NOT_DOCUMENT
    CHAINBASE_DEFAULT_DYNAMIC_CONSTRUCTOR(game_object, (name)(markets))

    typedef typename object<game_object_type, game_object>::id_type id_type;

    id_type id;

    account_name_type moderator;
    fc::shared_string name;
    time_point_sec start = time_point_sec::min();

    game_status status = game_status::created;

    betting::game_type game;
    fc::shared_flat_set<betting::market_type> markets;
};

using game_index
    = shared_multi_index_container<game_object,
                                   indexed_by<ordered_unique<tag<by_id>,
                                                             member<game_object,
                                                                    game_object::id_type,
                                                                    &game_object::id>>,
                                              ordered_unique<tag<by_name>,
                                                             member<game_object, fc::shared_string, &game_object::name>,
                                                             fc::strcmp_less>>>;
}
}

FC_REFLECT_ENUM(scorum::chain::game_status, (created)(started)(finished))
FC_REFLECT(scorum::chain::game_object, (id)(moderator)(name)(start)(status)(game)(markets))

CHAINBASE_SET_INDEX_TYPE(scorum::chain::game_object, scorum::chain::game_index)
