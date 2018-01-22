#pragma once

#include <scorum/chain/services/base_service.hpp>

#include <scorum/chain/scorum_objects.hpp>

#include <functional>

namespace scorum {
namespace chain {

class escrow_object;

struct escrow_service_i
{
    using modifier_type = std::function<void(escrow_object&)>;

    virtual const escrow_object& get(const account_name_type& name, uint32_t escrow_id) const = 0;

    virtual const escrow_object& create(uint32_t escrow_id,
                                        const account_name_type& from,
                                        const account_name_type& to,
                                        const account_name_type& agent,
                                        const time_point_sec& ratification_deadline,
                                        const time_point_sec& escrow_expiration,
                                        const asset& scorum_amount,
                                        const asset& pending_fee)
        = 0;

    virtual void update(const escrow_object& escrow, modifier_type modifier) = 0;

    virtual void remove(const escrow_object& escrow) = 0;
};

class dbs_escrow : public dbs_base, public escrow_service_i
{
    friend class dbservice_dbs_factory;

protected:
    explicit dbs_escrow(database& db);

public:
    const escrow_object& get(const account_name_type& name, uint32_t escrow_id) const override;

    const escrow_object& create(uint32_t escrow_id,
                                const account_name_type& from,
                                const account_name_type& to,
                                const account_name_type& agent,
                                const time_point_sec& ratification_deadline,
                                const time_point_sec& escrow_expiration,
                                const asset& scorum_amount,
                                const asset& pending_fee) override;

    void update(const escrow_object& escrow, modifier_type modifier) override;

    void remove(const escrow_object& escrow) override;
};
} // namespace chain
} // namespace scorum
