#pragma once

#include <scorum/chain/services/base_service.hpp>

namespace scorum {
namespace chain {

class vesting_delegation_object;
class vesting_delegation_expiration_object;

struct vesting_delegation_service_i
{
    virtual const vesting_delegation_object& get(const account_name_type& delegator,
                                                 const account_name_type& delegatee) const = 0;

    virtual bool is_exists(const account_name_type& delegator, const account_name_type& delegatee) const = 0;

    virtual const vesting_delegation_object&
    create(const account_name_type& delegator, const account_name_type& delegatee, const asset& vesting_shares)
        = 0;

    virtual const vesting_delegation_expiration_object&
    create_expiration(const account_name_type& delegator, const asset& vesting_shares, const time_point_sec& expiration)
        = 0;

    virtual void update(const vesting_delegation_object& vd, const asset& vesting_shares) = 0;

    virtual void remove(const vesting_delegation_object& vd) = 0;
};

class dbs_vesting_delegation : public dbs_base, public vesting_delegation_service_i
{
    friend class dbservice_dbs_factory;

protected:
    explicit dbs_vesting_delegation(database& db);

public:
    const vesting_delegation_object& get(const account_name_type& delegator,
                                         const account_name_type& delegatee) const override;

    bool is_exists(const account_name_type& delegator, const account_name_type& delegatee) const override;

    const vesting_delegation_object& create(const account_name_type& delegator,
                                            const account_name_type& delegatee,
                                            const asset& vesting_shares) override;

    const vesting_delegation_expiration_object& create_expiration(const account_name_type& delegator,
                                                                  const asset& vesting_shares,
                                                                  const time_point_sec& expiration) override;

    void update(const vesting_delegation_object& vd, const asset& vesting_shares) override;

    void remove(const vesting_delegation_object& vd) override;
};
} // namespace chain
} // namespace scorum
