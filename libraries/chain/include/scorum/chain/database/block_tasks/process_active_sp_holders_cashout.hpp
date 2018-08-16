#pragma once

#include <scorum/chain/database/block_tasks/block_tasks.hpp>

namespace scorum {
namespace chain {
namespace database_ns {

struct process_active_sp_holders_cashout : public block_task
{
    virtual void on_apply(block_task_context&);
};
}
}
}
