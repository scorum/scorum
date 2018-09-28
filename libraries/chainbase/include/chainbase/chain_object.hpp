#pragma once

#include <fc/variant.hpp>
#include <boost/preprocessor.hpp>

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
        friend bool operator<=(const oid& a, const oid& b)
        {
            return a._id <= b._id;
        }
        friend bool operator>=(const oid& a, const oid& b)
        {
            return a._id >= b._id;
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
        static constexpr uint16_t type_id = TypeNumber;
    };

//Default constructor for object in index
//Usage:
//  if object has members which use internal memory allocation(e.g. via new operator) pass them to the second macro parameter as a sequence,
//  otherwise pass BOOST_PP_SEQ_NIL or use CHAINBASE_DEFAULT_CONSTRUCTOR macro
//
//  class Example
//  {
//  public:
//    CHAINBASE_DEFAULT_DYNAMIC_CONSTRUCTOR(Example, (str1)(str2))
//
//    int i; // No need to pass i to macro? because it has no memory magement inside
//    std::string str1;
//    std::string str2;
//  };
//
#define CHAINBASE_COLON() :

#define CHAINBASE_COLON_IF_NOT(n) BOOST_PP_IF(BOOST_PP_EQUAL(n, 0), CHAINBASE_COLON, BOOST_PP_EMPTY)()

#define INIT_DYNAMIC_MEMBER(r, initval, n, param) CHAINBASE_COLON_IF_NOT(n) BOOST_PP_COMMA_IF(n) param(initval)

#define CHAINBASE_DEFAULT_DYNAMIC_CONSTRUCTOR(OBJECT_TYPE, DYNAMIC_MEMBERS)                                            \
    OBJECT_TYPE() = delete;                                                                                            \
    template <typename Constructor, typename Allocator>                                                                \
    OBJECT_TYPE(Constructor&& c, Allocator&& a)                                                                        \
    BOOST_PP_SEQ_FOR_EACH_I(INIT_DYNAMIC_MEMBER, a, DYNAMIC_MEMBERS)                                                   \
    {                                                                                                                  \
        c(*this);                                                                                                      \
    }

#define CHAINBASE_DEFAULT_CONSTRUCTOR(OBJECT_TYPE)  CHAINBASE_DEFAULT_DYNAMIC_CONSTRUCTOR(OBJECT_TYPE, BOOST_PP_SEQ_NIL)

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
