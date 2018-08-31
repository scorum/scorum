#pragma once

#include <boost/container/flat_map.hpp>

#include <chainbase/chain_object.hpp>
#include <chainbase/database_guard.hpp>
#include <chainbase/generic_index.hpp>

namespace chainbase {

/**
*  This class
*/
template <typename segment_manager> class database_index : public segment_manager, public database_guard
{
protected:
    database_index()
    {
    }

public:
    template <typename MultiIndexType> const generic_index<MultiIndexType>& add_index()
    {
        typedef generic_index<MultiIndexType> index_type;

        const uint16_t type_id = index_type::value_type::type_id;

        if (_index_map.find(type_id) != _index_map.end())
        {
            std::string type_name = boost::core::demangle(typeid(typename index_type::value_type).name());
            BOOST_THROW_EXCEPTION(std::logic_error(type_name + "::type_id is already in use"));
        }

        index_type* idx_ptr = this->template allocate_index<index_type>();

        idx_ptr->validate();

        _index_map[type_id] = idx_ptr;

        return *idx_ptr;
    }

    template <typename MultiIndexType> bool has_index() const
    {
        CHAINBASE_REQUIRE_READ_LOCK(typename MultiIndexType::value_type);
        typedef generic_index<MultiIndexType> index_type;
        return _index_map.find((uint16_t)index_type::value_type::type_id) != _index_map.end();
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

        return *index_type_ptr(_index_map.find((uint16_t)index_type::value_type::type_id)->second);
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

        return index_type_ptr(_index_map.find((uint16_t)index_type::value_type::type_id)->second)
            ->indices()
            .template get<ByIndex>();
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

        return *index_type_ptr(_index_map.find((uint16_t)index_type::value_type::type_id)->second);
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
        {
            std::string type_name = boost::core::demangle(typeid(ObjectType).name());
            std::string key_type_name = boost::core::demangle(typeid(CompatibleKey).name());
            BOOST_THROW_EXCEPTION(std::out_of_range("Can't get object of type " + type_name + " by key of type "
                                                    + key_type_name + ". It's not in index."));
        }
        return *obj;
    }

    template <typename ObjectType> const ObjectType& get(const oid<ObjectType>& key = oid<ObjectType>()) const
    {
        CHAINBASE_REQUIRE_READ_LOCK(ObjectType);
        auto obj = find<ObjectType>(key);
        if (!obj)
        {
            std::string type_name = boost::core::demangle(typeid(ObjectType).name());
            BOOST_THROW_EXCEPTION(std::out_of_range("Can't get object of type " + type_name + ". It's not in index."));
        }
        return *obj;
    }

    template <typename ObjectType, typename Modifier> void modify(const ObjectType& obj, Modifier&& m)
    {
        CHAINBASE_REQUIRE_WRITE_LOCK(ObjectType);
        typedef typename get_index_type<ObjectType>::type index_type;
        get_mutable_index<index_type>().modify(obj, m);
    }

    template <typename ObjectType> auto remove(const ObjectType& obj)
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
    /**
    * This is a full map (size 2^16) of all possible index designed for constant time lookup
    */
    boost::container::flat_map<uint16_t, void*> _index_map;
};
}
