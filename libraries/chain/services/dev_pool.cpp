#include <scorum/chain/services/dev_pool.hpp>
#include <scorum/chain/database.hpp>

#include <scorum/chain/schema/dev_committee_object.hpp>

namespace scorum {
namespace chain {

dbs_dev_pool::dbs_dev_pool(database& db)
    : _base_type(db)
{
}

const dev_committee& dbs_dev_pool::get() const
{
    try
    {
        return db_impl().get<dev_committee>();
    }
    FC_CAPTURE_AND_RETHROW()
}

bool dbs_dev_pool::is_exists() const
{
    return nullptr != db_impl().find<dev_committee>();
}

const dev_committee& dbs_dev_pool::create(const asset& balance)
{
    return db_impl().create<dev_committee>([&](dev_committee& o) { o.balance = balance; });
}

void dbs_dev_pool::increase_balance(const asset& amount)
{
    db_impl().modify(get(), [&](dev_committee& o) { o.balance += amount; });
}

void dbs_dev_pool::decrease_balance(const asset& amount)
{
    increase_balance(-amount);
}

} // namespace chain
} // namespace scorum
