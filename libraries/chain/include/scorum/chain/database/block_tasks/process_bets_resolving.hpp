#pragma once

#include <scorum/chain/database/block_tasks/block_tasks.hpp>

namespace scorum {
namespace chain {
class betting_service_i;
class betting_resolver_i;
namespace database_ns {

struct process_bets_resolving : public block_task
{
    process_bets_resolving(betting_service_i&, betting_resolver_i&);

    virtual void on_apply(block_task_context&);

private:
    betting_service_i& _betting_svc;
    betting_resolver_i& _resolver;
};
}
}
}
