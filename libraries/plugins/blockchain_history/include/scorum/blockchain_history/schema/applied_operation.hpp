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

struct applied_withdraw_operation : public applied_operation
{
    enum withdraw_status
    {
        active,
        finished,
        interrupted,
        empty
    };

    applied_withdraw_operation();
    applied_withdraw_operation(const operation_object& op_obj);

    asset withdrawn = asset(0, SP_SYMBOL);
    withdraw_status status = active;
};

enum class applied_operation_type
{
    all = 0,
    not_virt,
    virt,
    market
};
}
}

FC_REFLECT_ENUM(scorum::blockchain_history::applied_operation_type, (all)(not_virt)(virt)(market))

FC_REFLECT(scorum::blockchain_history::applied_operation, (trx_id)(block)(trx_in_block)(op_in_trx)(timestamp)(op))
FC_REFLECT_DERIVED(scorum::blockchain_history::applied_withdraw_operation,
                   (scorum::blockchain_history::applied_operation),
                   (withdrawn)(status))

FC_REFLECT_ENUM(scorum::blockchain_history::applied_withdraw_operation::withdraw_status,
                (active)(finished)(interrupted)(empty))
