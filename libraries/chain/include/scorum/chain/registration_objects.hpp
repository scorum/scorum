#pragma once

#include <fc/fixed_string.hpp>

#include <scorum/protocol/asset.hpp>

#include <scorum/chain/scorum_object_types.hpp>

#include <boost/multi_index/composite_key.hpp>

namespace scorum {
namespace chain {

using scorum::protocol::asset;

class registration_pool_object : public object<registration_pool_object_type, registration_pool_object>
{
    registration_pool_object() = delete;

public:
    template <typename Constructor, typename Allocator> registration_pool_object(Constructor&& c, allocator<Allocator>)
    {
        c(*this);
    }

    id_type id;

    asset vesting_balance = asset(0, VESTS_SYMBOL);

    // TODO
};

// TODO

typedef multi_index_container<registration_pool_object,
                              indexed_by<ordered_unique<tag<by_id>,
                                                        member<registration_pool_object,
                                                               registration_pool_id_type,
                                                               &registration_pool_object::id>>>,
                              allocator<registration_pool_object>>
    registration_pool_index;
}
}

FC_REFLECT(scorum::chain::registration_pool_object, (id)(vesting_balance))

CHAINBASE_SET_INDEX_TYPE(scorum::chain::registration_pool_object, scorum::chain::registration_pool_index)
