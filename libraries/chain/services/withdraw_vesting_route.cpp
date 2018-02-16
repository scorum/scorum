#include <scorum/chain/services/withdraw_vesting_route.hpp>

#include <scorum/chain/database.hpp>

#include <scorum/chain/schema/withdraw_vesting_route_object.hpp>
#include <scorum/chain/schema/account_objects.hpp>
#include <scorum/chain/schema/dev_committee_object.hpp>

namespace scorum {
namespace chain {

using namespace withdraw_route_service;

dbs_withdraw_vesting_route::dbs_withdraw_vesting_route(database& db)
    : dbs_base(db)
{
}

bool dbs_withdraw_vesting_route::is_exists(account_id_type from, account_id_type to) const
{
    return nullptr
        != db_impl().find<withdraw_vesting_route_object, by_withdraw_route>(boost::make_tuple(from, get_to_id(to)));
}

bool dbs_withdraw_vesting_route::is_exists(account_id_type from, dev_committee_id_type to) const
{
    return nullptr
        != db_impl().find<withdraw_vesting_route_object, by_withdraw_route>(boost::make_tuple(from, get_to_id(to)));
}

const withdraw_vesting_route_object& dbs_withdraw_vesting_route::get(account_id_type from, account_id_type to) const
{
    try
    {
        return db_impl().get<withdraw_vesting_route_object, by_withdraw_route>(boost::make_tuple(from, get_to_id(to)));
    }
    FC_CAPTURE_AND_RETHROW((from)(to))
}

const withdraw_vesting_route_object& dbs_withdraw_vesting_route::get(account_id_type from,
                                                                     dev_committee_id_type to) const
{
    try
    {
        return db_impl().get<withdraw_vesting_route_object, by_withdraw_route>(boost::make_tuple(from, get_to_id(to)));
    }
    FC_CAPTURE_AND_RETHROW((from)(to))
}

void dbs_withdraw_vesting_route::remove(const withdraw_vesting_route_object& obj)
{
    db_impl().remove(obj);
}

const withdraw_vesting_route_object& dbs_withdraw_vesting_route::create(const modifier_type& modifier)
{
    return db_impl().create<withdraw_vesting_route_object>([&](withdraw_vesting_route_object& o) { modifier(o); });
}

void dbs_withdraw_vesting_route::update(const withdraw_vesting_route_object& obj, const modifier_type& modifier)
{
    db_impl().modify(obj, [&](withdraw_vesting_route_object& o) { modifier(o); });
}

uint16_t dbs_withdraw_vesting_route::total_percent(account_id_type from) const
{
    const auto& wd_idx = db_impl().get_index<withdraw_vesting_route_index>().indices().get<by_withdraw_route>();

    auto itr = wd_idx.upper_bound(boost::make_tuple(from, withdraw_vesting_route_object_to_id_type()));
    uint16_t total_percent = 0;

    while (itr->from_account == from && itr != wd_idx.end())
    {
        total_percent += itr->percent;
        ++itr;
    }

    return total_percent;
}

namespace withdraw_route_service {

uint128_t get_to_id(const account_id_type& id)
{
    return get_withdraw_vesting_route_to_id(id, account_object_type);
}

uint128_t get_to_id(const account_object& object)
{
    return get_to_id(object.id);
}

uint128_t get_to_id(const dev_committee_id_type& id)
{
    return get_withdraw_vesting_route_to_id(id, dev_committee_object_type);
}

uint128_t get_to_id(const dev_committee_object& object)
{
    return get_to_id(object.id);
}

bool is_to_account(const withdraw_vesting_route_object& obj)
{
    return get_withdraw_vesting_route_object_type(obj.to_id) == account_object_type;
}

bool is_to_dev_committee(const withdraw_vesting_route_object& obj)
{
    return get_withdraw_vesting_route_object_type(obj.to_id) == dev_committee_object_type;
}

} // namespace withdraw_route_service

} // namespace chain
} // namespace scorum
