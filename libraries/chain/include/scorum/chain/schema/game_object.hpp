#pragma once
#include <fc/fixed_string.hpp>
#include <fc/shared_containers.hpp>
#include <scorum/protocol/betting/game.hpp>
#include <scorum/protocol/betting/market.hpp>
#include <scorum/protocol/betting/wincase.hpp>
#include <scorum/protocol/betting/wincase_comparison.hpp>
#include <scorum/chain/schema/scorum_object_types.hpp>

namespace scorum {
namespace chain {

using namespace scorum::protocol;

using scorum::protocol::betting::game_type;
using scorum::protocol::betting::market_type;
using scorum::protocol::betting::wincase_type;

enum class game_status : uint8_t
{
    created,
    started,
    finished
};

struct by_name;
struct by_start_time;

class game_object : public object<game_object_type, game_object>
{
public:
    /// @cond DO_NOT_DOCUMENT
    CHAINBASE_DEFAULT_DYNAMIC_CONSTRUCTOR(game_object, (name)(markets)(results))

    typedef typename object<game_object_type, game_object>::id_type id_type;

    id_type id;

    fc::shared_string name;
    time_point_sec start = time_point_sec::min();
    time_point_sec last_update = time_point_sec::min();
    time_point_sec bets_resolve_time = time_point_sec::maximum();

    game_status status = game_status::created;

    game_type game;
    fc::shared_flat_set<market_type> markets;
    fc::shared_flat_set<wincase_type> results;
};

using game_index
    = shared_multi_index_container<game_object,
                                   indexed_by<ordered_unique<tag<by_id>,
                                                             member<game_object,
                                                                    game_object::id_type,
                                                                    &game_object::id>>,
                                              ordered_unique<tag<by_name>,
                                                             member<game_object, fc::shared_string, &game_object::name>,
                                                             fc::strcmp_less>,
                                              ordered_non_unique<tag<by_start_time>,
                                                                 member<game_object,
                                                                        fc::time_point_sec,
                                                                        &game_object::start>>>>;
}
}

FC_REFLECT_ENUM(scorum::chain::game_status, (created)(started)(finished))
FC_REFLECT(scorum::chain::game_object,
           (id)(name)(start)(last_update)(bets_resolve_time)(status)(game)(markets)(results))

CHAINBASE_SET_INDEX_TYPE(scorum::chain::game_object, scorum::chain::game_index)
