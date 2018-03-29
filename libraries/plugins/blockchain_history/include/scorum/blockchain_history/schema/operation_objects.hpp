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

class operation_object : public object<operations_history, operation_object>
{
public:
    CHAINBASE_DEFAULT_DYNAMIC_CONSTRUCTOR(operation_object, (serialized_op))

    typedef typename object<operations_history, operation_object>::id_type id_type;

    id_type id;

    transaction_id_type trx_id;
    uint32_t block = 0;
    uint32_t trx_in_block = 0;
    uint16_t op_in_trx = 0;
    time_point_sec timestamp;
    fc::shared_buffer serialized_op;
};

struct by_location;
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

enum class applied_operation_type
{
    all = 0,
    not_virt,
    virt,
    market
};

template <applied_operation_type T> struct applied_operation_wrapper_type
{
};

using applied_operation_all = applied_operation_wrapper_type<applied_operation_type::all>;
using applied_operation_not_virt = applied_operation_wrapper_type<applied_operation_type::not_virt>;
using applied_operation_virt = applied_operation_wrapper_type<applied_operation_type::virt>;
using applied_operation_market = applied_operation_wrapper_type<applied_operation_type::market>;

using applied_operation_variant_type = fc::
    static_variant<applied_operation_all, applied_operation_not_virt, applied_operation_virt, applied_operation_market>;

inline applied_operation_variant_type get_applied_operation_variant(const applied_operation_type& opt)
{
    applied_operation_variant_type ret;
    ret.set_which(int(opt));
    return ret;
}

constexpr uint16_t get_object_type(uint16_t base_id, const applied_operation_type& opt)
{
    return base_id + (uint16_t)opt;
}

template <applied_operation_type OperationType>
class filtered_operation_object : public object<get_object_type(filtered_operations_history, OperationType),
                                                filtered_operation_object<OperationType>>
{
public:
    CHAINBASE_DEFAULT_CONSTRUCTOR(filtered_operation_object)

    typedef typename object<get_object_type(filtered_operations_history, OperationType),
                            filtered_operation_object<OperationType>>::id_type id_type;

    id_type id;

    operation_object::id_type op;
};

template <applied_operation_type OperationType>
using filtered_operation_index
    = shared_multi_index_container<filtered_operation_object<OperationType>,
                                   indexed_by<ordered_unique<tag<by_id>,
                                                             member<filtered_operation_object<OperationType>,
                                                                    typename filtered_operation_object<OperationType>::
                                                                        id_type,
                                                                    &filtered_operation_object<OperationType>::id>>>>;

using filtered_all_operation_object = filtered_operation_object<applied_operation_type::all>;
using filtered_not_virt_operation_object = filtered_operation_object<applied_operation_type::not_virt>;
using filtered_virt_operation_object = filtered_operation_object<applied_operation_type::virt>;
using filtered_market_operation_object = filtered_operation_object<applied_operation_type::market>;

using filtered_all_operation_index = filtered_operation_index<applied_operation_type::all>;
using filtered_not_virt_operation_index = filtered_operation_index<applied_operation_type::not_virt>;
using filtered_virt_operation_index = filtered_operation_index<applied_operation_type::virt>;
using filtered_market_operation_index = filtered_operation_index<applied_operation_type::market>;
}
}

FC_REFLECT(scorum::blockchain_history::operation_object,
           (id)(trx_id)(block)(trx_in_block)(op_in_trx)(timestamp)(serialized_op))
CHAINBASE_SET_INDEX_TYPE(scorum::blockchain_history::operation_object, scorum::blockchain_history::operation_index)

FC_REFLECT_ENUM(scorum::blockchain_history::applied_operation_type, (all)(not_virt)(virt)(market))

FC_REFLECT(scorum::blockchain_history::filtered_all_operation_object, (id)(op))
CHAINBASE_SET_INDEX_TYPE(scorum::blockchain_history::filtered_all_operation_object,
                         scorum::blockchain_history::filtered_all_operation_index)
FC_REFLECT(scorum::blockchain_history::filtered_not_virt_operation_object, (id)(op))
CHAINBASE_SET_INDEX_TYPE(scorum::blockchain_history::filtered_not_virt_operation_object,
                         scorum::blockchain_history::filtered_not_virt_operation_index)
FC_REFLECT(scorum::blockchain_history::filtered_virt_operation_object, (id)(op))
CHAINBASE_SET_INDEX_TYPE(scorum::blockchain_history::filtered_virt_operation_object,
                         scorum::blockchain_history::filtered_virt_operation_index)
FC_REFLECT(scorum::blockchain_history::filtered_market_operation_object, (id)(op))
CHAINBASE_SET_INDEX_TYPE(scorum::blockchain_history::filtered_market_operation_object,
                         scorum::blockchain_history::filtered_market_operation_index)
