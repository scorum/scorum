#pragma once

#include <scorum/chain/services/base_service.hpp>

#include <scorum/chain/schema/scorum_objects.hpp>

namespace scorum {
namespace chain {

class decline_voting_rights_request_object;

struct decline_voting_rights_request_service_i
{
    virtual const decline_voting_rights_request_object& get(const account_id_type& account_id) const = 0;

    virtual bool is_exists(const account_id_type& account_id) const = 0;

    virtual const decline_voting_rights_request_object& create(const account_id_type& account,
                                                               const fc::microseconds& time_to_life)
        = 0;

    virtual void remove(const decline_voting_rights_request_object& request) = 0;
};

class dbs_decline_voting_rights_request : public dbs_base, public decline_voting_rights_request_service_i
{
    friend class dbservice_dbs_factory;

protected:
    explicit dbs_decline_voting_rights_request(database& db);

public:
    const decline_voting_rights_request_object& get(const account_id_type& account_id) const override;

    bool is_exists(const account_id_type& account_id) const override;

    const decline_voting_rights_request_object& create(const account_id_type& account,
                                                       const fc::microseconds& time_to_life) override;

    void remove(const decline_voting_rights_request_object& request) override;
};
} // namespace chain
} // namespace scorum
