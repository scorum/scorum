#include <scorum/chain/database/block_tasks/process_witness_reward_in_sp_migration.hpp>

#include <scorum/chain/services/dynamic_global_property.hpp>
#include <scorum/chain/services/witness_reward_in_sp_migration.hpp>
#include <scorum/chain/services/account.hpp>
#include <scorum/chain/services/blocks_story.hpp>

#include <scorum/chain/schema/witness_objects.hpp>

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
    debug_log(ctx.get_block_info(), "process_witness_reward_in_sp_migration BEGIN");

    dynamic_global_property_service_i& dprops_service = ctx.services().dynamic_global_property_service();

    if (dprops_service.head_block_time() < SCORUM_WITNESS_REWARD_MIGRATION_DATE)
    {
        return;
    }

    witness_reward_in_sp_migration_service_i& witness_reward_in_sp_migration_service
        = ctx.services().witness_reward_in_sp_migration_service();

    if (!witness_reward_in_sp_migration_service.is_exists())
    {
        return;
    }

    blocks_story_service_i& blocks_story_service = ctx.services().blocks_story_service();

    using recipients_type = std::map<account_name_type, share_type>;
    uint32_t block_num = dprops_service.get().head_block_number;
    share_type total_payment = witness_reward_in_sp_migration_service.get().balance;
    recipients_type witnesses;
    while (block_num > old_reward_alg_switch_reward_block_num && total_payment > share_type())
    {
        optional<signed_block> b = blocks_story_service.fetch_block_by_number(block_num--);
        if (b.valid())
        {
            signed_block block = *b;

            share_type payment = std::min(migrate_deferred_payment, total_payment);
            total_payment -= payment;

            witnesses[block.witness] += payment;
        }
    }

    if (block_num > old_reward_alg_switch_reward_block_num)
    {
        wlog("Migration rest is not enough. ${n} blocks left",
             ("n", block_num - old_reward_alg_switch_reward_block_num));
    }
    if (total_payment > 0)
    {
        wlog("Migration rest is too large. Left ${r}", ("r", asset(total_payment, SP_SYMBOL)));
    }

    for (const auto& witness_reward : witnesses)
    {
        charge_witness_reward(ctx, witness_reward.first, witness_reward.second);
    }

    witness_reward_in_sp_migration_service.update(
        [=](witness_reward_in_sp_migration_object& obj) { obj.balance = total_payment; });

    if (total_payment == share_type())
    {
        witness_reward_in_sp_migration_service.remove();
    }

    debug_log(ctx.get_block_info(), "process_witness_reward_in_sp_migration END");
}

void process_witness_reward_in_sp_migration::adjust_witness_reward(block_task_context& ctx, asset& witness_reward)
{
    dynamic_global_property_service_i& dprops_service = ctx.services().dynamic_global_property_service();

    if (dprops_service.head_block_time() > SCORUM_WITNESS_REWARD_MIGRATION_DATE)
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

void process_witness_reward_in_sp_migration::charge_witness_reward(block_task_context& ctx,
                                                                   const account_name_type& witness,
                                                                   const share_type& reward)
{
    data_service_factory_i& services = ctx.services();
    account_service_i& account_service = services.account_service();

    const auto& account = account_service.get_account(witness);

    account_service.create_scorumpower(account, asset(reward, SCORUM_SYMBOL));

    if (reward.value != 0)
        ctx.push_virtual_operation(producer_reward_operation(witness, asset(reward, SP_SYMBOL)));
}
}
}
}
