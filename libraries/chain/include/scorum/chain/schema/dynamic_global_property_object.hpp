#pragma once
#include <fc/uint128.hpp>

#include <scorum/chain/schema/scorum_object_types.hpp>

#include <scorum/protocol/asset.hpp>
#include <scorum/protocol/version.hpp>
#include <scorum/protocol/chain_properties.hpp>

namespace scorum {
namespace chain {

using scorum::protocol::asset;
using scorum::protocol::chain_properties;
using scorum::protocol::version;

struct adv_total_stats
{
    struct budget_type_stat
    {
        /// sum of all budgets' balance
        asset volume = asset(0, SCORUM_SYMBOL);

        /// pending volume which will go to the activity pool
        asset budget_pending_outgo = asset(0, SCORUM_SYMBOL);

        /// pending volume which will be returned to budget owners
        asset owner_pending_income = asset(0, SCORUM_SYMBOL);
    };

    /// total's statistic for advertising banner budgets
    budget_type_stat banner_budgets;

    /// total's statistic for advertising post budgets
    budget_type_stat post_budgets;
};

struct betting_total_stats
{
    asset pending_bets_volume = asset(0, SCORUM_SYMBOL);
    asset matched_bets_volume = asset(0, SCORUM_SYMBOL);
};

/**
 * @class dynamic_global_property_object
 * @brief Maintains global state information
 * @ingroup object
 * @ingroup implementation
 *
 * This is an implementation detail. The values here are calculated during normal chain operations and reflect the
 * current values of global blockchain properties.
 */
class dynamic_global_property_object
    : public object<dynamic_global_property_object_type, dynamic_global_property_object>
{
public:
    CHAINBASE_DEFAULT_CONSTRUCTOR(dynamic_global_property_object)

    id_type id;

    /// Current head block number
    uint32_t head_block_number = 0;

    /// Current head block id
    block_id_type head_block_id;

    /// Current head block time
    time_point_sec time;

    /// Head block signed by current witness
    account_name_type current_witness;

    /// Total SCR and SP (circulating_scr + circulating_sp)
    asset total_supply = asset(0, SCORUM_SYMBOL);

    /// Total SCR and SP on circulating (on account balances). circulating_capital <= total_supply
    asset circulating_capital = asset(0, SCORUM_SYMBOL);

    /// Total SP on accounts scorumpower
    asset total_scorumpower = asset(0, SP_SYMBOL);

    /// Total amount of SCR received by witnesses
    asset total_witness_reward_scr = asset(0, SCORUM_SYMBOL);

    /// Total amount of SP received by witnesses
    asset total_witness_reward_sp = asset(0, SP_SYMBOL);

    /// Total amount of pending SCR
    asset total_pending_scr = asset(0, SCORUM_SYMBOL);

    /// Total amount of pending SP
    asset total_pending_sp = asset(0, SP_SYMBOL);

    /// Total amount of burned SCR
    asset total_burned_scr = asset(0, SCORUM_SYMBOL);

    /// Chain properties are decided by the set of active witnesses which change every round.
    /// Each witness posts what they think the chain properties should be as part of their witness
    /// properties, the median size is chosen to be the chain properties for the round.
    ///
    /// @note the minimum value for maximum_block_size is defined by the protocol to prevent the
    /// network from getting stuck by witnesses attempting to set this too low.
    chain_properties median_chain_props;

    /// Witnesses majority version
    version majority_version;

    /// The current absolute slot number. Equal to the total number of slots since genesis. Also equal to the total
    /// number of missed slots plus head_block_number.
    uint64_t current_aslot = 0;

    /// used to compute witness participation.
    fc::uint128_t recent_slots_filled;

    /// Divide by 128 to compute participation percentage
    uint8_t participation_count = 0;

    /// last irreversible block num
    uint32_t last_irreversible_block_num = 0;

    /// this section display information about advertising totals
    adv_total_stats advertising;

    /// this section display information about betting totals
    betting_total_stats betting_stats;
};

typedef shared_multi_index_container<dynamic_global_property_object,
                                     indexed_by<ordered_unique<tag<by_id>,
                                                               member<dynamic_global_property_object,
                                                                      dynamic_global_property_object::id_type,
                                                                      &dynamic_global_property_object::id>>>>
    dynamic_global_property_index;
} // namespace chain
} // namespace scorum

// clang-format off
FC_REFLECT(scorum::chain::dynamic_global_property_object,
          (id)
          (head_block_number)
          (head_block_id)
          (time)
          (current_witness)
          (total_supply)
          (circulating_capital)
          (total_scorumpower)
          (total_witness_reward_scr)
          (total_witness_reward_sp)
          (total_pending_scr)
          (total_pending_sp)
          (total_burned_scr)
          (median_chain_props)
          (majority_version)
          (current_aslot)
          (recent_slots_filled)
          (participation_count)
          (last_irreversible_block_num)
          (advertising)
          (betting_stats))

FC_REFLECT(scorum::chain::adv_total_stats::budget_type_stat, (volume)(budget_pending_outgo)(owner_pending_income))
FC_REFLECT(scorum::chain::adv_total_stats, (post_budgets)(banner_budgets))
FC_REFLECT(scorum::chain::betting_total_stats, (pending_bets_volume)(matched_bets_volume))
// clang-format on

CHAINBASE_SET_INDEX_TYPE(scorum::chain::dynamic_global_property_object, scorum::chain::dynamic_global_property_index)
