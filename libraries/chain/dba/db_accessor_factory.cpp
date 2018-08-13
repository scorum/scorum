#include <scorum/chain/dba/db_accessor_factory.hpp>
#include <scorum/chain/dba/db_accessor_i.hpp>
#include <scorum/chain/database/database.hpp>

#include <scorum/chain/schema/game_object.hpp>
#include <scorum/chain/schema/proposal_object.hpp>
#include <scorum/chain/schema/betting_property_object.hpp>
#include <scorum/chain/schema/bet_objects.hpp>

#define DB_TYPES                                                                                                       \
    (game_object)(proposal_object)(betting_property_object)(bet_object)(pending_bet_object)(matched_bet_object)

#define INSTANTIATE_DBA_FACTORY_METHODS(_1, _2, TYPE)                                                                  \
    template db_accessor_i<TYPE>& get_db_accessor<TYPE>(db_accessor_factory&);                                         \
    template db_accessor_i<TYPE>& db_accessor_factory::get_dba<TYPE>() const;                                          \
    template db_accessor_i<TYPE>& db_accessor_factory::get_dba_impl<TYPE>() const;

namespace scorum {
namespace chain {
namespace dba {

template <typename TObject> db_accessor_i<TObject>& get_db_accessor(db_accessor_factory& dba_factory)
{
    return dba_factory.get_dba_impl<TObject>();
}

db_accessor_factory::db_accessor_factory(database& db)
    : _db(db)
{
}

template <typename TObject> db_accessor_i<TObject>& db_accessor_factory::get_dba() const
{
    return get_db_accessor<TObject>(_db);
}

template <typename TObject> db_accessor_i<TObject>& db_accessor_factory::get_dba_impl() const
{
    auto it = _db_accessors
                  .emplace(boost::typeindex::type_id<TObject>(), db_accessor_i<TObject>(db_accessor<TObject>{ _db }))
                  .first;

    return boost::any_cast<db_accessor_i<TObject>&>(it->second);
}

BOOST_PP_SEQ_FOR_EACH(INSTANTIATE_DBA_FACTORY_METHODS, , DB_TYPES)
}
}
}
