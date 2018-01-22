#pragma once

#include <scorum/chain/dbs_base_impl.hpp>

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

class dbs_escrow : public escrow_service_i, public dbs_base
{
    friend class dbservice_dbs_factory;

protected:
    explicit dbs_escrow(database& db);

public:
    const escrow_object& get(const account_name_type& name, uint32_t escrow_id) const;

    using modifier_type = std::function<void(escrow_object&)>;

    const escrow_object& create(uint32_t escrow_id,
                                const account_name_type& from,
                                const account_name_type& to,
                                const account_name_type& agent,
                                const time_point_sec& ratification_deadline,
                                const time_point_sec& escrow_expiration,
                                const asset& scorum_amount,
                                const asset& pending_fee);

    void update(const escrow_object& escrow, modifier_type modifier);

    void remove(const escrow_object& escrow);
};
} // namespace chain
} // namespace scorum
