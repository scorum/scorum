#pragma once

#include <scorum/protocol/types.hpp>
#include <scorum/protocol/chain_properties.hpp>
#include <scorum/protocol/version.hpp>
#include <scorum/app/schema/api_template.hpp>
#include <scorum/chain/schema/scorum_objects.hpp>
#include <scorum/witness/witness_objects.hpp>

namespace scorum {
namespace app {

using namespace scorum::protocol;

struct reward_fund_api_obj
{
    reward_fund_api_obj()
    {
    }

    template <class FundType>
    reward_fund_api_obj(const FundType& obj)
        : activity_reward_balance(obj.activity_reward_balance)
        , recent_claims(obj.recent_claims)
        , last_update(obj.last_update)
        , author_reward_curve(obj.author_reward_curve)
        , curation_reward_curve(obj.curation_reward_curve)
    {
    }

    asset activity_reward_balance; // in SCR or SP
    fc::uint128_t recent_claims = 0;
    time_point_sec last_update;
    curve_id author_reward_curve;
    curve_id curation_reward_curve;
};

struct scheduled_hardfork_api_obj
{
    hardfork_version hf_version;
    fc::time_point_sec live_time;
};

struct chain_properties_api_obj : public api_obj<scorum::witness::reserve_ratio_object>
{
    template <class T> chain_properties_api_obj& operator=(const T& other)
    {
        T& base = static_cast<T&>(*this);
        base = other;
        return *this;
    }

    chain_id_type chain_id;

    block_id_type head_block_id;

    uint32_t head_block_number = 0;

    uint32_t last_irreversible_block_number = 0;

    /**
    * The current absolute slot number.  Equal to the total
    * number of slots since genesis.  Also equal to the total
    * number of missed slots plus head_block_number.
    */
    uint64_t current_aslot = 0;

    time_point_sec time;

    account_name_type current_witness;

    /**
    *  Chain properties are decided by the set of active witnesses which change every round.
    *  Each witness posts what they think the chain properties should be as part of their witness
    *  properties, the median size is chosen to be the chain properties for the round.
    *
    *  @note the minimum value for maximum_block_size is defined by the protocol to prevent the
    *  network from getting stuck by witnesses attempting to set this too low.
    */

    chain_properties median_chain_props;

    version majority_version;

    hardfork_version hf_version;
};

struct chain_capital_api_obj
{
    uint32_t head_block_number = 0;
    block_id_type head_block_id;
    time_point_sec head_block_time;
    account_name_type current_witness;

    // total SCR and SP (circulating_capital + pools supply)
    asset total_supply = asset(0, SCORUM_SYMBOL);

    // total SCR and SP on circulating (on account balances). circulating_capital <= total_supply
    asset circulating_capital = asset(0, SCORUM_SYMBOL);

    // total SCR on accounts
    asset total_scr = asset(0, SCORUM_SYMBOL);

    // total SP on accounts
    asset total_scorumpower = asset(0, SP_SYMBOL);

    asset registration_pool_balance = asset(0, SCORUM_SYMBOL);
    asset fund_budget_balance = asset(0, SP_SYMBOL);
    asset dev_pool_scr_balance = asset(0, SCORUM_SYMBOL);
    asset dev_pool_sp_balance = asset(0, SP_SYMBOL);

    asset content_balancer_scr = asset(0, SCORUM_SYMBOL);
    asset active_voters_balancer_scr = asset(0, SCORUM_SYMBOL);
    asset active_voters_balancer_sp = asset(0, SP_SYMBOL);

    asset content_reward_fund_scr_balance = asset(0, SCORUM_SYMBOL);
    asset content_reward_fund_sp_balance = asset(0, SP_SYMBOL);
    asset content_reward_fifa_world_cup_2018_bounty_fund_sp_balance = asset(0, SP_SYMBOL);

    asset total_witness_reward_scr = asset(0, SCORUM_SYMBOL);
    asset total_witness_reward_sp = asset(0, SP_SYMBOL);
    asset witness_reward_in_sp_migration_fund = asset(0, SP_SYMBOL);
};
}
}

// clang-format off
FC_REFLECT(scorum::app::reward_fund_api_obj,
           (activity_reward_balance)
           (recent_claims)
           (last_update)
           (author_reward_curve)
           (curation_reward_curve))

FC_REFLECT(scorum::app::scheduled_hardfork_api_obj,
           (hf_version)
           (live_time))

FC_REFLECT_DERIVED(scorum::app::chain_properties_api_obj,
                   (scorum::witness::reserve_ratio_object),
                   (chain_id)
                   (head_block_id)
                   (head_block_number)
                   (last_irreversible_block_number)
                   (current_aslot)
                   (time)
                   (current_witness)
                   (median_chain_props)
                   (majority_version)
                   (hf_version))

FC_REFLECT(scorum::app::chain_capital_api_obj,
           (head_block_number)
           (head_block_id)
           (head_block_time)
           (current_witness)
           (total_supply)
           (circulating_capital)
           (total_scr)
           (total_scorumpower)
           (registration_pool_balance)
           (fund_budget_balance)
           (dev_pool_scr_balance)
           (dev_pool_sp_balance)
           (content_balancer_scr)
           (active_voters_balancer_scr)
           (active_voters_balancer_sp)
           (content_reward_fund_scr_balance)
           (content_reward_fund_sp_balance)
           (content_reward_fifa_world_cup_2018_bounty_fund_sp_balance)
           (total_witness_reward_scr)
           (total_witness_reward_sp)
           (witness_reward_in_sp_migration_fund))
// clang-format on
