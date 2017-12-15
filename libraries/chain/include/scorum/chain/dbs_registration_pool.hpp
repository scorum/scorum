#pragma once

#include <scorum/chain/dbs_base_impl.hpp>
#include <vector>
#include <set>
#include <functional>

#include <scorum/chain/registration_objects.hpp>
#include <scorum/chain/account_object.hpp>

namespace scorum {
namespace chain {

/** DB service for operations with registration_pool object
 *  --------------------------------------------
*/
class dbs_registration_pool : public dbs_base
{
    friend class dbservice_dbs_factory;

protected:
    explicit dbs_registration_pool(database& db);

public:
    bool is_pool_exists() const;

    const registration_pool_object& get_pool() const;

    using schedule_item_type = registration_pool_object::schedule_item;
    using schedule_items_type = std::map<uint8_t /*stage field*/, schedule_item_type /*all other fields*/>;
    const registration_pool_object&
    create_pool(const asset& supply, const asset& maximum_bonus, const schedule_items_type& schedule_items);

    asset allocate_cash(const account_name_type& committee_member);

private:
    asset _calculate_per_reg(const registration_pool_object&);

    asset _decrease_balance(const registration_pool_object&, const asset&);

    bool _check_autoclose(const registration_pool_object&);

    void _close(const registration_pool_object&);
};
}
}
