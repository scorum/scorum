#pragma once

#include <scorum/chain/database.hpp>

namespace scorum {
namespace chain {

struct i_dbservice
{
    friend class database;

    protected:

    explicit i_dbservice(database &db): _db(db){}

    //-----------------------
    //scorum::chain::database
    //-----------------------

    const account_object&  get_account(  const account_name_type& name )const;

    //TODO (interface for EVALUATORs)

    //-------------------
    //chainbase::database
    //-------------------

    template <typename MultiIndexType, typename ByIndex>
    auto get_index() const -> decltype(((chainbase::generic_index<MultiIndexType>*)(nullptr))->indicies().template get<ByIndex>())
    {
        return _db.template get_index<MultiIndexType, ByIndex>;
    }

    template <typename ObjectType, typename Modifier>
    void modify(const ObjectType& obj, Modifier&& m)
    {
        return _db.template modify<ObjectType, Modifier>(obj, m);
    }

    //TODO (interface for EVALUATORs)

    private:

    database &_db;
};
}
}
