#pragma once
#include <boost/multi_index/composite_key.hpp>

#include <scorum/chain/schema/scorum_object_types.hpp>

#include <fc/exception/exception.hpp>
#include <functional>

namespace scorum {
namespace chain {

class withdraw_vesting_route_object : public object<withdraw_vesting_route_object_type, withdraw_vesting_route_object>
{
public:
    CHAINBASE_DEFAULT_CONSTRUCTOR(withdraw_vesting_route_object)

    id_type id;

    withdrawable_id_type from_id;
    withdrawable_id_type to_id;

    uint16_t percent = 0;
    bool auto_vest = false;
};

class withdraw_vesting_route_statistic_object
    : public object<withdraw_vesting_route_statistic_object_type, withdraw_vesting_route_statistic_object>
{
public:
    CHAINBASE_DEFAULT_CONSTRUCTOR(withdraw_vesting_route_statistic_object)

    id_type id;

    withdrawable_id_type from_id;

    uint16_t withdraw_routes = 0u;
};

using scorum::protocol::asset;

class withdraw_vesting_object : public object<withdraw_vesting_object_type, withdraw_vesting_object>
{
public:
    CHAINBASE_DEFAULT_CONSTRUCTOR(withdraw_vesting_object)

    id_type id;

    withdrawable_id_type from_id;

    asset vesting_withdraw_rate
        = asset(0, SP_SYMBOL); ///< at the time this is updated it can be at most scorumpower/104
    time_point_sec next_vesting_withdrawal
        = fc::time_point_sec::maximum(); ///< after every withdrawal this is incremented by 1 week
    asset withdrawn = asset(0, SP_SYMBOL); /// Track how many shares have been withdrawn
    asset to_withdraw = asset(0, SP_SYMBOL); /// Might be able to look this up with operation history.
};

namespace withdraw_vesting_objects {

class ids_visitor
{
public:
    typedef bool result_type;

    template <typename Op> bool operator()(const Op&) const
    {
        FC_ASSERT(false, "Unknown type.");
        return false;
    }

protected:
    ids_visitor(const withdrawable_id_type& right)
        : _right(right)
    {
    }

    const withdrawable_id_type& _right;
};

class less_ids_visitor : public ids_visitor
{
public:
    less_ids_visitor(const withdrawable_id_type& right)
        : ids_visitor(right)
    {
    }

    bool operator()(const account_id_type& left) const
    {
        return left < _right.get<account_id_type>();
    }

    bool operator()(const dev_committee_id_type& left) const
    {
        return left < _right.get<dev_committee_id_type>();
    }
};

class equal_ids_visitor : public ids_visitor
{
public:
    equal_ids_visitor(const withdrawable_id_type& right)
        : ids_visitor(right)
    {
    }

    bool operator()(const account_id_type& left) const
    {
        return _right.get<account_id_type>() == left;
    }

