#pragma once

#include <scorum/chain/schema/scorum_object_types.hpp>

namespace scorum {
namespace chain {

class chain_property_object;

class chain_property_object : public object<chain_property_object_type, chain_property_object>
{
public:
    template <typename Constructor, typename Allocator> chain_property_object(Constructor&& c, allocator<Allocator> a)
    {
        c(*this);
    }

    id_type id;
    chain_id_type chain_id;
};

typedef multi_index_container<chain_property_object,
                              indexed_by<ordered_unique<tag<by_id>,
                                                        member<chain_property_object,
                                                               chain_property_object::id_type,
                                                               &chain_property_object::id>>>,
                              allocator<chain_property_object>>
    chain_property_index;

} // namespace chain
} // namespace scorum

FC_REFLECT(scorum::chain::chain_property_object, (id)(chain_id))
CHAINBASE_SET_INDEX_TYPE(scorum::chain::chain_property_object, scorum::chain::chain_property_index)
