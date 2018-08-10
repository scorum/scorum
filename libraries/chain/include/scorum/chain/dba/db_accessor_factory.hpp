#pragma once
#include <boost/container/flat_map.hpp>
#include <boost/any.hpp>

namespace scorum {
namespace chain {
class database;

namespace dba {
template <typename TObject> struct db_accessor_i;

struct db_accessor_factory;
template <typename TObject> db_accessor_i<TObject>& get_db_accessor(db_accessor_factory& dba_factory);

struct db_accessor_factory
{
    db_accessor_factory(database& db);

    template <typename TObject> db_accessor_i<TObject>& get_dba() const;

private:
    template <typename UObject> friend db_accessor_i<UObject>& get_db_accessor(db_accessor_factory&);

    template <typename TObject> db_accessor_i<TObject>& get_dba_impl() const;

    mutable boost::container::flat_map<boost::typeindex::type_index, boost::any> _db_accessors;
    database& _db;
};

} // dba
} // chain
} // scorum
