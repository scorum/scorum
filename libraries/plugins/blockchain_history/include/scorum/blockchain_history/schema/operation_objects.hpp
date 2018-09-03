#pragma once

#include <scorum/blockchain_history/schema/blockchain_objects.hpp>

#include <fc/shared_buffer.hpp>

#include <scorum/protocol/authority.hpp>
#include <scorum/protocol/operations.hpp>
#include <scorum/protocol/scorum_operations.hpp>

#include <scorum/chain/schema/scorum_object_types.hpp>
#include <scorum/chain/schema/witness_objects.hpp>

#include <boost/multi_index/composite_key.hpp>

namespace scorum {
namespace blockchain_history {

using scorum::protocol::transaction_id_type;
using scorum::protocol::operation;

class operation_object : public object<operations_history_object_type, operation_object>
{
public:
    CHAINBASE_DEFAULT_DYNAMIC_CONSTRUCTOR(operation_object, (serialized_op))

    typedef typename object<operations_history_object_type, operation_object>::id_type id_type;

    id_type id;

    transaction_id_type trx_id;
    uint32_t block = 0;
    uint32_t trx_in_block = 0;
    uint16_t op_in_trx = 0;
    fc::time_point_sec timestamp;
    fc::shared_buffer serialized_op;
};

struct by_location;
struct by_timestamp;
struct by_transaction_id;
typedef shared_multi_index_container<operation_object,
                                     indexed_by<ordered_unique<tag<by_id>,
                                                               member<operation_object,
                                                                      operation_object::id_type,
                                                                      &operation_object::id>>,
                                                ordered_unique<tag<by_location>,
                                                               composite_key<operation_object,
                                                                             member<operation_object,
                                                                                    uint32_t,
                                                                                    &operation_object::block>,
                                                                             member<operation_object,
                                                                                    uint32_t,
                                                                                    &operation_object::trx_in_block>,
                                                                             member<operation_object,
                                                                                    uint16_t,
                                                                                    &operation_object::op_in_trx>,
                                                                             member<operation_object,
                                                                                    operation_object::id_type,
                                                                                    &operation_object::id>>>,
                                                ordered_unique<tag<by_timestamp>,
                                                               composite_key<operation_object,
                                                                             member<operation_object,
                                                                                    fc::time_point_sec,
                                                                                    &operation_object::timestamp>,
                                                                             member<operation_object,
                                                                                    operation_object::id_type,
                                                                                    &operation_object::id>>>
#ifndef SKIP_BY_TX_ID
                                                ,
                                                ordered_unique<tag<by_transaction_id>,
                                                               composite_key<operation_object,
                                                                             member<operation_object,
                                                                                    transaction_id_type,
                                                                                    &operation_object::trx_id>,
                                                                             member<operation_object,
                                                                                    operation_object::id_type,
                                                                                    &operation_object::id>>>
#endif
                                                >>
    operation_index;

template <blockchain_history_object_type OperationType>
class filtered_operation_object : public object<OperationType, filtered_operation_object<OperationType>>
{
public:
    CHAINBASE_DEFAULT_CONSTRUCTOR(filtered_operation_object)

    typedef typename object<OperationType, filtered_operation_object<OperationType>>::id_type id_type;

    id_type id;
    operation_object::id_type op;
};

template <blockchain_history_object_type OperationType>
using filtered_operation_index
    = shared_multi_index_container<filtered_operation_object<OperationType>,
                                   indexed_by<ordered_unique<tag<by_id>,
                                                             member<filtered_operation_object<OperationType>,
                                                                    typename filtered_operation_object<OperationType>::
                                                                        id_type,
                                                                    &filtered_operation_object<OperationType>::id>>>>;

using filtered_not_virt_operations_history_object
    = filtered_operation_object<filtered_not_virt_operations_history_object_type>;
using filtered_virt_operations_history_object = filtered_operation_object<filtered_virt_operations_history_object_type>;
using filtered_market_operations_history_object
    = filtered_operation_object<filtered_market_operations_history_object_type>;

using filtered_not_virt_operations_history_index
    = filtered_operation_index<filtered_not_virt_operations_history_object_type>;
using filtered_virt_operations_history_index = filtered_operation_index<filtered_virt_operations_history_object_type>;
using filtered_market_operations_history_index
    = filtered_operation_index<filtered_market_operations_history_object_type>;
}
}

FC_REFLECT(scorum::blockchain_history::operation_object,
           (id)(trx_id)(block)(trx_in_block)(op_in_trx)(timestamp)(serialized_op))
CHAINBASE_SET_INDEX_TYPE(scorum::blockchain_history::operation_object, scorum::blockchain_history::operation_index)

FC_REFLECT(scorum::blockchain_history::filtered_not_virt_operations_history_object, (id)(op))
CHAINBASE_SET_INDEX_TYPE(scorum::blockchain_history::filtered_not_virt_operations_history_object,
                         scorum::blockchain_history::filtered_not_virt_operations_history_index)
FC_REFLECT(scorum::blockchain_history::filtered_virt_operations_history_object, (id)(op))
CHAINBASE_SET_INDEX_TYPE(scorum::blockchain_history::filtered_virt_operations_history_object,
                         scorum::blockchain_history::filtered_virt_operations_history_index)
FC_REFLECT(scorum::blockchain_history::filtered_market_operations_history_object, (id)(op))
CHAINBASE_SET_INDEX_TYPE(scorum::blockchain_history::filtered_market_operations_history_object,
                         scorum::blockchain_history::filtered_market_operations_history_index)
