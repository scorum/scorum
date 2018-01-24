#include <scorum/chain/services/withdraw_vesting_route.hpp>

#include <scorum/chain/database.hpp>

#include <scorum/chain/schema/witness_objects.hpp>
#include <scorum/chain/schema/scorum_objects.hpp>

namespace scorum {
namespace chain {

dbs_withdraw_vesting_route::dbs_withdraw_vesting_route(database& db)
    : dbs_base(db)
{
}

bool dbs_withdraw_vesting_route::is_exists(account_id_type from, account_id_type to) const
{
    const auto* withdraw
        = db_impl().find<withdraw_vesting_route_object, by_withdraw_route>(boost::make_tuple(from, to));

    return withdraw == nullptr ? false : true;
}

const withdraw_vesting_route_object& dbs_withdraw_vesting_route::get(account_id_type from, account_id_type to) const
{
    try
    {
        return db_impl().get<withdraw_vesting_route_object, by_withdraw_route>(boost::make_tuple(from, to));
    }
    FC_CAPTURE_AND_RETHROW((from)(to))
}

void dbs_withdraw_vesting_route::remove(const withdraw_vesting_route_object& obj)
{
    db_impl().remove(obj);
}

void dbs_withdraw_vesting_route::create(account_id_type from, account_id_type to, uint16_t percent, bool auto_vest)
{
    db_impl().create<withdraw_vesting_route_object>([&](withdraw_vesting_route_object& wvdo) {
        wvdo.from_account = from;
        wvdo.to_account = to;
        wvdo.percent = percent;
        wvdo.auto_vest = auto_vest;
    });
}

void dbs_withdraw_vesting_route::update(const withdraw_vesting_route_object& obj,
                                        account_id_type from,
                                        account_id_type to,
                                        uint16_t percent,
                                        bool auto_vest)
{
    db_impl().modify(obj, [&](withdraw_vesting_route_object& wvdo) {
        wvdo.from_account = from;
        wvdo.to_account = to;
        wvdo.percent = percent;
        wvdo.auto_vest = auto_vest;
    });
}

uint16_t dbs_withdraw_vesting_route::total_percent(account_id_type from) const
{
    const auto& wd_idx = db_impl().get_index<withdraw_vesting_route_index>().indices().get<by_withdraw_route>();

    auto itr = wd_idx.upper_bound(boost::make_tuple(from, account_id_type()));
    uint16_t total_percent = 0;

    while (itr->from_account == from && itr != wd_idx.end())
    {
        total_percent += itr->percent;
        ++itr;
    }

    return total_percent;
}

} // namespace chain
} // namespace scorum
