#pragma once

#include <scorum/chain/database/block_tasks/block_tasks.hpp>

namespace scorum {
namespace protocol {
struct asset;
}
namespace chain {
namespace database_ns {

class process_fifa_world_cup_2018_bounty_initialize : public block_task
{
public:
    virtual void on_apply(block_task_context&);
};
}
}
}
