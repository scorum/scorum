#pragma once

#include <scorum/chain/services/service_base.hpp>
#include <scorum/chain/schema/scorum_objects.hpp>

namespace scorum {
namespace chain {

struct escrow_service_i : public base_service_i<escrow_object>
{
    using base_service_i<escrow_object>::get;

    virtual const escrow_object& get(const account_name_type& name, uint32_t escrow_id) const = 0;

    virtual const escrow_object& create_escrow(uint32_t escrow_id,
                                               const account_name_type& from,
                                               const account_name_type& to,
                                               const account_name_type& agent,
                                               const time_point_sec& ratification_deadline,
                                               const time_point_sec& escrow_expiration,
                                               const asset& scorum_amount,
                                               const asset& pending_fee)
        = 0;
};

class dbs_escrow : public dbs_service_base<escrow_service_i>
{
    friend class dbservice_dbs_factory;

protected:
    explicit dbs_escrow(database& db);

public:
    using base_service_i<escrow_object>::get;

    const escrow_object& get(const account_name_type& name, uint32_t escrow_id) const override;

    const escrow_object& create_escrow(uint32_t escrow_id,
                                       const account_name_type& from,
                                       const account_name_type& to,
                                       const account_name_type& agent,
                                       const time_point_sec& ratification_deadline,
                                       const time_point_sec& escrow_expiration,
                                       const asset& scorum_amount,
                                       const asset& pending_fee) override;
};
} // namespace chain
} // namespace scorum
