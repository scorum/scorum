#include <scorum/chain/services/decline_voting_rights_request.hpp>
#include <scorum/chain/database.hpp>

#include <tuple>

using namespace scorum::protocol;

namespace scorum {
namespace chain {

dbs_decline_voting_rights_request::dbs_decline_voting_rights_request(database& db)
    : _base_type(db)
{
}

const decline_voting_rights_request_object&
dbs_decline_voting_rights_request::get(const account_id_type& account_id) const
{
    try
    {
        return db_impl().get<decline_voting_rights_request_object, by_account>(account_id);
    }
    FC_CAPTURE_AND_RETHROW((account_id))
}

bool dbs_decline_voting_rights_request::is_exists(const account_id_type& account_id) const
{
    return nullptr != db_impl().find<decline_voting_rights_request_object, by_account>(account_id);
}

const decline_voting_rights_request_object&
dbs_decline_voting_rights_request::create(const account_id_type& account, const fc::microseconds& time_to_life)
{
    const dynamic_global_property_object& props = db_impl().get_dynamic_global_properties();

    const auto& new_object
        = db_impl().create<decline_voting_rights_request_object>([&](decline_voting_rights_request_object& req) {
              req.account = account;
              req.effective_date = props.time + time_to_life;
          });

    return new_object;
}

void dbs_decline_voting_rights_request::remove(const decline_voting_rights_request_object& req)
{
    db_impl().remove(req);
}

} // namespace chain
} // namespace scorum
