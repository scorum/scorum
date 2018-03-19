#pragma once

#include <scorum/protocol/operations.hpp>

#include <scorum/chain/schema/scorum_object_types.hpp>

namespace scorum {
namespace chain {

struct operation_notification
{
    operation_notification(const protocol::operation& o)
        : op(o)
    {
    }

    protocol::transaction_id_type trx_id;
    uint32_t block = 0;
    uint32_t trx_in_block = 0;
    uint16_t op_in_trx = 0;
    uint64_t virtual_op = 0;
    const protocol::operation& op;
};
}
}
