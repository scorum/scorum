#include <chainbase/session_index.hpp>
#include <chainbase/database_index.hpp>

namespace chainbase {

session::session()
{
}

session::session(session&& s)
    : _index_sessions(std::move(s._index_sessions))
    , _revision(s._revision)
{
}
session::session(std::vector<std::unique_ptr<abstract_session>>&& s)
    : _index_sessions(std::move(s))
{
    if (_index_sessions.size())
        _revision = _index_sessions[0]->revision();
}

session::~session()
{
    undo();
}

void session::push()
{
    for (auto& i : _index_sessions)
        i->push();
    _index_sessions.clear();
}

void session::squash()
{
    for (auto& i : _index_sessions)
        i->squash();
    _index_sessions.clear();
}

void session::undo()
{
    for (auto& i : _index_sessions)
        i->undo();
    _index_sessions.clear();
}

int64_t session::revision() const
{
    return _revision;
}

//////////////////////////////////////////////////////////////////////////
void session_index::add_session_index(abstract_index* new_index)
{
    _index_list.push_back(new_index);
}

void session_index::clear_session_index()
{
    _index_list.clear();
}

session session_index::start_undo_session(bool enabled)
{
    if (enabled)
    {
        std::vector<std::unique_ptr<abstract_session>> _sub_sessions;
        _sub_sessions.reserve(_index_list.size());
        for (auto& item : _index_list)
        {
            _sub_sessions.push_back(item->start_undo_session(enabled));
        }
        return session(std::move(_sub_sessions));
    }
    else
    {
        return session();
    }
}

void session_index::undo()
{
    for (auto& item : _index_list)
    {
        item->undo();
    }
}

void session_index::squash()
{
    for (auto& item : _index_list)
    {
        item->squash();
    }
}

void session_index::commit(int64_t revision)
{
    for (auto& item : _index_list)
    {
        item->commit(revision);
    }
}

void session_index::undo_all()
{
    for (auto& item : _index_list)
    {
        item->undo_all();
    }
}

int64_t session_index::revision() const
{
    if (_index_list.size() == 0)
        return -1;
    return _index_list[0]->revision();
}

void session_index::set_revision(int64_t revision)
{
    CHAINBASE_REQUIRE_WRITE_LOCK(int64_t);
    for (auto i : _index_list)
        i->set_revision(revision);
}
}
