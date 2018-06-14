#pragma once

#include <scorum/chain/schema/scorum_object_types.hpp>
#include <scorum/protocol/operations.hpp>

namespace scorum {
namespace chain {

struct operation_notification
{
    operation_notification(protocol::transaction_id_type trx_id,
                           uint32_t block,
                           uint32_t trx_in_block,
                           uint16_t op_in_trx,
                           const protocol::operation& o)
        : trx_id(trx_id)
        , block(block)
        , trx_in_block(trx_in_block)
        , op_in_trx(op_in_trx)
        , op(o)
    {
    }

    const protocol::transaction_id_type trx_id;
    const uint32_t block = 0;
    const uint32_t trx_in_block = 0;
    const uint16_t op_in_trx = 0;
    const protocol::operation& op;
};
}
}
