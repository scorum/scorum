#pragma once

#include <scorum/chain/schema/scorum_object_types.hpp>

#include <boost/multi_index/composite_key.hpp>

namespace scorum {
namespace witness {

using namespace scorum::chain;

#ifndef WITNESS_SPACE_ID
#define WITNESS_SPACE_ID 7
#endif

enum witness_plugin_object_type
{
    account_bandwidth_object_type = (WITNESS_SPACE_ID << OBJECT_TYPE_SPACE_ID_OFFSET),
    reserve_ratio_object_type
};

enum bandwidth_type
{
    post, ///< Rate limiting posting reward eligibility over time
    forum, ///< Rate limiting for all forum related actins
    market ///< Rate limiting for all other actions
};

class account_bandwidth_object : public object<account_bandwidth_object_type, account_bandwidth_object>
{
public:
    CHAINBASE_DEFAULT_CONSTRUCTOR(account_bandwidth_object)

    id_type id;

    account_name_type account;
    bandwidth_type type;
    share_type average_bandwidth;
    share_type lifetime_bandwidth;
    time_point_sec last_bandwidth_update;
};

typedef oid<account_bandwidth_object> account_bandwidth_id_type;

class reserve_ratio_object : public object<reserve_ratio_object_type, reserve_ratio_object>
{
public:
    CHAINBASE_DEFAULT_CONSTRUCTOR(reserve_ratio_object)

    id_type id;

    /**
     *  Average block size is updated every block to be:
     *
     *     average_block_size = (99 * average_block_size + new_block_size) / 100
     *
     *  This property is used to update the current_reserve_ratio to maintain approximately
     *  50% or less utilization of network capacity.
     */
    int32_t average_block_size = 0;

    /**
     *   Any time average_block_size <= 50% maximum_block_size this value grows by 1 until it
     *   reaches SCORUM_MAX_RESERVE_RATIO.  Any time average_block_size is greater than
     *   50% it falls by 1%.  Upward adjustments happen once per round, downward adjustments
     *   happen every block.
     */
    int64_t current_reserve_ratio = 1;

    /**
     * The maximum bandwidth the blockchain can support is:
     *
     *    max_bandwidth = maximum_block_size * SCORUM_BANDWIDTH_AVERAGE_WINDOW_SECONDS / SCORUM_BLOCK_INTERVAL
     *
     * The maximum virtual bandwidth is:
     *
     *    max_bandwidth * current_reserve_ratio
     */
    uint128_t max_virtual_bandwidth = 0;
};

typedef oid<reserve_ratio_object> reserve_ratio_id_type;

struct by_account_bandwidth_type;

typedef shared_multi_index_container<account_bandwidth_object,
                                     indexed_by<ordered_unique<tag<by_id>,
                                                               member<account_bandwidth_object,
                                                                      account_bandwidth_id_type,
                                                                      &account_bandwidth_object::id>>,
                                                ordered_unique<tag<by_account_bandwidth_type>,
                                                               composite_key<account_bandwidth_object,
                                                                             member<account_bandwidth_object,
                                                                                    account_name_type,
                                                                                    &account_bandwidth_object::account>,
                                                                             member<account_bandwidth_object,
                                                                                    bandwidth_type,
                                                                                    &account_bandwidth_object::type>>>>>
    account_bandwidth_index;

typedef shared_multi_index_container<reserve_ratio_object,
                                     indexed_by<ordered_unique<tag<by_id>,
                                                               member<reserve_ratio_object,
                                                                      reserve_ratio_id_type,
                                                                      &reserve_ratio_object::id>>>>
    reserve_ratio_index;
}
} // scorum::witness

// clang-format off

FC_REFLECT_ENUM(scorum::witness::bandwidth_type, (post)(forum)(market))

FC_REFLECT(scorum::witness::account_bandwidth_object,
    (id)(account)(type)(average_bandwidth)(lifetime_bandwidth)(last_bandwidth_update))
CHAINBASE_SET_INDEX_TYPE(scorum::witness::account_bandwidth_object, scorum::witness::account_bandwidth_index)

FC_REFLECT(
    scorum::witness::reserve_ratio_object, (id)(average_block_size)(current_reserve_ratio)(max_virtual_bandwidth))
CHAINBASE_SET_INDEX_TYPE(scorum::witness::reserve_ratio_object, scorum::witness::reserve_ratio_index)

// clang-format on
