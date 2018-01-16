#include <scorum/chain/dbs_dynamic_global_property.hpp>

#include <scorum/chain/global_property_object.hpp>
#include <scorum/chain/database.hpp>

namespace scorum {
namespace chain {

dbs_dynamic_global_property::dbs_dynamic_global_property(database& db)
    : _base_type(db)
{
}

const dynamic_global_property_object& dbs_dynamic_global_property::get_dynamic_global_properties() const
{
    return db_impl().get_dynamic_global_properties();
}

void dbs_dynamic_global_property::set_invite_quorum(uint64_t quorum)
{
    const dynamic_global_property_object& properties = get_dynamic_global_properties();
    db_impl().modify(properties, [&](dynamic_global_property_object& p) { p.invite_quorum = quorum; });
}

void dbs_dynamic_global_property::set_dropout_quorum(uint64_t quorum)
{
    const dynamic_global_property_object& properties = get_dynamic_global_properties();
    db_impl().modify(properties, [&](dynamic_global_property_object& p) { p.dropout_quorum = quorum; });
}

void dbs_dynamic_global_property::set_quorum(uint64_t quorum)
{
    const dynamic_global_property_object& properties = get_dynamic_global_properties();
    db_impl().modify(properties, [&](dynamic_global_property_object& p) { p.change_quorum = quorum; });
}

} // namespace scorum
} // namespace chain
