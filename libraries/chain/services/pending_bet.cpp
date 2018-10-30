#include <scorum/chain/services/pending_bet.hpp>
#include <boost/lambda/lambda.hpp>

namespace scorum {
namespace chain {

dbs_pending_bet::dbs_pending_bet(database& db)
    : base_service_type(db)
{
}


bool dbs_pending_bet::is_exists(const pending_bet_id_type& id) const
{
    return find_by<by_id>(id) != nullptr;
}

bool dbs_pending_bet::is_exists(const uuid_type& uuid) const
{
    return find_by<by_uuid>(uuid) != nullptr;
}

const pending_bet_object& dbs_pending_bet::get_pending_bet(const pending_bet_id_type& obj_id) const
{
    try
    {
        return get_by<by_id>(obj_id);
    }
    FC_CAPTURE_LOG_AND_RETHROW((obj_id))
}

const pending_bet_object& dbs_pending_bet::get_pending_bet(const uuid_type& uuid) const
{
    try
    {
        return get_by<by_uuid>(uuid);
    }
    FC_CAPTURE_LOG_AND_RETHROW((uuid))
}

dbs_pending_bet::view_type dbs_pending_bet::get_bets(pending_bet_id_type lower_bound) const
{
    auto& idx = db_impl().get_index<pending_bet_index, by_id>();
    return { idx.lower_bound(lower_bound), idx.end() };
}

} // namespace chain
} // namespace scorum
