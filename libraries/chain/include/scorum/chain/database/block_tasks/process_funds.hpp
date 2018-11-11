#pragma once
#include <scorum/chain/database/block_tasks/block_tasks.hpp>
#include <scorum/protocol/types.hpp>

namespace scorum {
namespace protocol {
struct asset;
}
namespace chain {

using scorum::protocol::budget_type;

class account_object;
struct dynamic_global_property_service_i;
struct advertising_property_service_i;
struct account_service_i;
struct content_reward_scr_service_i;
struct dev_pool_service_i;
struct fund_budget_service_i;
struct voters_reward_scr_service_i;
struct voters_reward_sp_service_i;
struct content_reward_fund_scr_service_i;
struct content_reward_fund_sp_service_i;
struct hardfork_property_service_i;
struct witness_service_i;
template <budget_type> struct adv_budget_service_i;

namespace database_ns {

using scorum::protocol::asset;

class process_funds : public block_task
{
public:
    process_funds(block_task_context&);
    virtual void on_apply(block_task_context&);

private:
    void distribute_reward(const asset& reward);
    void distribute_active_sp_holders_reward(const asset& reward);
    void distribute_witness_reward(const asset& reward);
    void pay_account_reward(const account_object&, const asset& reward);
    void pay_account_pending_reward(const account_object&, const asset& reward);
    void pay_witness_reward(const account_object&, const asset& reward);
    void pay_content_reward(const asset& reward);
    void pay_activity_reward(const asset& reward);
    const asset get_activity_reward(const asset& reward);
    bool apply_mainnet_schedule_crutches();

    asset run_auction_round();

    template <budget_type budget_type_v>
    asset process_adv_pending_payouts(adv_budget_service_i<budget_type_v>& budget_svc);

private:
    template <budget_type budget_type_v> void close_empty_budgets(adv_budget_service_i<budget_type_v>& budget_svc);

private:
    content_reward_scr_service_i& _content_reward_service;
    dev_pool_service_i& _dev_pool_service;
    dynamic_global_property_service_i& _dprops_service;
    fund_budget_service_i& _fund_budget_service;
    adv_budget_service_i<budget_type::post>& _post_budget_service;
    adv_budget_service_i<budget_type::banner>& _banner_budget_service;
    account_service_i& _account_service;
    advertising_property_service_i& _adv_property_svc;
    database_virtual_operations_emmiter_i& _virt_op_emitter;
    voters_reward_scr_service_i& _voter_reward_scr_svc;
    voters_reward_sp_service_i& _voter_reward_sp_svc;
    content_reward_fund_scr_service_i& _content_reward_fund_scr_svc;
    content_reward_fund_sp_service_i& _content_reward_fund_sp_svc;
    hardfork_property_service_i& _hardfork_svc;
    witness_service_i& _witness_svc;

    block_task_context& _ctx;

    uint32_t _block_num;
};
}
}
}
