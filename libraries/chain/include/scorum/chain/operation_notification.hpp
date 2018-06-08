#pragma once

#include <scorum/protocol/operations.hpp>

#include <scorum/chain/schema/scorum_object_types.hpp>
#include <scorum/protocol/proposal_operations.hpp>

namespace scorum {
namespace chain {

template <class Op> struct basic_operation_notification
{
    basic_operation_notification(const Op& o)
        : op(o)
    {
    }

    protocol::transaction_id_type trx_id;
    uint32_t block = 0;
    uint32_t trx_in_block = 0;
    uint16_t op_in_trx = 0;
    const Op& op;
};

using operation_notification = basic_operation_notification<protocol::operation>;
using proposal_operation_notification = basic_operation_notification<protocol::proposal_operation>;
}
}
