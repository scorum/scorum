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

typedef api_obj<scorum::chain::reward_fund_object> reward_fund_api_obj;

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
    // total SCR and SP (circulating_capital + pools supply)
    asset total_supply = asset(0, SCORUM_SYMBOL);

    // total SCR and SP on circulating (on account balances). circulating_capital <= total_supply
    asset circulating_capital = asset(0, SCORUM_SYMBOL);

    // total SP on accounts scorumpower
    asset total_scorumpower = asset(0, SP_SYMBOL);

    asset registration_pool_balance = asset(0, SCORUM_SYMBOL);
    asset fund_budget_balance = asset(0, SP_SYMBOL);
    asset reward_pool_balance = asset(0, SCORUM_SYMBOL);
    asset content_reward_balance = asset(0, SCORUM_SYMBOL);
};
}
}

FC_REFLECT(scorum::app::scheduled_hardfork_api_obj, (hf_version)(live_time))
FC_REFLECT(scorum::app::chain_capital_api_obj,
           (total_supply)(circulating_capital)(total_scorumpower)(registration_pool_balance)(fund_budget_balance)(
               reward_pool_balance)(content_reward_balance))

FC_REFLECT_DERIVED(scorum::app::reward_fund_api_obj, (scorum::chain::reward_fund_object), BOOST_PP_SEQ_NIL)
FC_REFLECT_DERIVED(scorum::app::chain_properties_api_obj,
                   (scorum::witness::reserve_ratio_object),
                   (chain_id)(head_block_id)(head_block_number)(last_irreversible_block_number)(current_aslot)(time)(
                       current_witness)(median_chain_props)(majority_version)(hf_version))
