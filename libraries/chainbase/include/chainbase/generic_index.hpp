#pragma once

#include <boost/throw_exception.hpp>
#include <stdexcept>

#include <fc/shared_containers.hpp>

#include <chainbase/undo_session.hpp>

namespace chainbase {

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
    using allocator_type = typename MultiIndexType::allocator_type;

    template <typename Allocator>
    base_index(const Allocator& a)
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

    const MultiIndexType& indices() const
    {
        return _indices;
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

protected:
    /**
    * Construct a new element in the shared_multi_index_container.
    * Set the ID to the next available ID, then increment _next_id and fire off on_create().
    */
    template <typename Constructor> const value_type& emplace(Constructor&& c)
    {
        auto new_id = _next_id;

        auto constructor = [&](value_type& v) {
            v.id = new_id;
            c(v);
            new_id = v.id;
        };

        const auto& val = emplace_(constructor, get_allocator());

        _next_id = new_id._id + 1;

        return val;
    }

    template <typename Modifier> void modify(const value_type& obj, Modifier&& m)
    {
        auto ok = _indices.modify(_indices.iterator_to(obj), m);
        if (!ok)
            BOOST_THROW_EXCEPTION(
                std::logic_error("Could not modify object, most likely a uniqueness constraint was violated"));
    }

    void remove(const value_type& obj)
    {
        _indices.erase(_indices.iterator_to(obj));
    }

    allocator_type get_allocator() const noexcept
    {
        return _indices.get_allocator();
    }

    template <class... Args> const value_type& emplace_(Args&&... args)
    {
        auto insert_result = _indices.emplace(args...);

        if (!insert_result.second)
        {
            BOOST_THROW_EXCEPTION(
                std::logic_error("could not insert object, most likely a uniqueness constraint was violated"));
        }

        return *insert_result.first;
    }

protected:
    typename value_type::id_type _next_id = 0;
    MultiIndexType _indices;
    uint32_t _size_of_value_type = 0;
    uint32_t _size_of_this = 0;
};

//------------------------------------------------------------------------------------------------------//

template <typename MultiIndexType>
class generic_index : public abstract_generic_index_i, public base_index<MultiIndexType>
{
public:
    using value_type = typename MultiIndexType::value_type;
    using base_index_type = base_index<MultiIndexType>;

private:
    //------------------------------------------//
    class undo_state
    {
    public:
        using id_type = typename value_type::id_type;
        using id_type_set = fc::shared_set<id_type>;
        using id_value_type_map = fc::shared_map<id_type, value_type>;

        template <typename T>
        undo_state(const fc::shared_allocator<T>& al)
            : old_values(al)
            , removed_values(al)
            , new_ids(al)
        {
        }

        id_value_type_map old_values;
        id_value_type_map removed_values;
        id_type_set new_ids;
        id_type old_next_id = 0;
        int64_t revision = 0;
    };

public:
    template <typename Allocator>
    generic_index(const Allocator& a)
        : base_index_type(a)
        , _stack(a)
    {
    }

    template <typename Constructor> const value_type& emplace(Constructor&& c)
    {
        const value_type& value = base_index_type::emplace(c);

        on_create(value);

        return value;
    }

    template <typename Modifier> void modify(const value_type& obj, Modifier&& m)
    {
        auto unmodified_copy = obj;

        base_index_type::modify(obj, m);

        on_modify(unmodified_copy);
    }

    void remove(const value_type& obj)
    {
        on_remove(obj); // after base_index_type::remove(obj); obj is invalid, so do this call here

        base_index_type::remove(obj);
    }

private:
    // abstract_generic_index_i interface
    abstract_undo_session_ptr start_undo_session() override
    {
        _stack.emplace_back(this->get_allocator());
        _stack.back().old_next_id = this->_next_id;
        _stack.back().revision = ++_revision;

        return std::move(abstract_undo_session_ptr(new session(*this)));
    }

    /**
    *  Restores the state to how it was prior to the current session discarding all changes
    *  made between the last revision and the current revision.
    */
    void undo() override
    {
        if (!enabled())
            return;

        const auto& head = _stack.back();

        for (auto& item : head.old_values)
        {
            base_index_type::modify(this->get(item.second.id), [&](value_type& v) { v = std::move(item.second); });
        }

        for (auto id : head.new_ids)
        {
            base_index_type::remove(this->get(id));
        }

        this->_next_id = head.old_next_id;

        for (auto& item : head.removed_values)
        {
            base_index_type::emplace_(std::move(item.second));
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
    void squash() override
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
    void commit(int64_t revision) override
    {
        while (_stack.size() && _stack[0].revision <= revision)
        {
            _stack.pop_front();
        }
    }

    /**
    * Unwinds all undo states
    */
    void undo_all() override
    {
        while (enabled())
            undo();
    }

    void set_revision(int64_t revision) override
    {
        if (_stack.size() != 0)
            BOOST_THROW_EXCEPTION(std::logic_error("cannot set revision while there is an existing undo stack"));
        _revision = revision;
    }

    int64_t revision() const override
    {
        return _revision;
    }

    //////////////////////////////////////////////////////////////////////////
    bool enabled() const
    {
        return !_stack.empty();
    }

    void on_modify(const value_type& v)
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

    void on_remove(const value_type& v)
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

    void on_create(const value_type& v)
    {
        if (!enabled())
            return;
        auto& head = _stack.back();

        head.new_ids.insert(v.id);
    }

private:
    /**
    *  Each new session increments the revision, a squash will decrement the revision by combining
    *  the two most recent revisions into one revision.
    *
    *  Commit will discard all revisions prior to the committed revision.
    */
    int64_t _revision = 0;

    fc::shared_deque<undo_state> _stack;
};

} // namespace chainbase
