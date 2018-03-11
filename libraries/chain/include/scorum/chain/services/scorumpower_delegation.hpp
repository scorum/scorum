#pragma once

#include <scorum/chain/services/dbs_base.hpp>

namespace scorum {
namespace chain {

class scorumpower_delegation_object;
class scorumpower_delegation_expiration_object;

struct scorumpower_delegation_service_i
{
    virtual const scorumpower_delegation_object& get(const account_name_type& delegator,
                                                     const account_name_type& delegatee) const = 0;

    virtual bool is_exists(const account_name_type& delegator, const account_name_type& delegatee) const = 0;

    virtual const scorumpower_delegation_object&
    create(const account_name_type& delegator, const account_name_type& delegatee, const asset& scorumpower)
        = 0;

    virtual const scorumpower_delegation_expiration_object&
    create_expiration(const account_name_type& delegator, const asset& scorumpower, const time_point_sec& expiration)
        = 0;

    virtual void update(const scorumpower_delegation_object& vd, const asset& scorumpower) = 0;

    virtual void remove(const scorumpower_delegation_object& vd) = 0;
};

class dbs_scorumpower_delegation : public dbs_base, public scorumpower_delegation_service_i
{
    friend class dbservice_dbs_factory;

protected:
    explicit dbs_scorumpower_delegation(database& db);

public:
    const scorumpower_delegation_object& get(const account_name_type& delegator,
                                             const account_name_type& delegatee) const override;

    bool is_exists(const account_name_type& delegator, const account_name_type& delegatee) const override;

    const scorumpower_delegation_object&
    create(const account_name_type& delegator, const account_name_type& delegatee, const asset& scorumpower) override;

    const scorumpower_delegation_expiration_object& create_expiration(const account_name_type& delegator,
                                                                      const asset& scorumpower,
                                                                      const time_point_sec& expiration) override;

    void update(const scorumpower_delegation_object& vd, const asset& scorumpower) override;

    void remove(const scorumpower_delegation_object& vd) override;
};
} // namespace chain
} // namespace scorum
