#pragma once
#include <scorum/chain/schema/scorum_object_types.hpp>

namespace scorum {
namespace chain {

using scorum::protocol::block_id_type;

/**
 *  @brief tracks minimal information about past blocks to implement TaPOS
 *  @ingroup object
 *
 *  When attempting to calculate the validity of a transaction we need to
 *  lookup a past block and check its block hash and the time it occurred
 *  so we can calculate whether the current transaction is valid and at
 *  what time it should expire.
 */
class block_summary_object : public object<block_summary_object_type, block_summary_object>
{
public:
    CHAINBASE_DEFAULT_CONSTRUCTOR(block_summary_object)

    id_type id;
    block_id_type block_id;
};

typedef multi_index_container<block_summary_object,
                              indexed_by<ordered_unique<tag<by_id>,
                                                        member<block_summary_object,
                                                               block_summary_object::id_type,
                                                               &block_summary_object::id>>>,
                              allocator<block_summary_object>>
    block_summary_index;
}
} // scorum::chain

FC_REFLECT(scorum::chain::block_summary_object, (id)(block_id))
CHAINBASE_SET_INDEX_TYPE(scorum::chain::block_summary_object, scorum::chain::block_summary_index)
