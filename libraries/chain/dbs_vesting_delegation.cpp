#include <scorum/chain/dbs_vesting_delegation.hpp>
#include <scorum/chain/database.hpp>

#include <tuple>

using namespace scorum::protocol;

namespace scorum {
namespace chain {

dbs_vesting_delegation::dbs_vesting_delegation(database& db)
    : _base_type(db)
{
}

const vesting_delegation_object& dbs_vesting_delegation::get(const account_name_type& delegator,
                                                             const account_name_type& delegatee) const
{
    try
    {
        return db_impl().get<vesting_delegation_object, by_delegation>(boost::make_tuple(delegator, delegatee));
    }
    FC_CAPTURE_AND_RETHROW((delegator)(delegatee))
}

bool dbs_vesting_delegation::is_exists(const account_name_type& delegator, const account_name_type& delegatee) const
{
    return nullptr != db_impl().find<vesting_delegation_object, by_delegation>(boost::make_tuple(delegator, delegatee));
}

const vesting_delegation_object& dbs_vesting_delegation::create(const account_name_type& delegator,
                                                                const account_name_type& delegatee,
                                                                const asset& vesting_shares)
{
    const auto& props = db_impl().get_dynamic_global_properties();

    const auto& new_vd = db_impl().create<vesting_delegation_object>([&](vesting_delegation_object& vd) {
        vd.delegator = delegator;
        vd.delegatee = delegatee;
        vd.vesting_shares = vesting_shares;
        vd.min_delegation_time = props.time;
    });
    return new_vd;
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

void dbs_vesting_delegation::update(const vesting_delegation_object& vd, const asset& vesting_shares)
{
    db_impl().modify(vd, [&](vesting_delegation_object& obj) { obj.vesting_shares = vesting_shares; });
}

void dbs_vesting_delegation::remove(const vesting_delegation_object& vd)
{
    db_impl().remove(vd);
}

} // namespace chain
} // namespace scorum
