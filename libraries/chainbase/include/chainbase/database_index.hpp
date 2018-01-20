#pragma once

#include <chainbase/chain_object.hpp>
#include <chainbase/database_guard.hpp>
#include <chainbase/generic_index.hpp>

namespace chainbase {

/** this class is meant to be specified to enable lookup of index type by object type using
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

/**
*  This class
*/
class database_index : public database_guard
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

    template <typename MultiIndexType> const generic_index<MultiIndexType>& add_index()
    {
        typedef generic_index<MultiIndexType> index_type;
        typedef typename index_type::allocator_type index_alloc;

        const uint16_t type_id = index_type::value_type::type_id;

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

        _index_map[type_id] = idx_ptr;

        return *idx_ptr;
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

        return *index_type_ptr(_index_map[index_type::value_type::type_id]);
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

        return index_type_ptr(_index_map[index_type::value_type::type_id])->indices().template get<ByIndex>();
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

        return *index_type_ptr(_index_map[index_type::value_type::type_id]);
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
    std::vector<void*> _index_map;
};
}
