#pragma once

#include <scorum/protocol/asset.hpp>
#include <scorum/chain/schema/scorum_object_types.hpp>

namespace scorum {
namespace chain {

using scorum::protocol::asset;

class reward_balancer_object : public object<reward_balancer_object_type, reward_balancer_object>
{
public:
    CHAINBASE_DEFAULT_CONSTRUCTOR(reward_balancer_object)

public:
    reward_pool_id_type id;

    asset balance = asset(0, SCORUM_SYMBOL);
    asset current_per_block_reward = SCORUM_MIN_PER_BLOCK_REWARD;
};

// clang-format off
using reward_pool_index = shared_multi_index_container<reward_balancer_object,
                                                indexed_by<ordered_unique<tag<by_id>, 
                                                                          member<reward_balancer_object, 
                                                                                 reward_pool_id_type, 
                                                                                 &reward_balancer_object::id>>>>;

} // namespace chain
} // namespace scorum


FC_REFLECT( scorum::chain::reward_balancer_object,
            (id)
            (balance)
            (current_per_block_reward)
           )

CHAINBASE_SET_INDEX_TYPE(scorum::chain::reward_balancer_object, scorum::chain::reward_pool_index)

// clang-format on
