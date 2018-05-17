#include <scorum/chain/database/block_tasks/process_witness_reward_in_sp_migration.hpp>

#include <scorum/chain/services/dynamic_global_property.hpp>
#include <scorum/chain/services/witness_reward_in_sp_migration.hpp>

namespace scorum {
namespace chain {
namespace database_ns {

void process_witness_reward_in_sp_migration::on_apply(block_task_context&)
{
    // TODO
}

void process_witness_reward_in_sp_migration::adjust_witness_reward(block_task_context& ctx, asset& witness_reward)
{
    if (witness_reward.symbol() == SP_SYMBOL)
    {
        dynamic_global_property_service_i& dgp_service = ctx.services().dynamic_global_property_service();

        if (dgp_service.get().head_block_number > 24)
        {
            witness_reward_in_sp_migration_service_i& witness_reward_in_sp_migration_service
                = ctx.services().witness_reward_in_sp_migration_service();

            const share_value_type rest = 1141553;
            FC_ASSERT(witness_reward.amount > rest);
            witness_reward.amount -= rest;

            witness_reward_in_sp_migration_service.update(
                [=](witness_reward_in_sp_migration_object& wri) { wri.balance += rest; });
        }
    }
}
}
}
}
