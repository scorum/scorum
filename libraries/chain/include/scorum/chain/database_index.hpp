#pragma once

#include <scorum/chain/database.hpp>

namespace scorum {
namespace chain {

    struct i_database_index
    {
        friend class database;

     private:

        template <typename MultiIndexType>
        void _add_index_impl()
        {
            _db.add_index<MultiIndexType>();
        }

     protected:

        explicit i_database_index(database &db): _db(db){}

        template <typename MultiIndexType>
        void add_core_index()
        {
            _add_index_impl<MultiIndexType>();
        }

     public:

        template <typename MultiIndexType>
        void add_plugin_index()
        {
            _db._plugin_index_signal.connect([this]() { this->_add_index_impl<MultiIndexType>(); });
        }

     private:

        database &_db;
    };
}
}
