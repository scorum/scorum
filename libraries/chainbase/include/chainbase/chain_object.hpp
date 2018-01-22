#pragma once

#include <fc/variant.hpp>

// clang-format off

namespace chainbase {

    /**
     *  Object ID type that includes the type of the object it references
     */
    template <typename T> class oid
    {
    public:
        oid(int64_t i = 0)
            : _id(i)
        {
        }

        oid& operator++()
        {
            ++_id;
            return *this;
        }

        friend bool operator<(const oid& a, const oid& b)
        {
            return a._id < b._id;
        }
        friend bool operator>(const oid& a, const oid& b)
        {
            return a._id > b._id;
        }
        friend bool operator==(const oid& a, const oid& b)
        {
            return a._id == b._id;
        }
        friend bool operator!=(const oid& a, const oid& b)
        {
            return a._id != b._id;
        }
        int64_t _id = 0;
    };

    template <uint16_t TypeNumber, typename Derived> struct object
    {
        typedef oid<Derived> id_type;
        static const uint16_t type_id = TypeNumber;
    };

#define CHAINBASE_DEFAULT_CONSTRUCTOR(OBJECT_TYPE)                                                                     \
    template <typename Constructor, typename Allocator> OBJECT_TYPE(Constructor&& c, Allocator&&)                      \
    {                                                                                                                  \
        c(*this);                                                                                                      \
    }

} // namespace chainbase

namespace fc {

    template <typename T> void to_variant(const chainbase::oid<T>& var, variant& vo)
    {
        vo = var._id;
    }

    template <typename T> void from_variant(const variant& vo, chainbase::oid<T>& var)
    {
        var._id = vo.as_int64();
    }

    namespace raw {

        template <typename Stream, typename T> inline void pack(Stream& s, const chainbase::oid<T>& id)
        {
            s.write((const char*)&id._id, sizeof(id._id));
        }
        template <typename Stream, typename T> inline void unpack(Stream& s, chainbase::oid<T>& id)
        {
            s.read((char*)&id._id, sizeof(id._id));
        }

    } // namespace raw
} // namespace fc

// clang-format on
