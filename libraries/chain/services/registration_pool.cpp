#include <scorum/chain/services/registration_pool.hpp>
#include <scorum/chain/database/database.hpp>

#include <scorum/chain/services/registration_committee.hpp>

namespace scorum {
namespace chain {

dbs_registration_pool::dbs_registration_pool(database& db)
    : _base_type(db)
{
}

const registration_pool_object& dbs_registration_pool::get() const
{
    try
    {
        // This method uses get(id == 0). It is correct because we create object only one time
        return db_impl().get<registration_pool_object>();
    }
    FC_CAPTURE_AND_RETHROW(("Pool does not exist."))
}

bool dbs_registration_pool::is_exists() const
{
    return nullptr != db_impl().find<registration_pool_object>();
}

const registration_pool_object& dbs_registration_pool::create_pool(const asset& supply,
                                                                   const asset& maximum_bonus,
                                                                   const schedule_items_type& schedule_items)
{
    FC_ASSERT(supply > asset(0, SCORUM_SYMBOL), "Registration supply amount must be more than zerro.");
    FC_ASSERT(maximum_bonus > asset(0, SCORUM_SYMBOL), "Registration maximum bonus amount must be more than zerro.");
    FC_ASSERT(!schedule_items.empty(), "Registration schedule must have at least one item.");

    // check schedule
    for (const auto& value : schedule_items)
    {
        const auto& stage = value.first;
        const schedule_item_type& item = value.second;
        FC_ASSERT(item.users > 0, "Invalid schedule value (users in thousands) for stage ${1}.", ("1", stage));
        FC_ASSERT(item.bonus_percent >= 0 && item.bonus_percent <= 100,
                  "Invalid schedule value (percent) for stage ${1}.", ("1", stage));
    }

    // Check existence here to allow unit tests check input data even if object exists in DB
    // This method uses get(id == 0). Look get_pool.
    FC_ASSERT(db_impl().find<registration_pool_object>() == nullptr, "Can't create more than one pool.");

    // create pool
    const auto& new_pool = db_impl().create<registration_pool_object>([&](registration_pool_object& pool) {
        pool.balance = supply;
        pool.maximum_bonus = maximum_bonus;
        pool.schedule_items.reserve(schedule_items.size());
        for (const auto& value : schedule_items)
        {
            pool.schedule_items.push_back(value.second);
        }
    });

    return new_pool;
}

void dbs_registration_pool::decrease_balance(const asset& amount)
{
    const registration_pool_object& this_pool = get();

    db_impl().modify(this_pool, [&](registration_pool_object& pool) { pool.balance -= amount; });
}

void dbs_registration_pool::increase_already_allocated_count()
{
    const registration_pool_object& this_pool = get();

    db_impl().modify(this_pool, [&](registration_pool_object& pool) { pool.already_allocated_count++; });
}
} // namespace chain
} // namespace scorum
