#include <scorum/chain/services/dev_pool.hpp>
#include <scorum/chain/database/database.hpp>

#include <scorum/chain/schema/dev_committee_object.hpp>

namespace scorum {
namespace chain {

dbs_dev_pool::dbs_dev_pool(database& db)
    : _base_type(db)
{
}

const dev_committee_object& dbs_dev_pool::get() const
{
    try
    {
        return db_impl().get<dev_committee_object>();
    }
    FC_CAPTURE_AND_RETHROW()
}

bool dbs_dev_pool::is_exists() const
{
    return nullptr != db_impl().find<dev_committee_object>();
}

const dev_committee_object& dbs_dev_pool::create(const modifier_type& modifier)
{
    return db_impl().create<dev_committee_object>([&](dev_committee_object& o) { modifier(o); });
}

void dbs_dev_pool::update(const modifier_type& modifier)
{
    db_impl().modify(get(), [&](dev_committee_object& o) { modifier(o); });
}

} // namespace chain
} // namespace scorum
