#pragma once
#include <scorum/utils/any_range.hpp>
#include <scorum/chain/database/block_tasks/block_tasks.hpp>

namespace scorum {
namespace chain {
struct betting_service_i;
struct database_virtual_operations_emmiter_i;
namespace database_ns {

struct process_bets_auto_resolving : public block_task
{
    process_bets_auto_resolving(betting_service_i&, database_virtual_operations_emmiter_i&);

    virtual void on_apply(block_task_context&);

private:
    betting_service_i& _betting_svc;
    database_virtual_operations_emmiter_i& _virt_op_emitter;
};
}
}
}
