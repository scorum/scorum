#pragma once

#include <vector>

#include <chainbase/database_guard.hpp>

namespace chainbase {

class abstract_index;
class session_index;

class abstract_session
{
public:
    virtual ~abstract_session(){};
    virtual void push() = 0;
    virtual void squash() = 0;
    virtual void undo() = 0;
    virtual int64_t revision() const = 0;
};

template <typename SessionType> class session_impl : public abstract_session
{
public:
    session_impl(SessionType&& s)
        : _session(std::move(s))
    {
    }

    virtual void push() override
    {
        _session.push();
    }
    virtual void squash() override
    {
        _session.squash();
    }
    virtual void undo() override
    {
        _session.undo();
    }
    virtual int64_t revision() const override
    {
        return _session.revision();
    }

private:
    SessionType _session;
};

struct session
{
    std::vector<std::unique_ptr<abstract_session>> _index_sessions;
    int64_t _revision = -1;

private:
    friend class session_index;
    session();

public:
    session(session&& s);
    session(std::vector<std::unique_ptr<abstract_session>>&& s);
    ~session();

    void push();
    void squash();
    void undo();
    int64_t revision() const;
};

class session_index : public database_guard
{
    /**
    * This is a sparse list of known indices kept to accelerate creation of undo sessions
    */
    std::vector<abstract_index*> _index_list;

public:
    virtual ~session_index()
    {
    }

    int64_t revision() const;
    void set_revision(int64_t revision);

    void add_session_index(abstract_index* new_index);
    void clear_session_index();

    session start_undo_session(bool enabled);
    void undo();
    void squash();
    void commit(int64_t revision);
    void undo_all();
};
}
