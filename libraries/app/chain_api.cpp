#include <scorum/app/chain_api.hpp>
#include <scorum/app/api_context.hpp>
#include <scorum/app/application.hpp>
#include <scorum/chain/services/budget.hpp>
#include <scorum/chain/services/development_committee.hpp>
#include <scorum/chain/services/dynamic_global_property.hpp>
#include <scorum/chain/services/hardfork_property.hpp>
#include <scorum/chain/services/registration_pool.hpp>
#include <scorum/chain/services/reward_balancer.hpp>
#include <scorum/chain/services/reward_funds.hpp>
#include <scorum/chain/services/witness_reward_in_sp_migration.hpp>
#include <scorum/chain/schema/budget_object.hpp>
#include <scorum/chain/schema/dev_committee_object.hpp>
#include <scorum/chain/schema/reward_balancer_objects.hpp>
#include <scorum/chain/schema/scorum_objects.hpp>

namespace scorum {
namespace app {

using namespace scorum::chain;

chain_api::chain_api(const api_context& ctx)
    : _db(*ctx.app.chain_database())
{
}

void chain_api::on_api_startup()
{
}

chain_properties_api_obj chain_api::get_chain_properties() const
{
    return _db.with_read_lock([&]() {

        chain_properties_api_obj ret_val;

        if (_db.has_index<witness::reserve_ratio_index>())
        {
            const auto& r = _db.find(witness::reserve_ratio_id_type());

            if (BOOST_LIKELY(r != nullptr))
            {
                ret_val = *r;
            }
        }

        const dynamic_global_property_object& dpo = _db.obtain_service<dbs_dynamic_global_property>().get();

        ret_val.head_block_id = dpo.head_block_id;
        ret_val.head_block_number = dpo.head_block_number;
        ret_val.last_irreversible_block_number = dpo.last_irreversible_block_num;
        ret_val.current_aslot = dpo.current_aslot;
        ret_val.time = dpo.time;
        ret_val.current_witness = dpo.current_witness;
        ret_val.majority_version = dpo.majority_version;
        ret_val.median_chain_props = dpo.median_chain_props;
        ret_val.chain_id = _db.get_chain_id();
        ret_val.hf_version = _db.obtain_service<dbs_hardfork_property>().get().current_hardfork_version;

        return ret_val;
    });
}

scheduled_hardfork_api_obj chain_api::get_next_scheduled_hardfork() const
{
    return _db.with_read_lock([&]() {
        scheduled_hardfork_api_obj shf;
        const auto& hpo = _db.obtain_service<dbs_hardfork_property>().get();
        shf.hf_version = hpo.next_hardfork;
        shf.live_time = hpo.next_hardfork_time;
        return shf;
    });
}

reward_fund_api_obj chain_api::get_reward_fund(reward_fund_type type_of_fund) const
{
    return _db.with_read_lock([&]() {
        switch (type_of_fund)
        {
        case reward_fund_type::content_reward_fund_scr:
        {
            auto& rf_service = _db.obtain_service<dbs_content_reward_fund_scr>();
            FC_ASSERT(rf_service.is_exists(), "${f} object does not exist", ("f", type_of_fund));
            return reward_fund_api_obj(rf_service.get());
        }
        case reward_fund_type::content_reward_fund_sp:
        {
            auto& rf_service = _db.obtain_service<dbs_content_reward_fund_sp>();
            FC_ASSERT(rf_service.is_exists(), "${f} object does not exist", ("f", type_of_fund));
            return reward_fund_api_obj(rf_service.get());
        }
        case reward_fund_type::content_fifa_world_cup_2018_bounty_reward_fund:
        {
            auto& rf_service = _db.obtain_service<dbs_content_fifa_world_cup_2018_bounty_reward_fund>();
            FC_ASSERT(rf_service.is_exists(), "${f} object does not exist", ("f", type_of_fund));
            return reward_fund_api_obj(rf_service.get());
        }
        default:
            FC_ASSERT(false, "Unknown fund");
            return reward_fund_api_obj();
        }
    });
}

chain_capital_api_obj chain_api::get_chain_capital() const
{
    return _db.with_read_lock([&]() {

        // clang-format off
        chain_capital_api_obj capital;

        const dynamic_global_property_object& dpo = _db.obtain_service<dbs_dynamic_global_property>().get();

        capital.head_block_number = dpo.head_block_number;
        capital.head_block_id = dpo.head_block_id;
        capital.head_block_time = dpo.time;
        capital.current_witness = dpo.current_witness;

        capital.total_supply = dpo.total_supply;
        capital.circulating_capital = dpo.circulating_capital;
        capital.total_scorumpower = dpo.total_scorumpower;
        capital.total_witness_reward_scr = dpo.total_witness_reward_scr;
        capital.total_witness_reward_sp = dpo.total_witness_reward_sp;

        capital.registration_pool_balance = _db.obtain_service<dbs_registration_pool>().get().balance;
        capital.fund_budget_balance = _db.obtain_service<dbs_budget>().get_fund_budget().balance;

        capital.dev_pool_scr_balance = _db.obtain_service<dbs_development_committee>().get().scr_balance;
        capital.dev_pool_sp_balance = _db.obtain_service<dbs_development_committee>().get().sp_balance;

        capital.content_balancer_scr = _db.obtain_service<dbs_content_reward_scr>().get().balance;
        capital.active_voters_balancer_scr = _db.obtain_service<dbs_voters_reward_scr>().get().balance;
        capital.active_voters_balancer_sp = _db.obtain_service<dbs_voters_reward_sp>().get().balance;

        capital.content_reward_fund_scr_balance = _db.obtain_service<dbs_content_reward_fund_scr>().get().activity_reward_balance;
        capital.content_reward_fund_sp_balance = _db.obtain_service<dbs_content_reward_fund_sp>().get().activity_reward_balance;

        const auto& fifa_world_cup_2018_bounty_fund_service = _db.obtain_service<dbs_content_fifa_world_cup_2018_bounty_reward_fund>();
        if (fifa_world_cup_2018_bounty_fund_service.is_exists())
        {
            capital.content_reward_fifa_world_cup_2018_bounty_fund_sp_balance = fifa_world_cup_2018_bounty_fund_service.get().activity_reward_balance;
        }
        
        const auto& migration_service = _db.obtain_service<dbs_witness_reward_in_sp_migration>();
        if (migration_service.is_exists())
        {
            capital.witness_reward_in_sp_migration_fund = asset(migration_service.get().balance, SP_SYMBOL);
        }
        // clang-format on

        return capital;
    });
}
}
}
