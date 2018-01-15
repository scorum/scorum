#pragma once

#include <boost/interprocess/managed_mapped_file.hpp>
#include <boost/interprocess/allocators/allocator.hpp>

#include <boost/interprocess/containers/map.hpp>
#include <boost/interprocess/containers/set.hpp>
#include <boost/interprocess/containers/deque.hpp>

#include <boost/multi_index_container.hpp>

#include <boost/throw_exception.hpp>
#include <stdexcept>

namespace chainbase {

namespace bip = boost::interprocess;

template <typename T> using allocator = bip::allocator<T, bip::managed_mapped_file::segment_manager>;

template <typename Object, typename... Args>
using shared_multi_index_container = boost::multi_index_container<Object, Args..., allocator<Object>>;

/**
*  The value_type stored in the multiindex container must have a integer field with the name 'id'.  This will
*  be the primary key and it will be assigned and managed by generic_index.
*
*  Additionally, the constructor for value_type must take an allocator
*/
template <typename MultiIndexType> class base_index
{
public:
    using value_type = typename MultiIndexType::value_type;

protected:
    typename value_type::id_type _next_id = 0;
    MultiIndexType _indices;
    uint32_t _size_of_value_type = 0;
    uint32_t _size_of_this = 0;

protected:
    virtual void on_create(const value_type& v) = 0;
    virtual void on_remove(const value_type& v) = 0;
    virtual void on_modify(const value_type& v) = 0;

public:
    base_index(allocator<value_type> a)
        : _indices(a)
        , _size_of_value_type(sizeof(typename MultiIndexType::node_type))
        , _size_of_this(sizeof(*this))
    {
    }

    void validate() const /// What for???
    {
        if (sizeof(typename MultiIndexType::node_type) != _size_of_value_type || sizeof(*this) != _size_of_this)
            BOOST_THROW_EXCEPTION(std::runtime_error("content of memory does not match data expected by executable"));
    }

    /**
    * Construct a new element in the multi_index_container.
    * Set the ID to the next available ID, then increment _next_id and fire off on_create().
    */
    template <typename Constructor> const value_type& emplace(Constructor&& c)
    {
        auto new_id = _next_id;

        auto constructor = [&](value_type& v) {
            v.id = new_id;
            c(v);
        };

        auto insert_result = _indices.emplace(constructor, _indices.get_allocator());

        if (!insert_result.second)
        {
            BOOST_THROW_EXCEPTION(
                std::logic_error("could not insert object, most likely a uniqueness constraint was violated"));
        }

        ++_next_id;
        on_create(*insert_result.first);
        return *insert_result.first;
    }

    template <typename Modifier> void modify(const value_type& obj, Modifier&& m)
    {
        on_modify(obj);
        auto ok = _indices.modify(_indices.iterator_to(obj), m);
        if (!ok)
            BOOST_THROW_EXCEPTION(
                std::logic_error("Could not modify object, most likely a uniqueness constraint was violated"));
    }

    void remove(const value_type& obj)
    {
        on_remove(obj);
        _indices.erase(_indices.iterator_to(obj));
    }

    template <typename CompatibleKey> const value_type* find(CompatibleKey&& key) const
    {
        auto itr = _indices.find(std::forward<CompatibleKey>(key));
        if (itr != _indices.end())
            return &*itr;
        return nullptr;
    }

    template <typename CompatibleKey> const value_type& get(CompatibleKey&& key) const
    {
        auto ptr = find(key);
        if (!ptr)
            BOOST_THROW_EXCEPTION(std::out_of_range("key not found"));
        return *ptr;
    }

    const MultiIndexType& indices() const
    {
        return _indices;
    }

    void remove_object(int64_t id)
    {
        const value_type* val = find(typename value_type::id_type(id));
        if (!val)
            BOOST_THROW_EXCEPTION(std::out_of_range(boost::lexical_cast<std::string>(id)));
        remove(*val);
    }
};

////////////////////////////////////////////////////////////////////////////////////////////////////////

template <typename value_type> class undo_state
{
public:
    using id_type = typename value_type::id_type;
    using id_value_allocator_type = allocator<std::pair<const id_type, value_type>>;
    using id_allocator_type = allocator<id_type>;

    template <typename T>
    undo_state(allocator<T> al)
        : old_values(id_value_allocator_type(al.get_segment_manager()))
        , removed_values(id_value_allocator_type(al.get_segment_manager()))
        , new_ids(id_allocator_type(al.get_segment_manager()))
    {
    }

    using id_value_type_map
        = boost::interprocess::map<id_type, value_type, std::less<id_type>, id_value_allocator_type>;
    using id_type_set = boost::interprocess::set<id_type, std::less<id_type>, id_allocator_type>;

    id_value_type_map old_values;
    id_value_type_map removed_values;
    id_type_set new_ids;
    id_type old_next_id = 0;
    int64_t revision = 0;
};

//------------------------------------------------------------------------------------------------------//

template <typename MultiIndexType> class generic_index : public base_index<MultiIndexType>
{
public:
    using allocator_type = allocator<generic_index>;
    using value_type = typename MultiIndexType::value_type;
    using undo_state_type = undo_state<value_type>;

private:
    /**
    *  Each new session increments the revision, a squash will decrement the revision by combining
    *  the two most recent revisions into one revision.
    *
    *  Commit will discard all revisions prior to the committed revision.
    */
    int64_t _revision = 0;

    boost::interprocess::deque<undo_state_type, allocator<undo_state_type>> _stack;

public:
    generic_index(allocator<value_type> a)
        : base_index<MultiIndexType>(a)
        , _stack(a)
    {
    }

    int64_t revision() const
    {
        return _revision;
    }

    class session
    {
    public:
        session(session&& mv)
            : _index(mv._index)
            , _apply(mv._apply)
        {
            mv._apply = false;
        }

        ~session()
        {
            if (_apply)
            {
                _index.undo();
            }
        }

        /** leaves the UNDO state on the stack when session goes out of scope */
        void push()
        {
            _apply = false;
        }
        /** combines this session with the prior session */
        void squash()
        {
            if (_apply)
                _index.squash();
            _apply = false;
        }
        void undo()
        {
            if (_apply)
                _index.undo();
            _apply = false;
        }

        session& operator=(session&& mv)
        {
            if (this == &mv)
                return *this;
            if (_apply)
                _index.undo();
            _apply = mv._apply;
            mv._apply = false;
            return *this;
        }

        int64_t revision() const
        {
            return _revision;
        }

    private:
        friend class generic_index;

        session(generic_index& idx, int64_t revision)
            : _index(idx)
            , _revision(revision)
        {
            if (revision == -1)
                _apply = false;
        }

        generic_index& _index;
        bool _apply = true;
        int64_t _revision = 0;
    };

    session start_undo_session(bool enabled)
    {
        if (enabled)
        {
            _stack.emplace_back(this->_indices.get_allocator());
            _stack.back().old_next_id = this->_next_id;
            _stack.back().revision = ++_revision;
            return session(*this, _revision);
        }
        else
        {
            return session(*this, -1);
        }
    }

    /**
    *  Restores the state to how it was prior to the current session discarding all changes
    *  made between the last revision and the current revision.
    */
    void undo()
    {
        if (!enabled())
            return;

        const auto& head = _stack.back();

        for (auto& item : head.old_values)
        {
            auto ok = this->_indices.modify(this->_indices.find(item.second.id),
                                            [&](value_type& v) { v = std::move(item.second); });
            if (!ok)
                BOOST_THROW_EXCEPTION(
                    std::logic_error("Could not modify object, most likely a uniqueness constraint was violated"));
        }

        for (auto id : head.new_ids)
        {
            this->_indices.erase(this->_indices.find(id));
        }
        this->_next_id = head.old_next_id;

        for (auto& item : head.removed_values)
        {
            bool ok = this->_indices.emplace(std::move(item.second)).second;
            if (!ok)
                BOOST_THROW_EXCEPTION(
                    std::logic_error("Could not restore object, most likely a uniqueness constraint was violated"));
        }

        _stack.pop_back();
        --_revision;
    }

    /**
    *  This method works similar to git squash, it merges the change set from the two most
    *  recent revision numbers into one revision number (reducing the head revision number)
    *
    *  This method does not change the state of the index, only the state of the undo buffer.
    */
    void squash()
    {
        if (!enabled())
            return;
        if (_stack.size() == 1)
        {
            _stack.pop_front();
            return;
        }

        auto& state = _stack.back();
        auto& prev_state = _stack[_stack.size() - 2];

        // An object's relationship to a state can be:
        // in new_ids            : new
        // in old_values (was=X) : upd(was=X)
        // in removed (was=X)    : del(was=X)
        // not in any of above   : nop
        //
        // When merging A=prev_state and B=state we have a 4x4 matrix of all possibilities:
        //
        //                   |--------------------- B ----------------------|
        //
        //                +------------+------------+------------+------------+
        //                | new        | upd(was=Y) | del(was=Y) | nop        |
        //   +------------+------------+------------+------------+------------+
        // / | new        | N/A        | new       A| nop       C| new       A|
        // | +------------+------------+------------+------------+------------+
        // | | upd(was=X) | N/A        | upd(was=X)A| del(was=X)C| upd(was=X)A|
        // A +------------+------------+------------+------------+------------+
        // | | del(was=X) | N/A        | N/A        | N/A        | del(was=X)A|
        // | +------------+------------+------------+------------+------------+
        // \ | nop        | new       B| upd(was=Y)B| del(was=Y)B| nop      AB|
        //   +------------+------------+------------+------------+------------+
        //
        // Each entry was composed by labelling what should occur in the given case.
        //
        // Type A means the composition of states contains the same entry as the first of the two merged states for that
        // object.
        // Type B means the composition of states contains the same entry as the second of the two merged states for
        // that object.
        // Type C means the composition of states contains an entry different from either of the merged states for that
        // object.
        // Type N/A means the composition of states violates causal timing.
        // Type AB means both type A and type B simultaneously.
        //
        // The merge() operation is defined as modifying prev_state in-place to be the state object which represents the
        // composition of
        // state A and B.
        //
        // Type A (and AB) can be implemented as a no-op; prev_state already contains the correct value for the merged
        // state.
        // Type B (and AB) can be implemented by copying from state to prev_state.
        // Type C needs special case-by-case logic.
        // Type N/A can be ignored or assert(false) as it can only occur if prev_state and state have illegal values
        // (a serious logic error which should never happen).
        //

        // We can only be outside type A/AB (the nop path) if B is not nop, so it suffices to iterate through B's three
        // containers.

        for (const auto& item : state.old_values)
        {
            if (prev_state.new_ids.find(item.second.id) != prev_state.new_ids.end())
            {
                // new+upd -> new, type A
                continue;
            }
            if (prev_state.old_values.find(item.second.id) != prev_state.old_values.end())
            {
                // upd(was=X) + upd(was=Y) -> upd(was=X), type A
                continue;
            }
            // del+upd -> N/A
            assert(prev_state.removed_values.find(item.second.id) == prev_state.removed_values.end());
            // nop+upd(was=Y) -> upd(was=Y), type B
            prev_state.old_values.emplace(std::move(item));
        }

        // *+new, but we assume the N/A cases don't happen, leaving type B nop+new -> new
        for (auto id : state.new_ids)
            prev_state.new_ids.insert(id);

        // *+del
        for (auto& obj : state.removed_values)
        {
            if (prev_state.new_ids.find(obj.second.id) != prev_state.new_ids.end())
            {
                // new + del -> nop (type C)
                prev_state.new_ids.erase(obj.second.id);
                continue;
            }
            auto it = prev_state.old_values.find(obj.second.id);
            if (it != prev_state.old_values.end())
            {
                // upd(was=X) + del(was=Y) -> del(was=X)
                prev_state.removed_values.emplace(std::move(*it));
                prev_state.old_values.erase(obj.second.id);
                continue;
            }
            // del + del -> N/A
            assert(prev_state.removed_values.find(obj.second.id) == prev_state.removed_values.end());
            // nop + del(was=Y) -> del(was=Y)
            prev_state.removed_values.emplace(std::move(obj)); //[obj.second->id] = std::move(obj.second);
        }

        _stack.pop_back();
        --_revision;
    }

    /**
    * Discards all undo history prior to revision
    */
    void commit(int64_t revision)
    {
        while (_stack.size() && _stack[0].revision <= revision)
        {
            _stack.pop_front();
        }
    }

    /**
    * Unwinds all undo states
    */
    void undo_all()
    {
        while (enabled())
            undo();
    }

    void set_revision(int64_t revision)
    {
        if (_stack.size() != 0)
            BOOST_THROW_EXCEPTION(std::logic_error("cannot set revision while there is an existing undo stack"));
        _revision = revision;
    }

private:
    bool enabled() const
    {
        return _stack.size();
    }

    void on_modify(const value_type& v) override
    {
        if (!enabled())
            return;

        auto& head = _stack.back();

        if (head.new_ids.find(v.id) != head.new_ids.end())
            return;

        auto itr = head.old_values.find(v.id);
        if (itr != head.old_values.end())
            return;

        head.old_values.emplace(std::pair<typename value_type::id_type, const value_type&>(v.id, v));
    }

    void on_remove(const value_type& v) override
    {
        if (!enabled())
            return;

        auto& head = _stack.back();
        if (head.new_ids.count(v.id))
        {
            head.new_ids.erase(v.id);
            return;
        }

        auto itr = head.old_values.find(v.id);
        if (itr != head.old_values.end())
        {
            head.removed_values.emplace(std::move(*itr));
            head.old_values.erase(v.id);
            return;
        }

        if (head.removed_values.count(v.id))
            return;

        head.removed_values.emplace(std::pair<typename value_type::id_type, const value_type&>(v.id, v));
    }

    void on_create(const value_type& v) override
    {
        if (!enabled())
            return;
        auto& head = _stack.back();

        head.new_ids.insert(v.id);
    }
};

} // namespace chainbase