    bool operator()(const dev_committee_id_type& left) const
    {
        return _right.get<dev_committee_id_type>() == left;
    }
};
}

struct less_for_withdrawable_id : public std::binary_function<withdrawable_id_type, withdrawable_id_type, bool>
{
    bool operator()(const withdrawable_id_type& a, const withdrawable_id_type& b) const
    {
        using namespace withdraw_vesting_objects;

        if (a.which() == b.which()) // both have same type
        {
            return a.visit(less_ids_visitor(b));
        }
        return a.which() < b.which(); // compare by type
    }
};

inline bool is_equal_withdrawable_id(const withdrawable_id_type& a, const withdrawable_id_type& b)
{
    using namespace withdraw_vesting_objects;

    if (a.which() == b.which())
    {
        return a.visit(equal_ids_visitor(b));
    }

    return false;
}

struct by_withdraw_route;
struct by_destination;
typedef shared_multi_index_container<withdraw_vesting_route_object,
                                     indexed_by<ordered_unique<tag<by_id>,
                                                               member<withdraw_vesting_route_object,
                                                                      withdraw_vesting_route_id_type,
                                                                      &withdraw_vesting_route_object::id>>,
                                                ordered_unique<tag<by_withdraw_route>,
                                                               composite_key<withdraw_vesting_route_object,
                                                                             member<withdraw_vesting_route_object,
                                                                                    withdrawable_id_type,
                                                                                    &withdraw_vesting_route_object::
                                                                                        from_id>,
                                                                             member<withdraw_vesting_route_object,
                                                                                    withdrawable_id_type,
                                                                                    &withdraw_vesting_route_object::
                                                                                        to_id>>,
                                                               composite_key_compare<less_for_withdrawable_id,
                                                                                     less_for_withdrawable_id>>,
                                                ordered_unique<tag<by_destination>,
                                                               composite_key<withdraw_vesting_route_object,
                                                                             member<withdraw_vesting_route_object,
                                                                                    withdrawable_id_type,
                                                                                    &withdraw_vesting_route_object::
                                                                                        to_id>,
                                                                             member<withdraw_vesting_route_object,
                                                                                    withdraw_vesting_route_id_type,
                                                                                    &withdraw_vesting_route_object::
                                                                                        id>>,
                                                               composite_key_compare<less_for_withdrawable_id,
                                                                                     std::
                                                                                         less<withdraw_vesting_route_id_type>>>>>
    withdraw_vesting_route_index;

typedef shared_multi_index_container<withdraw_vesting_route_statistic_object,
                                     indexed_by<ordered_unique<tag<by_id>,
                                                               member<withdraw_vesting_route_statistic_object,
                                                                      withdraw_vesting_route_statistic_id_type,
                                                                      &withdraw_vesting_route_statistic_object::id>>,
                                                ordered_unique<tag<by_destination>,
                                                               member<withdraw_vesting_route_statistic_object,
                                                                      withdrawable_id_type,
                                                                      &withdraw_vesting_route_statistic_object::
                                                                          from_id>,
                                                               less_for_withdrawable_id>>>
    withdraw_vesting_route_statistic_index;

struct by_next_vesting_withdrawal;
typedef shared_multi_index_container<withdraw_vesting_object,
                                     indexed_by<ordered_unique<tag<by_id>,
                                                               member<withdraw_vesting_object,
                                                                      withdraw_vesting_id_type,
                                                                      &withdraw_vesting_object::id>>,
                                                ordered_unique<tag<by_destination>,
                                                               member<withdraw_vesting_object,
                                                                      withdrawable_id_type,
                                                                      &withdraw_vesting_object::from_id>,
                                                               less_for_withdrawable_id>,
                                                ordered_unique<tag<by_next_vesting_withdrawal>,
                                                               composite_key<withdraw_vesting_object,
                                                                             member<withdraw_vesting_object,
                                                                                    time_point_sec,
                                                                                    &withdraw_vesting_object::
                                                                                        next_vesting_withdrawal>,
                                                                             member<withdraw_vesting_object,
                                                                                    withdraw_vesting_id_type,
                                                                                    &withdraw_vesting_object::id>>>>>
    withdraw_vesting_index;

} // namespace chain
} // namespace scorum

// clang-format off
FC_REFLECT_TYPENAME(scorum::chain::withdrawable_id_type)
FC_REFLECT(scorum::chain::withdraw_vesting_route_object,
           (id)
           (from_id)
           (to_id)
           (percent)
           (auto_vest))
CHAINBASE_SET_INDEX_TYPE(scorum::chain::withdraw_vesting_route_object, scorum::chain::withdraw_vesting_route_index)

FC_REFLECT(scorum::chain::withdraw_vesting_route_statistic_object,
           (id)
           (from_id)
           (withdraw_routes))
CHAINBASE_SET_INDEX_TYPE(scorum::chain::withdraw_vesting_route_statistic_object, scorum::chain::withdraw_vesting_route_statistic_index)

FC_REFLECT(scorum::chain::withdraw_vesting_object,
           (id)
           (from_id)
           (vesting_withdraw_rate)
           (next_vesting_withdrawal)
           (withdrawn)
           (to_withdraw))
CHAINBASE_SET_INDEX_TYPE(scorum::chain::withdraw_vesting_object, scorum::chain::withdraw_vesting_index)
// clang-format on
