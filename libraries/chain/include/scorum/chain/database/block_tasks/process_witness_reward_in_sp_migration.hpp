#pragma once

#include <scorum/chain/database/block_tasks/block_tasks.hpp>

namespace scorum {
namespace protocol {
struct asset;
}
namespace chain {
class account_object;
namespace database_ns {

using scorum::protocol::asset;

class process_witness_reward_in_sp_migration : public block_task
{
public:
    virtual void on_apply(block_task_context&);

    void adjust_witness_reward(block_task_context&, asset& witness_reward);
};
}
}
}
