#pragma once

#include <chainbase/abstract_interfaces.hpp>
#include <chainbase/database_index.hpp>
#include <chainbase/segment_manager.hpp>

namespace chainbase {

class undo_db_state : public database_index<segment_manager>
{
public:
    template <typename Lambda> void for_each_index(Lambda&& functor)
    {
        for (auto& item : _index_map)
        {
            abstract_generic_index* index = static_cast<abstract_generic_index*>(item.second);
            functor(*index);
        }
    }

    abstract_undo_session_ptr start_undo_session();
};
}
