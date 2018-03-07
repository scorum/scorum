#include <scorum/chain/services/scorumpower_delegation.hpp>
#include <scorum/chain/database/database.hpp>

#include <tuple>

#include <scorum/chain/schema/account_objects.hpp>

using namespace scorum::protocol;

namespace scorum {
namespace chain {

dbs_scorumpower_delegation::dbs_scorumpower_delegation(database& db)
    : _base_type(db)
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
    return nullptr
        != db_impl().find<scorumpower_delegation_object, by_delegation>(boost::make_tuple(delegator, delegatee));
}

const scorumpower_delegation_object& dbs_scorumpower_delegation::create(const account_name_type& delegator,
                                                                        const account_name_type& delegatee,
                                                                        const asset& scorumpower)
{
    const auto& props = db_impl().get_dynamic_global_properties();

    const auto& new_vd = db_impl().create<scorumpower_delegation_object>([&](scorumpower_delegation_object& vd) {
        vd.delegator = delegator;
        vd.delegatee = delegatee;
        vd.scorumpower = scorumpower;
        vd.min_delegation_time = props.time;
    });
    return new_vd;
}

const scorumpower_delegation_expiration_object& dbs_scorumpower_delegation::create_expiration(
    const account_name_type& delegator, const asset& scorumpower, const time_point_sec& expiration)
{
    const auto& new_vd
        = db_impl().create<scorumpower_delegation_expiration_object>([&](scorumpower_delegation_expiration_object& vd) {
              vd.delegator = delegator;
              vd.scorumpower = scorumpower;
              vd.expiration = expiration;
          });
    return new_vd;
}

void dbs_scorumpower_delegation::update(const scorumpower_delegation_object& vd, const asset& scorumpower)
{
    db_impl().modify(vd, [&](scorumpower_delegation_object& obj) { obj.scorumpower = scorumpower; });
}

void dbs_scorumpower_delegation::remove(const scorumpower_delegation_object& vd)
{
    db_impl().remove(vd);
}

} // namespace chain
} // namespace scorum
