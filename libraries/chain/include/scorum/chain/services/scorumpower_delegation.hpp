#pragma once

#include <scorum/chain/services/service_base.hpp>
#include <scorum/chain/schema/account_objects.hpp>

namespace scorum {
namespace chain {

class scorumpower_delegation_object;
class scorumpower_delegation_expiration_object;

struct scorumpower_delegation_service_i : public base_service_i<scorumpower_delegation_object>
{
    using base_service_i<scorumpower_delegation_object>::get;
    using base_service_i<scorumpower_delegation_object>::is_exists;

    virtual const scorumpower_delegation_object& get(const account_name_type& delegator,
                                                     const account_name_type& delegatee) const = 0;

    virtual bool is_exists(const account_name_type& delegator, const account_name_type& delegatee) const = 0;

    virtual const scorumpower_delegation_expiration_object&
    create_expiration(const account_name_type& delegator, const asset& scorumpower, const time_point_sec& expiration)
        = 0;
};

class dbs_scorumpower_delegation : public dbs_service_base<scorumpower_delegation_service_i>
{
    friend class dbservice_dbs_factory;

protected:
    explicit dbs_scorumpower_delegation(database& db);

public:
    using base_service_i<scorumpower_delegation_object>::get;
    using base_service_i<scorumpower_delegation_object>::is_exists;

    const scorumpower_delegation_object& get(const account_name_type& delegator,
                                             const account_name_type& delegatee) const override;

    bool is_exists(const account_name_type& delegator, const account_name_type& delegatee) const override;

    const scorumpower_delegation_expiration_object& create_expiration(const account_name_type& delegator,
                                                                      const asset& scorumpower,
                                                                      const time_point_sec& expiration) override;
};
} // namespace chain
} // namespace scorum
