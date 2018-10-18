#pragma once

#include <scorum/chain/services/service_base.hpp>
#include <scorum/chain/schema/scorum_objects.hpp>

namespace scorum {
namespace chain {

struct decline_voting_rights_request_service_i : public base_service_i<decline_voting_rights_request_object>
{
    using base_service_i<decline_voting_rights_request_object>::get;
    using base_service_i<decline_voting_rights_request_object>::is_exists;

    virtual const decline_voting_rights_request_object& get(const account_id_type& account_id) const = 0;

    virtual bool is_exists(const account_id_type& account_id) const = 0;

    virtual const decline_voting_rights_request_object& create_rights(const account_id_type& account,
                                                                      const fc::microseconds& time_to_life)
        = 0;

    virtual void remove(const decline_voting_rights_request_object& request) = 0;
};

class dbs_decline_voting_rights_request : public dbs_service_base<decline_voting_rights_request_service_i>
{
    friend class dbservice_dbs_factory;

protected:
    explicit dbs_decline_voting_rights_request(database& db);

public:
    using base_service_i<decline_voting_rights_request_object>::get;
    using base_service_i<decline_voting_rights_request_object>::is_exists;

    const decline_voting_rights_request_object& get(const account_id_type& account_id) const override;

    bool is_exists(const account_id_type& account_id) const override;

    const decline_voting_rights_request_object& create_rights(const account_id_type& account,
                                                              const fc::microseconds& time_to_life) override;

private:
    dynamic_global_property_service_i& _dgp_svc;
};
} // namespace chain
} // namespace scorum
