#include <scorum/chain/services/withdraw_vesting_route.hpp>

#include <scorum/chain/database/database.hpp>

#include <scorum/chain/schema/withdraw_vesting_objects.hpp>
#include <scorum/chain/schema/account_objects.hpp>
#include <scorum/chain/schema/dev_committee_object.hpp>

namespace scorum {
namespace chain {

class dbs_withdraw_vesting_route_impl : public dbs_base
{
public:
    dbs_withdraw_vesting_route_impl(database& db)
        : dbs_base(db)
    {
    }

    template <typename FromIdType>
    dbs_withdraw_vesting_route::withdraw_vesting_route_refs_type get_all(const FromIdType& from) const
    {
        dbs_withdraw_vesting_route::withdraw_vesting_route_refs_type ret;

        const auto& idx = db_impl().get_index<withdraw_vesting_route_index>().indices().get<by_withdraw_route>();

        withdrawable_id_type from_id = from;
        auto it = idx.upper_bound(boost::make_tuple(from_id, withdrawable_id_type()));
        const auto it_end = idx.cend();
        while (is_equal_withdrawable_id(it->from_id, from_id) && it != it_end)
        {
            ret.push_back(std::cref(*it));
            ++it;
        }

        return ret;
    }

    template <typename FromIdType> uint16_t total_percent(const FromIdType& from) const
    {
        uint16_t total_percent = 0;
        for (const withdraw_vesting_route_object& wvro : get_all(from))
        {
            total_percent += wvro.percent;
        }
        return total_percent;
    }
};

dbs_withdraw_vesting_route::dbs_withdraw_vesting_route(database& db)
    : base_service_type(db)
    , _impl(new dbs_withdraw_vesting_route_impl(db))
{
}

dbs_withdraw_vesting_route::~dbs_withdraw_vesting_route()
{
}

bool dbs_withdraw_vesting_route::is_exists(const account_id_type& from, const account_id_type& to) const
{
    return find_by<by_withdraw_route>(boost::make_tuple(withdrawable_id_type(from), withdrawable_id_type(to)));
}

bool dbs_withdraw_vesting_route::is_exists(const account_id_type& from, const dev_committee_id_type& to) const
{
    return find_by<by_withdraw_route>(boost::make_tuple(withdrawable_id_type(from), withdrawable_id_type(to)));
}

bool dbs_withdraw_vesting_route::is_exists(const dev_committee_id_type& from, const dev_committee_id_type& to) const
{
    return find_by<by_withdraw_route>(boost::make_tuple(withdrawable_id_type(from), withdrawable_id_type(to)));
}

bool dbs_withdraw_vesting_route::is_exists(const dev_committee_id_type& from, const account_id_type& to) const
{
    return find_by<by_withdraw_route>(boost::make_tuple(withdrawable_id_type(from), withdrawable_id_type(to)));
}

const withdraw_vesting_route_object& dbs_withdraw_vesting_route::get(const account_id_type& from,
                                                                     const account_id_type& to) const
{
    try
    {
        return get_by<by_withdraw_route>(boost::make_tuple(withdrawable_id_type(from), withdrawable_id_type(to)));
    }
    FC_CAPTURE_AND_RETHROW((from)(to))
}

const withdraw_vesting_route_object& dbs_withdraw_vesting_route::get(const account_id_type& from,
                                                                     const dev_committee_id_type& to) const
{
    try
    {
        return get_by<by_withdraw_route>(boost::make_tuple(withdrawable_id_type(from), withdrawable_id_type(to)));
    }
    FC_CAPTURE_AND_RETHROW((from)(to))
}

const withdraw_vesting_route_object& dbs_withdraw_vesting_route::get(const dev_committee_id_type& from,
                                                                     const dev_committee_id_type& to) const
{
    try
    {
        return get_by<by_withdraw_route>(boost::make_tuple(withdrawable_id_type(from), withdrawable_id_type(to)));
    }
    FC_CAPTURE_AND_RETHROW((from)(to))
}

const withdraw_vesting_route_object& dbs_withdraw_vesting_route::get(const dev_committee_id_type& from,
                                                                     const account_id_type& to) const
{
    try
    {
        return get_by<by_withdraw_route>(boost::make_tuple(withdrawable_id_type(from), withdrawable_id_type(to)));
    }
    FC_CAPTURE_AND_RETHROW((from)(to))
}

dbs_withdraw_vesting_route::withdraw_vesting_route_refs_type
dbs_withdraw_vesting_route::get_all(const withdrawable_id_type& from) const
{
    return _impl->get_all(from);
}

uint16_t dbs_withdraw_vesting_route::total_percent(const account_id_type& from) const
{
    return _impl->total_percent(from);
}

uint16_t dbs_withdraw_vesting_route::total_percent(const dev_committee_id_type& from) const
{
    return _impl->total_percent(from);
}

} // namespace chain
} // namespace scorum
