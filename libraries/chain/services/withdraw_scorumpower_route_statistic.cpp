#include <scorum/chain/services/withdraw_scorumpower_route_statistic.hpp>
#include <scorum/chain/schema/withdraw_scorumpower_objects.hpp>

namespace scorum {
namespace chain {

dbs_withdraw_scorumpower_route_statistic::dbs_withdraw_scorumpower_route_statistic(database& db)
    : base_service_type(db)
{
}

dbs_withdraw_scorumpower_route_statistic::~dbs_withdraw_scorumpower_route_statistic()
{
}

bool dbs_withdraw_scorumpower_route_statistic::is_exists(const account_id_type& from) const
{
    return find_by<by_destination>(from);
}

bool dbs_withdraw_scorumpower_route_statistic::is_exists(const dev_committee_id_type& from) const
{
    return find_by<by_destination>(from);
}

const withdraw_scorumpower_route_statistic_object&
dbs_withdraw_scorumpower_route_statistic::get(const account_id_type& from) const
{
    try
    {
        return get_by<by_destination>(from);
    }
    FC_CAPTURE_AND_RETHROW((from))
}

const withdraw_scorumpower_route_statistic_object&
dbs_withdraw_scorumpower_route_statistic::get(const dev_committee_id_type& from) const
{
    try
    {
        return get_by<by_destination>(from);
    }
    FC_CAPTURE_AND_RETHROW((from))
}

} // namespace chain
} // namespace scorum
