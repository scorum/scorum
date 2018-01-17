#pragma once

#include <chainbase/chain_object.hpp>
#include <chainbase/undo_db_state.hpp>
#include <chainbase/generic_index.hpp>

namespace chainbase {

/** this class is ment to be specified to enable lookup of index type by object type using
* the SET_INDEX_TYPE macro.
**/
template <typename T> struct get_index_type
{
};

/**
*  This macro must be used at global scope and OBJECT_TYPE and INDEX_TYPE must be fully qualified
*/
#define CHAINBASE_SET_INDEX_TYPE(OBJECT_TYPE, INDEX_TYPE)                                                              \
    namespace chainbase {                                                                                              \
    template <> struct get_index_type<OBJECT_TYPE>                                                                     \
    {                                                                                                                  \
        typedef INDEX_TYPE type;                                                                                       \
    };                                                                                                                 \
    }

#define CHAINBASE_DEFAULT_CONSTRUCTOR(OBJECT_TYPE)                                                                     \
    template <typename Constructor, typename Allocator> OBJECT_TYPE(Constructor&& c, Allocator&&)                      \
    {                                                                                                                  \
        c(*this);                                                                                                      \
    }

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

    virtual void* get() const = 0;
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

    virtual void* get() const
    {
        return &_base;
    }

private:
    BaseIndex& _base;
};

/**
*  This class
*/
class database_index : public undo_db_state
{
protected:
    database_index()
    {
    }

public:
    auto get_segment_manager() -> decltype(((bip::managed_mapped_file*)nullptr)->get_segment_manager())
    {
        return _segment->get_segment_manager();
    }

    size_t get_free_memory() const
    {
        return _segment->get_segment_manager()->get_free_memory();
    }

    template <typename MultiIndexType> void add_index()
    {

        const uint16_t type_id = generic_index<MultiIndexType>::value_type::type_id;
        typedef generic_index<MultiIndexType> index_type;
        typedef typename index_type::allocator_type index_alloc;

        std::string type_name = boost::core::demangle(typeid(typename index_type::value_type).name());

        if (!(_index_map.size() <= type_id || _index_map[type_id] == nullptr))
        {
            BOOST_THROW_EXCEPTION(std::logic_error(type_name + "::type_id is already in use"));
        }

        // clang-format off
        index_type* idx_ptr = nullptr;
        if (!_read_only)
        {
            idx_ptr = _segment->find_or_construct<index_type>(type_name.c_str())(index_alloc(_segment->get_segment_manager()));
        }
        else
        {
            idx_ptr = _segment->find<index_type>(type_name.c_str()).first;
            if (!idx_ptr)
                BOOST_THROW_EXCEPTION(std::runtime_error("unable to find index for " + type_name + " in read only database"));
        }
        // clang-format on

        idx_ptr->validate();

        if (type_id >= _index_map.size())
            _index_map.resize(type_id + 1);

        auto new_index = new undo_session_impl<index_type>(*idx_ptr);
        _index_map[type_id].reset(new_index);
        add_undo_session(new_index);
    }

    template <typename MultiIndexType> bool has_index() const
    {
        CHAINBASE_REQUIRE_READ_LOCK(typename MultiIndexType::value_type);
        typedef generic_index<MultiIndexType> index_type;
        return _index_map.size() > index_type::value_type::type_id && _index_map[index_type::value_type::type_id];
    }

    template <typename MultiIndexType> const generic_index<MultiIndexType>& get_index() const
    {
        CHAINBASE_REQUIRE_READ_LOCK(typename MultiIndexType::value_type);
        typedef generic_index<MultiIndexType> index_type;
        typedef index_type* index_type_ptr;

        if (!has_index<MultiIndexType>())
        {
            std::string type_name = boost::core::demangle(typeid(typename index_type::value_type).name());
            BOOST_THROW_EXCEPTION(std::runtime_error("unable to find index for " + type_name + " in database"));
        }

        return *index_type_ptr(_index_map[index_type::value_type::type_id]->get());
    }

