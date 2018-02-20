#include <scorum/chain/services/withdraw_vesting_route.hpp>

#include <scorum/chain/database.hpp>

#include <scorum/chain/schema/withdraw_vesting_route_objects.hpp>
#include <scorum/chain/schema/account_objects.hpp>
#include <scorum/chain/schema/dev_committee_object.hpp>

namespace scorum {
namespace chain {

using namespace withdraw_route_service;

class dbs_withdraw_vesting_route_impl : public dbs_base
{
public:
    dbs_withdraw_vesting_route_impl(database& db)
        : dbs_base(db)
    {
    }

    template <typename FromIdType, typename ToIdType> bool is_exists(const FromIdType& from, const ToIdType& to) const
    {
        return nullptr
            != db_impl().find<withdraw_vesting_route_object, by_withdraw_route>(
                   boost::make_tuple(withdrawable_id_type(from), withdrawable_id_type(to)));
    }

    template <typename FromIdType, typename ToIdType>
    const withdraw_vesting_route_object& get(const FromIdType& from, const ToIdType& to) const
    {
        try
        {
            return db_impl().get<withdraw_vesting_route_object, by_withdraw_route>(
                boost::make_tuple(withdrawable_id_type(from), withdrawable_id_type(to)));
        }
        FC_CAPTURE_AND_RETHROW((from)(to))
    }

    template <typename FromIdType> uint16_t total_percent(const FromIdType& from) const
    {
        const auto& wd_idx = db_impl().get_index<withdraw_vesting_route_index>().indices().get<by_withdraw_route>();

        withdrawable_id_type from_id = from;
        auto itr = wd_idx.upper_bound(boost::make_tuple(from_id, withdrawable_id_type()));
        uint16_t total_percent = 0;

        while (is_equal_withdrawable_id(itr->from_id, from_id) && itr != wd_idx.end())
        {
            total_percent += itr->percent;
            ++itr;
        }

        return total_percent;
    }
};

dbs_withdraw_vesting_route::dbs_withdraw_vesting_route(database& db)
    : dbs_base(db)
    , _impl(new dbs_withdraw_vesting_route_impl(db))
{
}

dbs_withdraw_vesting_route::~dbs_withdraw_vesting_route()
{
}

bool dbs_withdraw_vesting_route::is_exists(account_id_type from, account_id_type to) const
{
    return _impl->is_exists(from, to);
}

bool dbs_withdraw_vesting_route::is_exists(account_id_type from, dev_committee_id_type to) const
{
    return _impl->is_exists(from, to);
}

bool dbs_withdraw_vesting_route::is_exists(dev_committee_id_type from, dev_committee_id_type to) const
{
    return _impl->is_exists(from, to);
}

const withdraw_vesting_route_object& dbs_withdraw_vesting_route::get(account_id_type from, account_id_type to) const
{
    return _impl->get(from, to);
}

const withdraw_vesting_route_object& dbs_withdraw_vesting_route::get(account_id_type from,
                                                                     dev_committee_id_type to) const
{
    return _impl->get(from, to);
}

const withdraw_vesting_route_object& dbs_withdraw_vesting_route::get(dev_committee_id_type from,
                                                                     dev_committee_id_type to) const
{
    return _impl->get(from, to);
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
    return _impl->total_percent(from);
}

uint16_t dbs_withdraw_vesting_route::total_percent(dev_committee_id_type from) const
{
    return _impl->total_percent(from);
}

//

namespace withdraw_route_service {

bool is_from_account(const withdraw_vesting_route_object& obj)
{
    return withdrawable_id_is_account_id(obj.from_id);
}

bool is_to_account(const withdraw_vesting_route_object& obj)
{
    return withdrawable_id_is_account_id(obj.to_id);
}

bool is_from_dev_committee(const withdraw_vesting_route_object& obj)
{
    return withdrawable_id_is_dev_committee_id(obj.from_id);
}

bool is_to_dev_committee(const withdraw_vesting_route_object& obj)
{
    return withdrawable_id_is_dev_committee_id(obj.to_id);
}

} // namespace withdraw_route_service

} // namespace chain
} // namespace scorum
