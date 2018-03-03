#include <tuple>
#include <scorum/chain/services/vesting_delegation.hpp>
#include <scorum/chain/database/database.hpp>
#include <scorum/chain/schema/account_objects.hpp>
#include <scorum/chain/services/dynamic_global_property.hpp>

using namespace scorum::protocol;

namespace scorum {
namespace chain {

dbs_vesting_delegation::dbs_vesting_delegation(database& db)
    : base_service_type(db)
{
}

const vesting_delegation_object& dbs_vesting_delegation::get(const account_name_type& delegator,
                                                             const account_name_type& delegatee) const
{
    try
    {
        return get_by<by_delegation>(boost::make_tuple(delegator, delegatee));
    }
    FC_CAPTURE_AND_RETHROW((delegator)(delegatee))
}

bool dbs_vesting_delegation::is_exists(const account_name_type& delegator, const account_name_type& delegatee) const
{
    return find_by<by_delegation>(boost::make_tuple(delegator, delegatee));
}

const vesting_delegation_expiration_object& dbs_vesting_delegation::create_expiration(
    const account_name_type& delegator, const asset& vesting_shares, const time_point_sec& expiration)
{
    const auto& new_vd
        = db_impl().create<vesting_delegation_expiration_object>([&](vesting_delegation_expiration_object& vd) {
              vd.delegator = delegator;
              vd.vesting_shares = vesting_shares;
              vd.expiration = expiration;
          });
    return new_vd;
}

} // namespace chain
} // namespace scorum
