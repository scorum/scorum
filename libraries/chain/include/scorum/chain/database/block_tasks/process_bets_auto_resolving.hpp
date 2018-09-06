#pragma once

#include <scorum/chain/database/block_tasks/block_tasks.hpp>

namespace scorum {
namespace chain {
namespace betting {
class betting_service_i;
class betting_resolver_i;
}
namespace database_ns {

struct process_bets_auto_resolving : public block_task
{
    process_bets_auto_resolving(betting::betting_service_i&, betting::betting_resolver_i&);

    virtual void on_apply(block_task_context&);

private:
    betting::betting_service_i& _betting_svc;
    betting::betting_resolver_i& _resolver;
};
}
}
}
