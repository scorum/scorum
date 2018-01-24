#pragma once

#include <scorum/chain/services/dbs_base.hpp>
#include <vector>
#include <set>
#include <functional>

#include <scorum/chain/schema/registration_objects.hpp>

namespace scorum {
namespace chain {

struct registration_pool_service_i
{
    using schedule_item_type = registration_pool_object::schedule_item;
    using schedule_items_type = std::map<uint8_t /*stage field*/, schedule_item_type /*all other fields*/>;

    virtual const registration_pool_object&
    create_pool(const asset& supply, const asset& maximum_bonus, const schedule_items_type& schedule_items)
        = 0;

    virtual asset allocate_cash(const account_name_type& committee_member) = 0;
    virtual const registration_pool_object& get_pool() const = 0;
};

/**
 * DB service for operations with registration_pool object
 */
class dbs_registration_pool : public dbs_base, public registration_pool_service_i
{
    friend class dbservice_dbs_factory;

protected:
    explicit dbs_registration_pool(database& db);

public:
    virtual const registration_pool_object& get_pool() const override;

    virtual const registration_pool_object&
    create_pool(const asset& supply, const asset& maximum_bonus, const schedule_items_type& schedule_items) override;

    virtual asset allocate_cash(const account_name_type& committee_member) override;

private:
    asset _calculate_per_reg();

    asset _decrease_balance(const asset&);

    bool _check_autoclose();

    void _close();
};

} // namespace chain
} // namespace scorum
