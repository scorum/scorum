#pragma once
#include <boost/container/flat_map.hpp>
#include <boost/any.hpp>

namespace scorum {
namespace chain {
class database;

namespace dba {
template <typename TObject> struct db_accessor;

struct db_accessor_factory
{
    db_accessor_factory(database& db);

    template <typename TObject> db_accessor<TObject>& get_dba() const;

    mutable boost::container::flat_map<boost::typeindex::type_index, boost::any> _db_accessors;
    database& _db;
};

} // dba
} // chain
} // scorum
