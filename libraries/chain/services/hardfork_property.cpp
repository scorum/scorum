#include <scorum/chain/database/database.hpp>
#include <scorum/chain/services/hardfork_property.hpp>
#include <scorum/chain/schema/witness_objects.hpp>

namespace scorum {
namespace chain {

dbs_hardfork_property::dbs_hardfork_property(database& db)
    : _base_type(db)
{
}

const hardfork_property_object& dbs_hardfork_property::create(const modifier_type& modifier)
{
    return db_impl().create<hardfork_property_object>([&](hardfork_property_object& o) { modifier(o); });
}

const hardfork_property_object& dbs_hardfork_property::get() const
{
    try
    {
        return db_impl().get<hardfork_property_object>();
    }
    FC_CAPTURE_AND_RETHROW()
}

void dbs_hardfork_property::update(const modifier_type& modifier)
{
    db_impl().modify(get(), [&](hardfork_property_object& c) { modifier(c); });
}

void dbs_hardfork_property::remove()
{
    db_impl().remove(get());
}

bool dbs_hardfork_property::is_exists() const
{
    return nullptr != db_impl().find<hardfork_property_object>();
}

} // namespace chain
} // namespace scorum
