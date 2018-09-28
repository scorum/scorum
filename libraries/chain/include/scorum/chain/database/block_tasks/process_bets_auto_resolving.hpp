#pragma once

#include <scorum/chain/database/block_tasks/block_tasks.hpp>

namespace scorum {
namespace chain {
struct betting_service_i;
namespace database_ns {

struct process_bets_auto_resolving : public block_task
{
    process_bets_auto_resolving(betting_service_i&);

    virtual void on_apply(block_task_context&);

private:
    betting_service_i& _betting_svc;
};
}
}
}
