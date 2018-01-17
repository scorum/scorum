#pragma once

#include <chainbase/abstract_undo_session.hpp>
#include <chainbase/database_guard.hpp>

namespace chainbase {

class extended_abstract_undo_session;

class undo_db_state : public database_guard
{
    /**
    * This is a sparse list of known indices kept to accelerate creation of undo sessions
    */
    std::vector<extended_abstract_undo_session*> _undo_session_list;

public:
    virtual ~undo_db_state()
    {
    }

    int64_t revision() const;
    void set_revision(int64_t revision);

    void add_undo_session(extended_abstract_undo_session* new_session);
    void clear_undo_session();

    abstract_undo_session_ptr start_undo_session();
    void undo();
    void squash();
    void commit(int64_t revision);
    void undo_all();
};
}
