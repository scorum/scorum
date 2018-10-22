#include <chainbase/undo_db_state.hpp>
#include <chainbase/database_index.hpp>

namespace chainbase {

//////////////////////////////////////////////////////////////////////////
struct session_container : public abstract_undo_session
{
    abstract_undo_session_list _session_list;

private:
    friend class undo_db_state;

public:
    session_container(abstract_undo_session_list&& s)
        : _session_list(std::move(s))
    {
    }

    virtual void push() override
    {
        for (auto& i : _session_list)
            i->push();
    }
};

//////////////////////////////////////////////////////////////////////////
abstract_undo_session_ptr undo_db_state::start_undo_session()
{
    abstract_undo_session_list sub_sessions;
    sub_sessions.reserve(_index_map.size());

    for_each_index([&](abstract_generic_index_i& item) { sub_sessions.push_back(item.start_undo_session()); });

    return abstract_undo_session_ptr(new session_container(std::move(sub_sessions)));
}
}
