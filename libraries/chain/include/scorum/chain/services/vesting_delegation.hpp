#pragma once

#include <scorum/chain/services/service_base.hpp>
#include <scorum/chain/schema/account_objects.hpp>

namespace scorum {
namespace chain {

class vesting_delegation_expiration_object;

struct vesting_delegation_service_i : public base_service_i<vesting_delegation_object>
{
    virtual const vesting_delegation_object& get(const account_name_type& delegator,
                                                 const account_name_type& delegatee) const = 0;

    virtual bool is_exists(const account_name_type& delegator, const account_name_type& delegatee) const = 0;

    virtual const vesting_delegation_expiration_object&
    create_expiration(const account_name_type& delegator, const asset& vesting_shares, const time_point_sec& expiration)
        = 0;
};

class dbs_vesting_delegation : public dbs_service_base<vesting_delegation_service_i>
{
    friend class dbservice_dbs_factory;

protected:
    explicit dbs_vesting_delegation(database& db);

public:
    const vesting_delegation_object& get(const account_name_type& delegator,
                                         const account_name_type& delegatee) const override;

    bool is_exists(const account_name_type& delegator, const account_name_type& delegatee) const override;

    const vesting_delegation_expiration_object& create_expiration(const account_name_type& delegator,
                                                                  const asset& vesting_shares,
                                                                  const time_point_sec& expiration) override;
};
} // namespace chain
} // namespace scorum
