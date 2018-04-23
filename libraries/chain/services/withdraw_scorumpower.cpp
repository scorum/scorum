#include <scorum/chain/services/withdraw_scorumpower.hpp>

#include <scorum/chain/database/database.hpp>

#include <scorum/chain/schema/withdraw_scorumpower_objects.hpp>

namespace scorum {
namespace chain {

dbs_withdraw_scorumpower::dbs_withdraw_scorumpower(database& db)
    : base_service_type(db)
{
}

dbs_withdraw_scorumpower::~dbs_withdraw_scorumpower()
{
}

bool dbs_withdraw_scorumpower::is_exists(const account_id_type& from) const
{
    return find_by<by_destination>(from) != nullptr;
}

bool dbs_withdraw_scorumpower::is_exists(const dev_committee_id_type& from) const
{
    return find_by<by_destination>(from) != nullptr;
}

const withdraw_scorumpower_object& dbs_withdraw_scorumpower::get(const account_id_type& from) const
{
    try
    {
        return get_by<by_destination>(from);
    }
    FC_CAPTURE_AND_RETHROW((from))
}

const withdraw_scorumpower_object& dbs_withdraw_scorumpower::get(const dev_committee_id_type& from) const
{
    try
    {
        return get_by<by_destination>(from);
    }
    FC_CAPTURE_AND_RETHROW((from))
}

dbs_withdraw_scorumpower::withdraw_scorumpower_refs_type
dbs_withdraw_scorumpower::get_until(const time_point_sec& until) const
{
    withdraw_scorumpower_refs_type ret;

    const auto& idx = db_impl().get_index<withdraw_scorumpower_index>().indices().get<by_next_vesting_withdrawal>();
    auto it = idx.cbegin();
    const auto it_end = idx.cend();
    while (it != it_end && it->next_vesting_withdrawal <= until)
    {
        ret.push_back(std::cref(*it));
        ++it;
    }

    return ret;
}

asset dbs_withdraw_scorumpower::get_withdraw_rest(const account_id_type& from) const
{
    if (!is_exists(from))
        return asset(0, SP_SYMBOL);
    const withdraw_scorumpower_object& wvo = get(from);
    return wvo.to_withdraw - wvo.withdrawn;
}

} // namespace chain
} // namespace scorum
