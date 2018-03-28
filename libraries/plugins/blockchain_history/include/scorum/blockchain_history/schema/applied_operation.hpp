#pragma once

#include <scorum/protocol/operations.hpp>
#include "operation_objects.hpp"

namespace scorum {
namespace blockchain_history {

using scorum::protocol::transaction_id_type;
using scorum::protocol::operation;

struct applied_operation
{
    applied_operation();
    applied_operation(const operation_object& op_obj);

    transaction_id_type trx_id;
    uint32_t block = 0;
    uint32_t trx_in_block = 0;
    uint16_t op_in_trx = 0;
    fc::time_point_sec timestamp;
    operation op;
};

enum class applied_operation_type
{
    all = 0,
    not_virtual_operation,
    virtual_operation,
    market_operation
};
}
}

FC_REFLECT_ENUM(scorum::blockchain_history::applied_operation_type,
                (all)(not_virtual_operation)(virtual_operation)(market_operation))

FC_REFLECT(scorum::blockchain_history::applied_operation, (trx_id)(block)(trx_in_block)(op_in_trx)(timestamp)(op))
