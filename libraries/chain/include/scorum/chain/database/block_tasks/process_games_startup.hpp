#pragma once

#include <scorum/chain/database/block_tasks/block_tasks.hpp>

namespace scorum {
namespace chain {
namespace betting {
class betting_resolver_i;
}
namespace database_ns {

struct process_games_startup : public block_task
{
    process_games_startup(betting::betting_resolver_i&);

    virtual void on_apply(block_task_context&);

private:
    betting::betting_resolver_i& _resolver;
};
}
}
}
