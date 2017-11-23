#pragma once

#include <fc/fixed_string.hpp>

#include <scorum/protocol/authority.hpp>
#include <scorum/protocol/scorum_operations.hpp>

#include <scorum/chain/scorum_object_types.hpp>
#include <scorum/chain/witness_objects.hpp>
#include <scorum/chain/shared_authority.hpp>

#include <boost/multi_index/composite_key.hpp>

#include <numeric>

namespace scorum {
namespace chain {

using scorum::protocol::asset;

class budget_object : public object<budget_object_type, budget_object>
{
    budget_object() = delete;

public:

    template <typename Constructor, typename Allocator>
    budget_object(Constructor&& c, allocator<Allocator> a): content_permlink(a)
    {
        c(*this);
    }

    id_type id;

    time_point_sec created = time_point_sec::min();
    account_name_type owner;
    shared_string content_permlink;

    asset balance = asset(0, SCORUM_SYMBOL);
    share_type per_block = 0;
    uint32_t block_last_allocated_for = 0;
    bool auto_close = true;

};

class budget_with_schedule_object: public object<budget_with_schedule_object_type, budget_with_schedule_object>
{
    budget_with_schedule_object() = delete;

public:

    template <typename Constructor, typename Allocator>
    budget_with_schedule_object(Constructor&& c, allocator<Allocator> a)
    {
        c(*this);
    }

    id_type id;

    budget_id_type budget;
    budget_schedule_id_type schedule;
};

class budget_schedule_object : public object<budget_schedule_object_type, budget_schedule_object>
{
    budget_schedule_object() = delete;

public:
    enum budget_schedule_type
    {
        unconditional, /**< _Unconditional schedule. If it is unlocked, it works (time parameters are ignored)_ */
        by_time_range, /**< _Conditional schedule: [start_date, end_date]_ */
        by_period,     /**< _Conditional schedule: [start_date, start_date + period]_ */
    };

    template <typename Constructor, typename Allocator>
    budget_schedule_object(Constructor&& c, allocator<Allocator> a)
    {
        c(*this);
    }

    id_type id;

    budget_schedule_type schedule_alg = unconditional;

    time_point_sec start_date = time_point_sec::min();
    time_point_sec end_date = time_point_sec::maximum();
    uint32_t period = time_point_sec::maximum().sec_since_epoch();

    bool locked = false;
};

struct by_owner_name;

typedef multi_index_container<budget_object,
                              indexed_by<ordered_unique<tag<by_id>,
                                                        member<budget_object,
                                                               budget_id_type,
                                                               &budget_object::id>>,
                                         ordered_non_unique<tag<by_owner_name>,
                                                        member<budget_object,
                                                               account_name_type,
                                                               &budget_object::owner>>>,
                              allocator<budget_object>>
    budget_index;

struct by_budget_id;
struct by_schedule_id;

typedef multi_index_container<budget_with_schedule_object,
                              indexed_by<ordered_unique<tag<by_id>,
                                                        member<budget_with_schedule_object,
                                                               budget_with_schedule_id_type,
                                                              &budget_with_schedule_object::id>>,
                                         ordered_non_unique<tag<by_budget_id>,
                                                        member<budget_with_schedule_object,
                                                               budget_id_type,
                                                               &budget_with_schedule_object::budget>>,
                                         ordered_non_unique<tag<by_schedule_id>,
                                                        member<budget_with_schedule_object,
                                                               budget_schedule_id_type,
                                                               &budget_with_schedule_object::schedule>>>,
                              allocator<budget_with_schedule_object>>
    budget_with_schedule_index;

typedef multi_index_container<budget_schedule_object,
                              indexed_by<ordered_unique<tag<by_id>,
                                                        member<budget_schedule_object,
                                                               budget_schedule_id_type,
                                                               &budget_schedule_object::id>>>,
                              allocator<budget_schedule_object>>
    budget_schedule_index;

}
}

FC_REFLECT( scorum::chain::budget_object,
             (id)(created)(owner)(content_permlink)(balance)(per_block)(block_last_allocated_for)(auto_close)
)
CHAINBASE_SET_INDEX_TYPE( scorum::chain::budget_object, scorum::chain::budget_index )

FC_REFLECT( scorum::chain::budget_with_schedule_object,
             (id)(budget)(schedule)
)
CHAINBASE_SET_INDEX_TYPE( scorum::chain::budget_with_schedule_object, scorum::chain::budget_with_schedule_index )

FC_REFLECT_ENUM( scorum::chain::budget_schedule_object::budget_schedule_type, (unconditional)(by_time_range)(by_period) )

FC_REFLECT( scorum::chain::budget_schedule_object,
             (id)(schedule_alg)(start_date)(end_date)(period)(locked)
)
CHAINBASE_SET_INDEX_TYPE( scorum::chain::budget_schedule_object, scorum::chain::budget_schedule_index )
