#pragma once

#include <chainbase/abstract_undo_session.hpp>
#include <chainbase/database_index.hpp>

namespace chainbase {

struct extended_abstract_undo_session
{
    virtual ~extended_abstract_undo_session()
    {
    }

    virtual int64_t revision() const = 0;
    virtual void set_revision(int64_t revision) = 0;

    virtual abstract_undo_session_ptr start_undo_session() = 0;
    virtual void undo() const = 0;
    virtual void squash() const = 0;
    virtual void commit(int64_t revision) const = 0;
    virtual void undo_all() const = 0;
};

template <typename BaseIndex> class undo_session_impl : public extended_abstract_undo_session
{
public:
    undo_session_impl(BaseIndex& base)
        : _base(base)
    {
    }

    virtual abstract_undo_session_ptr start_undo_session() override
    {
        return std::move(_base.start_undo_session());
    }

    virtual void set_revision(int64_t revision) override
    {
        _base.set_revision(revision);
    }
    virtual int64_t revision() const override
    {
        return _base.revision();
    }
    virtual void undo() const override
    {
        _base.undo();
    }
    virtual void squash() const override
    {
        _base.squash();
    }
    virtual void commit(int64_t revision) const override
    {
        _base.commit(revision);
    }
    virtual void undo_all() const override
    {
        _base.undo_all();
    }

private:
    BaseIndex& _base;
};

class undo_db_state : public database_index
{
    /**
    * This is a sparse list of known indices kept to accelerate creation of undo sessions
    */
    std::vector<std::unique_ptr<extended_abstract_undo_session>> _undo_session_list;

protected:
    void add_undo_session(std::unique_ptr<extended_abstract_undo_session>&& new_session);
    void clear_undo_session();

public:
    virtual ~undo_db_state()
    {
    }

    template <typename MultiIndexType> const generic_index<MultiIndexType>& add_index()
    {
        typedef generic_index<MultiIndexType> index_type;

        const index_type& index = database_index::add_index<MultiIndexType>();

        auto new_session = new undo_session_impl<index_type>(const_cast<index_type&>(index));
        add_undo_session(std::unique_ptr<extended_abstract_undo_session>(new_session));

        return index;
    }

    int64_t revision() const;
    void set_revision(int64_t revision);

    abstract_undo_session_ptr start_undo_session();
    void undo();
    void squash();
    void commit(int64_t revision);
    void undo_all();
};
}
