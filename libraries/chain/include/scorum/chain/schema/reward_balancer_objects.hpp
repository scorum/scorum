#pragma once

#include <scorum/protocol/asset.hpp>
#include <scorum/chain/schema/scorum_object_types.hpp>

namespace scorum {
namespace chain {

using scorum::protocol::asset;
using scorum::protocol::asset_symbol_type;

template <uint16_t ObjectType, asset_symbol_type SymbolType>
class reward_balancer_object : public object<ObjectType, reward_balancer_object<ObjectType, SymbolType>>
{
public:
    CHAINBASE_DEFAULT_CONSTRUCTOR(reward_balancer_object)

    typedef typename object<ObjectType, reward_balancer_object<ObjectType, SymbolType>>::id_type id_type;

public:
    id_type id;

    asset balance = asset(0, SymbolType);
    asset current_per_block_reward = asset(SCORUM_MIN_PER_BLOCK_REWARD, SymbolType);
};

template <typename BalancerObjectType>
using reward_balancer_index
    = shared_multi_index_container<BalancerObjectType,
                                   indexed_by<ordered_unique<tag<by_id>,
                                                             member<BalancerObjectType,
                                                                    typename BalancerObjectType::id_type,
                                                                    &BalancerObjectType::id>>>>;

using content_reward_balancer_scr_object
    = reward_balancer_object<content_reward_balancer_scr_object_type, SCORUM_SYMBOL>;
using voters_reward_balancer_scr_object = reward_balancer_object<voters_reward_balancer_scr_object_type, SCORUM_SYMBOL>;
using voters_reward_balancer_sp_object = reward_balancer_object<voters_reward_balancer_sp_object_type, SP_SYMBOL>;

using content_reward_balancer_scr_index = reward_balancer_index<content_reward_balancer_scr_object>;
using voters_reward_balancer_scr_index = reward_balancer_index<voters_reward_balancer_scr_object>;
using voters_reward_balancer_sp_index = reward_balancer_index<voters_reward_balancer_sp_object>;

} // namespace chain
} // namespace scorum

FC_REFLECT(scorum::chain::content_reward_balancer_scr_object, (id)(balance)(current_per_block_reward))
FC_REFLECT(scorum::chain::voters_reward_balancer_scr_object, (id)(balance)(current_per_block_reward))
FC_REFLECT(scorum::chain::voters_reward_balancer_sp_object, (id)(balance)(current_per_block_reward))

CHAINBASE_SET_INDEX_TYPE(scorum::chain::content_reward_balancer_scr_object,
                         scorum::chain::content_reward_balancer_scr_index)
CHAINBASE_SET_INDEX_TYPE(scorum::chain::voters_reward_balancer_scr_object,
                         scorum::chain::voters_reward_balancer_scr_index)
CHAINBASE_SET_INDEX_TYPE(scorum::chain::voters_reward_balancer_sp_object,
                         scorum::chain::voters_reward_balancer_sp_index)
