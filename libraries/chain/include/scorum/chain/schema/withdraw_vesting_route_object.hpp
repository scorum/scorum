#pragma once
#include <boost/multi_index/composite_key.hpp>

#include <scorum/chain/schema/scorum_object_types.hpp>

#include <fc/exception/exception.hpp>
#include <limits>

namespace scorum {
namespace chain {

using withdraw_vesting_route_object_to_id_type = uint128_t;

class withdraw_vesting_route_object : public object<withdraw_vesting_route_object_type, withdraw_vesting_route_object>
{
public:
    CHAINBASE_DEFAULT_CONSTRUCTOR(withdraw_vesting_route_object)

    id_type id;

    account_id_type from_account;
    withdraw_vesting_route_object_to_id_type to_id; // combination of 'to object' object_type and his int64_t id (uniq)
    fc::variant to_object;

    uint16_t percent = 0;
    bool auto_vest = false;
};

template <typename ObjectIdType>
withdraw_vesting_route_object_to_id_type get_withdraw_vesting_route_to_id(const ObjectIdType& object_id,
                                                                          object_type type_id)
{
    withdraw_vesting_route_object_to_id_type to_id = 0;
    FC_ASSERT(sizeof(object_id) == sizeof(to_id) / 2);
    FC_ASSERT(type_id < std::numeric_limits<int64_t>::max());
    to_id.hi = object_id._id;
    to_id.lo = type_id;
    return to_id;
}

object_type get_withdraw_vesting_route_object_type(const withdraw_vesting_route_object_to_id_type& to_id);

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
                                                                                    account_id_type,
                                                                                    &withdraw_vesting_route_object::
                                                                                        from_account>,
                                                                             member<withdraw_vesting_route_object,
                                                                                    withdraw_vesting_route_object_to_id_type,
                                                                                    &withdraw_vesting_route_object::
                                                                                        to_id>>,
                                                               composite_key_compare<std::less<account_id_type>,
                                                                                     std::
                                                                                         less<withdraw_vesting_route_object_to_id_type>>>,
                                                ordered_unique<tag<by_destination>,
                                                               composite_key<withdraw_vesting_route_object,
                                                                             member<withdraw_vesting_route_object,
                                                                                    withdraw_vesting_route_object_to_id_type,
                                                                                    &withdraw_vesting_route_object::
                                                                                        to_id>,
                                                                             member<withdraw_vesting_route_object,
                                                                                    withdraw_vesting_route_id_type,
                                                                                    &withdraw_vesting_route_object::
                                                                                        id>>>>>
    withdraw_vesting_route_index;

} // namespace chain
} // namespace scorum

// clang-format off
FC_REFLECT(scorum::chain::withdraw_vesting_route_object,
           (id)
           (from_account)
           (to_id)
           (to_object)
           (percent)
           (auto_vest))
CHAINBASE_SET_INDEX_TYPE(scorum::chain::withdraw_vesting_route_object, scorum::chain::withdraw_vesting_route_index)
// clang-format on
