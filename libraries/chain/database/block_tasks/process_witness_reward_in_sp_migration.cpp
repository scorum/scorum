#include <scorum/chain/database/block_tasks/process_witness_reward_in_sp_migration.hpp>

#include <scorum/chain/services/dynamic_global_property.hpp>
#include <scorum/chain/services/witness_reward_in_sp_migration.hpp>

namespace scorum {
namespace chain {
namespace database_ns {

const uint32_t process_witness_reward_in_sp_migration::old_reward_alg_switch_reward_block_num = 24;
const share_type process_witness_reward_in_sp_migration::new_reward_to_migrate = 22831050;
const share_type process_witness_reward_in_sp_migration::old_reward_to_migrate = 21689497;
const share_type process_witness_reward_in_sp_migration::migrate_deferred_payment
    = (process_witness_reward_in_sp_migration::new_reward_to_migrate
       - process_witness_reward_in_sp_migration::old_reward_to_migrate);

void process_witness_reward_in_sp_migration::on_apply(block_task_context& ctx)
{
    dynamic_global_property_service_i& dprops_service = ctx.services().dynamic_global_property_service();

    if (dprops_service.head_block_time() < SCORUM_WITNESS_REWARD_MIGRATION_DATE)
    {
        return;
    }

    // TODO
}

void process_witness_reward_in_sp_migration::adjust_witness_reward(block_task_context& ctx, asset& witness_reward)
{
    dynamic_global_property_service_i& dprops_service = ctx.services().dynamic_global_property_service();

    if (dprops_service.head_block_time() >= SCORUM_WITNESS_REWARD_MIGRATION_DATE)
    {
        return;
    }

    if (witness_reward.symbol() == SP_SYMBOL)
    {
        dynamic_global_property_service_i& dgp_service = ctx.services().dynamic_global_property_service();

        if (dgp_service.get().head_block_number > old_reward_alg_switch_reward_block_num
            && witness_reward.amount == new_reward_to_migrate)
        {
            witness_reward_in_sp_migration_service_i& witness_reward_in_sp_migration_service
                = ctx.services().witness_reward_in_sp_migration_service();

            witness_reward.amount -= migrate_deferred_payment;

            witness_reward_in_sp_migration_service.update(
                [=](witness_reward_in_sp_migration_object& wri) { wri.balance += migrate_deferred_payment; });
        }
    }
}
}
}
}
