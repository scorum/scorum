#pragma once

#include <scorum/chain/database/block_tasks/block_tasks.hpp>
#include <scorum/protocol/asset.hpp>

namespace scorum {
namespace chain {
namespace database_ns {

using scorum::protocol::asset;
using scorum::protocol::share_type;

class process_witness_reward_in_sp_migration : public block_task
{
public:
    virtual void on_apply(block_task_context&);

    void adjust_witness_reward(block_task_context&, asset& witness_reward);

    static const uint32_t old_reward_alg_switch_reward_block_num;
    static const share_type new_reward_to_migrate;
    static const share_type old_reward_to_migrate;
    static const share_type migrate_deferred_payment;
};
}
}
}
