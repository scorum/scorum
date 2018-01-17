#include <chainbase/undo_db_state.hpp>
#include <chainbase/database_index.hpp>

namespace chainbase {

//////////////////////////////////////////////////////////////////////////
struct session_container : public abstract_undo_session
{
    abstract_undo_session_list _session_list;
    int64_t _revision = -1;

private:
    friend class undo_db_state;
    session_container();

public:
    session_container(session_container&& s);
    session_container(abstract_undo_session_list&& s);
    ~session_container();

    virtual void push() override;
    virtual void squash() override;
    virtual void undo() override;
    virtual int64_t revision() const override;
};

session_container::session_container()
{
}

session_container::session_container(session_container&& s)
    : _session_list(std::move(s._session_list))
    , _revision(s._revision)
{
}
session_container::session_container(abstract_undo_session_list&& s)
    : _session_list(std::move(s))
{
    if (_session_list.size())
        _revision = _session_list[0]->revision();
}

session_container::~session_container()
{
    undo();
}

void session_container::push()
{
    for (auto& i : _session_list)
        i->push();
    _session_list.clear();
}

void session_container::squash()
{
    for (auto& i : _session_list)
        i->squash();
    _session_list.clear();
}

void session_container::undo()
{
    for (auto& i : _session_list)
        i->undo();
    _session_list.clear();
}

int64_t session_container::revision() const
{
    return _revision;
}

//////////////////////////////////////////////////////////////////////////
void undo_db_state::add_undo_session(extended_abstract_undo_session* new_session)
{
    _undo_session_list.push_back(new_session);
}

void undo_db_state::clear_undo_session()
{
    _undo_session_list.clear();
}

abstract_undo_session_ptr undo_db_state::start_undo_session()
{
    abstract_undo_session_list sub_sessions;
    sub_sessions.reserve(_undo_session_list.size());
    for (auto& item : _undo_session_list)
    {
        sub_sessions.push_back(item->start_undo_session());
    }
    return std::move(abstract_undo_session_ptr(new session_container(std::move(sub_sessions))));
}

void undo_db_state::undo()
{
    for (auto& item : _undo_session_list)
    {
        item->undo();
    }
}

void undo_db_state::squash()
{
    for (auto& item : _undo_session_list)
    {
        item->squash();
    }
}

void undo_db_state::commit(int64_t revision)
{
    for (auto& item : _undo_session_list)
    {
        item->commit(revision);
    }
}

void undo_db_state::undo_all()
{
    for (auto& item : _undo_session_list)
    {
        item->undo_all();
    }
}

int64_t undo_db_state::revision() const
{
    if (_undo_session_list.size() == 0)
        return -1;
    return _undo_session_list[0]->revision();
}

void undo_db_state::set_revision(int64_t revision)
{
    CHAINBASE_REQUIRE_WRITE_LOCK(int64_t);

    for (auto& i : _undo_session_list)
        i->set_revision(revision);
}
}
