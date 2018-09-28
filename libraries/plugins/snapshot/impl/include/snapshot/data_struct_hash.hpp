#pragma once

#include <fc/io/raw.hpp>
#include <fc/crypto/ripemd160.hpp>
#include <fc/reflect/reflect.hpp>

#include <iostream>

namespace scorum {
namespace snapshot {

template <typename Stream, typename Class> struct pack_data_struct_visitor
{
    pack_data_struct_visitor(const Class& _c, Stream& _s)
        : c(_c)
        , s(_s)
    {
    }

    template <typename T, typename C, T(C::*p)> void operator()(const char* name) const
    {
        fc::raw::pack(s, name);
        fc::raw::pack(s, sizeof(c.*p));
    }

private:
    const Class& c;
    Stream& s;
};

template <class ObjectType> fc::ripemd160 get_data_struct_hash(const ObjectType& etalon)
{
    fc::ripemd160::encoder hash_enc;
    fc::reflector<ObjectType>::visit(pack_data_struct_visitor<fc::ripemd160::encoder, ObjectType>(etalon, hash_enc));
    return hash_enc.result();
}
}
}
