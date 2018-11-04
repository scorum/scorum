#include <scorum/chain/services/decline_voting_rights_request.hpp>
#include <scorum/chain/database/database.hpp>
#include <scorum/chain/schema/scorum_objects.hpp>
#include <scorum/chain/services/dynamic_global_property.hpp>
#include <tuple>

using namespace scorum::protocol;

namespace scorum {
namespace chain {

dbs_decline_voting_rights_request::dbs_decline_voting_rights_request(database& db)
    : base_service_type(db)
    , _dgp_svc(db.dynamic_global_property_service())
{
}

const decline_voting_rights_request_object&
dbs_decline_voting_rights_request::get(const account_id_type& account_id) const
{
    try
    {
        return get_by<by_account>(account_id);
    }
    FC_CAPTURE_AND_RETHROW((account_id))
}

bool dbs_decline_voting_rights_request::is_exists(const account_id_type& account_id) const
{
    return find_by<by_account>(account_id) != nullptr;
}

const decline_voting_rights_request_object&
dbs_decline_voting_rights_request::create_rights(const account_id_type& account, const fc::microseconds& time_to_life)
{
    const auto& new_object = create([&](decline_voting_rights_request_object& req) {
        req.account = account;
        req.effective_date = _dgp_svc.head_block_time() + time_to_life;
    });

    return new_object;
}

} // namespace chain
} // namespace scorum