    template <typename MultiIndexType, typename ByIndex>
    auto get_index() const -> decltype(((generic_index<MultiIndexType>*)(nullptr))->indices().template get<ByIndex>())
    {
        CHAINBASE_REQUIRE_READ_LOCK(typename MultiIndexType::value_type);
        typedef generic_index<MultiIndexType> index_type;
        typedef index_type* index_type_ptr;

        if (!has_index<MultiIndexType>())
        {
            std::string type_name = boost::core::demangle(typeid(typename index_type::value_type).name());
            BOOST_THROW_EXCEPTION(std::runtime_error("unable to find index for " + type_name + " in database"));
        }

        return index_type_ptr(_index_map[index_type::value_type::type_id]->get())->indices().template get<ByIndex>();
    }

    template <typename MultiIndexType> generic_index<MultiIndexType>& get_mutable_index()
    {
        CHAINBASE_REQUIRE_WRITE_LOCK(typename MultiIndexType::value_type);
        typedef generic_index<MultiIndexType> index_type;
        typedef index_type* index_type_ptr;

        if (!has_index<MultiIndexType>())
        {
            std::string type_name = boost::core::demangle(typeid(typename index_type::value_type).name());
            BOOST_THROW_EXCEPTION(std::runtime_error("unable to find index for " + type_name + " in database"));
        }

        return *index_type_ptr(_index_map[index_type::value_type::type_id]->get());
    }

    template <typename ObjectType, typename IndexedByType, typename CompatibleKey>
    const ObjectType* find(CompatibleKey&& key) const
    {
        CHAINBASE_REQUIRE_READ_LOCK(ObjectType);
        typedef typename get_index_type<ObjectType>::type index_type;
        const auto& idx = get_index<index_type>().indices().template get<IndexedByType>();
        auto itr = idx.find(std::forward<CompatibleKey>(key));
        if (itr == idx.end())
            return nullptr;
        return &*itr;
    }

    template <typename ObjectType> const ObjectType* find(oid<ObjectType> key = oid<ObjectType>()) const
    {
        CHAINBASE_REQUIRE_READ_LOCK(ObjectType);
        typedef typename get_index_type<ObjectType>::type index_type;
        const auto& idx = get_index<index_type>().indices();
        auto itr = idx.find(key);
        if (itr == idx.end())
            return nullptr;
        return &*itr;
    }

    template <typename ObjectType, typename IndexedByType, typename CompatibleKey>
    const ObjectType& get(CompatibleKey&& key) const
    {
        CHAINBASE_REQUIRE_READ_LOCK(ObjectType);
        auto obj = find<ObjectType, IndexedByType>(std::forward<CompatibleKey>(key));
        if (!obj)
            BOOST_THROW_EXCEPTION(std::out_of_range("unknown key"));
        return *obj;
    }

    template <typename ObjectType> const ObjectType& get(const oid<ObjectType>& key = oid<ObjectType>()) const
    {
        CHAINBASE_REQUIRE_READ_LOCK(ObjectType);
        auto obj = find<ObjectType>(key);
        if (!obj)
            BOOST_THROW_EXCEPTION(std::out_of_range("unknown key"));
        return *obj;
    }

    template <typename ObjectType, typename Modifier> void modify(const ObjectType& obj, Modifier&& m)
    {
        CHAINBASE_REQUIRE_WRITE_LOCK(ObjectType);
        typedef typename get_index_type<ObjectType>::type index_type;
        get_mutable_index<index_type>().modify(obj, m);
    }

    template <typename ObjectType> void remove(const ObjectType& obj)
    {
        CHAINBASE_REQUIRE_WRITE_LOCK(ObjectType);
        typedef typename get_index_type<ObjectType>::type index_type;
        return get_mutable_index<index_type>().remove(obj);
    }

    template <typename ObjectType, typename Constructor> const ObjectType& create(Constructor&& con)
    {
        CHAINBASE_REQUIRE_WRITE_LOCK(ObjectType);
        typedef typename get_index_type<ObjectType>::type index_type;
        return get_mutable_index<index_type>().emplace(std::forward<Constructor>(con));
    }

protected:
    std::unique_ptr<bip::managed_mapped_file> _segment;
    std::unique_ptr<bip::managed_mapped_file> _meta;

    /**
    * This is a full map (size 2^16) of all possible index designed for constant time lookup
    */
    std::vector<std::unique_ptr<extended_abstract_undo_session>> _index_map;
};
}
