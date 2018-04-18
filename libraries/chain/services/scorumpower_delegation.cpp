#include <tuple>
#include <scorum/chain/services/scorumpower_delegation.hpp>
#include <scorum/chain/services/dynamic_global_property.hpp>
#include <scorum/chain/database/database.hpp>
#include <scorum/chain/schema/account_objects.hpp>

using namespace scorum::protocol;

namespace scorum {
namespace chain {

dbs_scorumpower_delegation::dbs_scorumpower_delegation(database& db)
    : base_service_type(db)
{
}

const scorumpower_delegation_object& dbs_scorumpower_delegation::get(const account_name_type& delegator,
                                                                     const account_name_type& delegatee) const
{
    try
    {
        return db_impl().get<scorumpower_delegation_object, by_delegation>(boost::make_tuple(delegator, delegatee));
    }
    FC_CAPTURE_AND_RETHROW((delegator)(delegatee))
}

bool dbs_scorumpower_delegation::is_exists(const account_name_type& delegator, const account_name_type& delegatee) const
{
    return find_by<by_delegation>(boost::make_tuple(delegator, delegatee));
}

const scorumpower_delegation_expiration_object& dbs_scorumpower_delegation::create_expiration(
    const account_name_type& delegator, const asset& scorumpower, const time_point_sec& expiration)
{
    const auto& new_o
        = db_impl().create<scorumpower_delegation_expiration_object>([&](scorumpower_delegation_expiration_object& o) {
              o.delegator = delegator;
              o.scorumpower = scorumpower;
              o.expiration = expiration;
          });
    return new_o;
}

} // namespace chain
} // namespace scorum
