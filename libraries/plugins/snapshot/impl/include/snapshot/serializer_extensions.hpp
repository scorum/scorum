#pragma once

#include <fc/io/raw.hpp>
#include <fc/shared_string.hpp>
#include <fc/shared_containers.hpp>
#include <fc/shared_buffer.hpp>

#include <scorum/protocol/types.hpp>

#include <chainbase/chain_object.hpp>

// These operators are used by pack/unpack methods
//
namespace fc {
namespace raw {

template <typename Stream, typename Val> Stream& pack(Stream& stream, const fc::shared_vector<Val>& vec)
{
    size_t sz = vec.size();
    stream << sz;
    for (size_t ci = 0; ci < sz; ++ci)
    {
        stream << (*vec.nth(ci));
    }
    return stream;
}

template <typename Stream, typename Val> Stream& unpack(Stream& stream, fc::shared_vector<Val>& vec)
{
    size_t sz = 0;
    stream >> sz;
    vec.reserve(sz);
    for (size_t ci = 0; ci < sz; ++ci)
    {
        Val v;
        stream >> v;
        vec.push_back(v);
    }
    return stream;
}

template <typename Stream, typename Key> Stream& pack(Stream& stream, const fc::shared_flat_set<Key>& set)
{
    size_t sz = set.size();
    stream << sz;
    for (size_t ci = 0; ci < sz; ++ci)
    {
        stream << (*set.nth(ci));
    }
    return stream;
}

template <typename Stream, typename Key> Stream& unpack(Stream& stream, fc::shared_flat_set<Key>& set)
{
    size_t sz = 0;
    stream >> sz;
    for (size_t ci = 0; ci < sz; ++ci)
    {
        Key v;
        stream >> v;
        set.insert(v);
    }
    return stream;
}

template <typename Stream, typename Key, typename Val>
Stream& pack(Stream& stream, const fc::shared_flat_map<Key, Val>& map)
{
    size_t sz = map.size();
    stream << sz;
    for (size_t ci = 0; ci < sz; ++ci)
    {
        stream << (*map.nth(ci)).first << (*map.nth(ci)).second;
    }
    return stream;
}

template <typename Stream, typename Key, typename Val>
Stream& unpack(Stream& stream, fc::shared_flat_map<Key, Val>& map)
{
    size_t sz = 0;
    stream >> sz;
    for (size_t ci = 0; ci < sz; ++ci)
    {
        Key k;
        Val v;
        stream >> k >> v;
        map.insert(std::make_pair(k, v));
    }
    return stream;
}
}

template <typename Stream, typename ObjectType> Stream& operator<<(Stream& stream, const chainbase::oid<ObjectType>& id)
{
    fc::raw::pack(stream, id);
    return stream;
}

template <typename Stream, typename ObjectType> Stream& operator>>(Stream& stream, chainbase::oid<ObjectType>& id)
{
    fc::raw::unpack(stream, id);
    return stream;
}

template <typename Stream> Stream& operator<<(Stream& stream, const fc::shared_string& sstr)
{
    std::string buff{ fc::to_string(sstr) };
    fc::raw::pack(stream, buff);
    return stream;
}

template <typename Stream> Stream& operator>>(Stream& stream, fc::shared_string& sstr)
{
    std::string buff;
    fc::raw::unpack(stream, buff);
    fc::from_string(sstr, buff);
    return stream;
}

template <typename Stream, typename Key> Stream& operator<<(Stream& stream, const fc::shared_flat_set<Key>& set)
{
    fc::raw::pack(stream, set);
    return stream;
}

template <typename Stream, typename Key> Stream& operator>>(Stream& stream, fc::shared_flat_set<Key>& set)
{
    fc::raw::unpack(stream, set);
    return stream;
}
}
