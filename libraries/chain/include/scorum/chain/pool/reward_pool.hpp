#pragma once

#include <scorum/protocol/asset.hpp>
#include <scorum/chain/scorum_object_types.hpp>

namespace scorum {
namespace chain {

using scorum::protocol::asset;
using reward_pool_name_type = protocol::fixed_string_16;

class reward_pool_object : public object<reward_pool_object_type, reward_pool_object>
{
public:
    template <typename Constructor, typename Allocator> reward_pool_object(Constructor&& c, allocator<Allocator> a)
    {
        c(*this);
    }

public:
    reward_pool_id_type id;
    reward_pool_name_type name = reward_pool_name_type("reward_pool");

    asset balance;
    asset current_per_block_reward;
};

struct by_name;
typedef multi_index_container<
    reward_pool_object,
    indexed_by<
        ordered_unique<tag<by_id>, member<reward_pool_object, reward_pool_id_type, &reward_pool_object::id>>,
        ordered_unique<tag<by_name>, member<reward_pool_object, reward_pool_name_type, &reward_pool_object::name>>>,
    allocator<reward_pool_object>>
    reward_pool_index;

} // namespace chain
} // namespace scorum

// clang-format off
FC_REFLECT( scorum::chain::reward_pool_object,
            (id)
            (name)
            (balance)
            (current_per_block_reward)
           )

CHAINBASE_SET_INDEX_TYPE(scorum::chain::reward_pool_object, scorum::chain::reward_pool_index)

// clang-format on
