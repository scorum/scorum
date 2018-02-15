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

    virtual const registration_pool_object& get() const = 0;

    virtual bool is_exists() const = 0;

    virtual const registration_pool_object&
    create_pool(const asset& supply, const asset& maximum_bonus, const schedule_items_type& schedule_items)
        = 0;

    virtual asset allocate_cash(const account_name_type& committee_member) = 0;

    virtual asset calculate_per_reg() = 0;

    virtual asset decrease_balance(const asset&) = 0;

    virtual bool check_autoclose() = 0;

    virtual void increase_already_allocated_count() = 0;
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
    const registration_pool_object& get() const override;

    bool is_exists() const override;

    const registration_pool_object&
    create_pool(const asset& supply, const asset& maximum_bonus, const schedule_items_type& schedule_items) override;

    // TODO: move to evaluator
    virtual asset allocate_cash(const account_name_type& committee_member) override;

    asset calculate_per_reg() override;

    asset decrease_balance(const asset&) override;

    bool check_autoclose() override;

    void increase_already_allocated_count() override;

private:
    void _close();
};

} // namespace chain
} // namespace scorum
