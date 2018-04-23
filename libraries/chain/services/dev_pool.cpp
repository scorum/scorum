#include <scorum/chain/services/dev_pool.hpp>
#include <scorum/chain/database/database.hpp>

#include <scorum/chain/schema/dev_committee_object.hpp>

namespace scorum {
namespace chain {

dbs_dev_pool::dbs_dev_pool(database& db)
    : base_service_type(db)
{
}

asset dbs_dev_pool::get_scr_balace() const
{
    return get().scr_balance;
}

void dbs_dev_pool::decrease_scr_balance(const asset& amount)
{
    update([&](dev_committee_object& o) { o.scr_balance -= amount; });
}

} // namespace chain
} // namespace scorum
