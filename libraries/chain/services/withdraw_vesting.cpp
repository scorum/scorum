#include <scorum/chain/services/withdraw_vesting.hpp>

#include <scorum/chain/database/database.hpp>

#include <scorum/chain/schema/withdraw_vesting_objects.hpp>

namespace scorum {
namespace chain {

dbs_withdraw_vesting::dbs_withdraw_vesting(database& db)
    : base_service_type(db)
{
}

dbs_withdraw_vesting::~dbs_withdraw_vesting()
{
}

bool dbs_withdraw_vesting::is_exists(const account_id_type& from) const
{
    return find_by<by_destination>(from);
}

bool dbs_withdraw_vesting::is_exists(const dev_committee_id_type& from) const
{
    return find_by<by_destination>(from);
}

const withdraw_vesting_object& dbs_withdraw_vesting::get(const account_id_type& from) const
{
    try
    {
        return get_by<by_destination>(from);
    }
    FC_CAPTURE_AND_RETHROW((from))
}

const withdraw_vesting_object& dbs_withdraw_vesting::get(const dev_committee_id_type& from) const
{
    try
    {
        return get_by<by_destination>(from);
    }
    FC_CAPTURE_AND_RETHROW((from))
}

dbs_withdraw_vesting::withdraw_vesting_refs_type dbs_withdraw_vesting::get_until(const time_point_sec& until) const
{
    withdraw_vesting_refs_type ret;

    const auto& idx = db_impl().get_index<withdraw_vesting_index>().indices().get<by_next_vesting_withdrawal>();
    auto it = idx.cbegin();
    const auto it_end = idx.cend();
    while (it != it_end && it->next_vesting_withdrawal <= until)
    {
        ret.push_back(std::cref(*it));
        ++it;
    }

    return ret;
}

asset dbs_withdraw_vesting::get_withdraw_rest(const account_id_type& from) const
{
    if (!is_exists(from))
        return asset(0, VESTS_SYMBOL);
    const withdraw_vesting_object& wvo = get(from);
    return wvo.to_withdraw - wvo.withdrawn;
}

} // namespace chain
} // namespace scorum
