#include <scorum/chain/services/pending_bet.hpp>
#include <boost/lambda/lambda.hpp>

namespace scorum {
namespace chain {

dbs_pending_bet::dbs_pending_bet(database& db)
    : base_service_type(db)
{
}

bool dbs_pending_bet::is_exists(const uuid_type& uuid) const
{
    return find_by<by_uuid>(uuid) != nullptr;
}

const pending_bet_object& dbs_pending_bet::get_pending_bet(const uuid_type& uuid) const
{
    try
    {
        return get_by<by_uuid>(uuid);
    }
    FC_CAPTURE_LOG_AND_RETHROW((uuid))
}

} // namespace chain
} // namespace scorum
