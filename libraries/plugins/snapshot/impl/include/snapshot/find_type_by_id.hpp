#pragma once

#include <boost/preprocessor/iteration/local.hpp>
#include <boost/preprocessor/repetition/enum.hpp>
#include <boost/preprocessor/arithmetic/add.hpp>
#include <boost/preprocessor/arithmetic/mul.hpp>
#include <boost/preprocessor/arithmetic/sub.hpp>

#ifdef FUSION_MAX_VECTOR_SIZE
#undef FUSION_MAX_VECTOR_SIZE
#endif
#define FUSION_MAX_VECTOR_SIZE 20

#include <boost/fusion/container/vector.hpp>
#include <boost/fusion/sequence.hpp>

#include <type_traits>

#include <snapshot/object_types.hpp>
#include <snapshot/serializer_extensions.hpp>

#define BLOCK_SIZE FUSION_MAX_VECTOR_SIZE

static_assert(BLOCK_SIZE <= FUSION_MAX_VECTOR_SIZE,
              "Can't set BLOCK_SIZE more then "
              "boost::fusion::vector capacity");

namespace scorum {
namespace snapshot {
struct find_object_type
{
// clang-format off
#define OBJECT_TYPES_VECTOR(k)                                \
    boost::fusion::vector<                                    \
                          get_object_type<BLOCK_SIZE*k + 0>,  \
                          get_object_type<BLOCK_SIZE*k + 1>,  \
                          get_object_type<BLOCK_SIZE*k + 2>,  \
                          get_object_type<BLOCK_SIZE*k + 3>,  \
                          get_object_type<BLOCK_SIZE*k + 4>,  \
                          get_object_type<BLOCK_SIZE*k + 5>,  \
                          get_object_type<BLOCK_SIZE*k + 6>,  \
                          get_object_type<BLOCK_SIZE*k + 7>,  \
                          get_object_type<BLOCK_SIZE*k + 8>,  \
                          get_object_type<BLOCK_SIZE*k + 9>,  \
                          get_object_type<BLOCK_SIZE*k + 10>, \
                          get_object_type<BLOCK_SIZE*k + 11>, \
                          get_object_type<BLOCK_SIZE*k + 12>, \
                          get_object_type<BLOCK_SIZE*k + 13>, \
                          get_object_type<BLOCK_SIZE*k + 14>, \
                          get_object_type<BLOCK_SIZE*k + 15>, \
                          get_object_type<BLOCK_SIZE*k + 16>, \
                          get_object_type<BLOCK_SIZE*k + 17>, \
                          get_object_type<BLOCK_SIZE*k + 18>, \
                          get_object_type<BLOCK_SIZE*k + 19>  \
    > BOOST_PP_CAT(space_, k)

    OBJECT_TYPES_VECTOR(0);
    OBJECT_TYPES_VECTOR(1);
    OBJECT_TYPES_VECTOR(2);
    OBJECT_TYPES_VECTOR(3);
    OBJECT_TYPES_VECTOR(4);
    OBJECT_TYPES_VECTOR(5);
    OBJECT_TYPES_VECTOR(6);
    OBJECT_TYPES_VECTOR(7);
    OBJECT_TYPES_VECTOR(8);
    OBJECT_TYPES_VECTOR(9);
    OBJECT_TYPES_VECTOR(10);
    OBJECT_TYPES_VECTOR(11);
    OBJECT_TYPES_VECTOR(12);
    OBJECT_TYPES_VECTOR(13);
    OBJECT_TYPES_VECTOR(14);
    OBJECT_TYPES_VECTOR(15);
    OBJECT_TYPES_VECTOR(16);
    OBJECT_TYPES_VECTOR(17);
    OBJECT_TYPES_VECTOR(18);
    OBJECT_TYPES_VECTOR(19);
    OBJECT_TYPES_VECTOR(20);
    OBJECT_TYPES_VECTOR(21);
    OBJECT_TYPES_VECTOR(22);
    OBJECT_TYPES_VECTOR(23);
    OBJECT_TYPES_VECTOR(24);
    OBJECT_TYPES_VECTOR(25);
    OBJECT_TYPES_VECTOR(26);
    OBJECT_TYPES_VECTOR(27);
    OBJECT_TYPES_VECTOR(28);
    OBJECT_TYPES_VECTOR(29);
    OBJECT_TYPES_VECTOR(30);
    OBJECT_TYPES_VECTOR(31);
    OBJECT_TYPES_VECTOR(32);
    OBJECT_TYPES_VECTOR(33);
    OBJECT_TYPES_VECTOR(34);
    OBJECT_TYPES_VECTOR(35);
    OBJECT_TYPES_VECTOR(36);
    OBJECT_TYPES_VECTOR(37);
    OBJECT_TYPES_VECTOR(38);
    OBJECT_TYPES_VECTOR(39);
    OBJECT_TYPES_VECTOR(40);
    OBJECT_TYPES_VECTOR(41);
    OBJECT_TYPES_VECTOR(42);
    OBJECT_TYPES_VECTOR(43);
    OBJECT_TYPES_VECTOR(44);
    OBJECT_TYPES_VECTOR(45);
    OBJECT_TYPES_VECTOR(46);
    OBJECT_TYPES_VECTOR(47);
    OBJECT_TYPES_VECTOR(48);
    OBJECT_TYPES_VECTOR(49); //< Maximum index = 50 * BLOCK_SIZE = 1000
    // clang-format on

    // Following pretty macroses do not work because of restrictions hardcoded in BOOST (256 for all iterations)
    // ...
    // #define BOOST_PP_LOCAL_MACRO(n) OBJECT_TYPES_VECTOR(_1, n, _2);
    // #define BOOST_PP_LOCAL_LIMITS (0, BLOCK_COUNT - 1)
    // #include BOOST_PP_LOCAL_ITERATE()
    // ...
    // #define BOOST_PP_LOCAL_MACRO(n) FIND_OBJECT_TYPE_IN_SECTION(_1, n, _2)
    // #define BOOST_PP_LOCAL_LIMITS (0, BLOCK_COUNT - 1)
    // #include BOOST_PP_LOCAL_ITERATE()

    template <typename T, typename U> struct decay_equiv : std::is_same<typename std::decay<T>::type, U>::type
    {
    };

    template <typename Visitor, typename Container, int N, int End> struct find_object_type_impl
    {
        bool operator()(const Visitor& v, Container& t, const int id) const
        {
            if (id == N)
            {
                auto& tp = boost::fusion::at_c<N>(t);
                using object_type = typename std::decay<decltype(tp)>::type;
                if ((decay_equiv<typename object_type::type, empty_object_type>::value))
                    return false;

                typename object_type::type* objp = nullptr;
                v(objp); // trow exception
                return true;
            }
            return find_object_type_impl<Visitor, Container, N + 1, End>()(v, t, id);
        }
    };

    template <typename Visitor, typename Container, int N> struct find_object_type_impl<Visitor, Container, N, N>
    {
        bool operator()(const Visitor&, Container&, const int) const
        {
            return false;
        }
    };

    template <typename Visitor> bool apply(const Visitor& v, int id)
    {
// clang-format off
#define FIND_OBJECT_TYPE_IN_SECTION(k)                                                                              \
        if (id < (k + 1) * BLOCK_SIZE)                                                                               \
        {                                                                                                            \
            auto& space = BOOST_PP_CAT(space_, k);                                                                   \
            return find_object_type_impl<Visitor, std::decay<decltype(space)>::type, 0,                              \
                                         boost::fusion::result_of::size<std::decay<decltype(space)>::type>::type::   \
                                         value>()(v, BOOST_PP_CAT(space_, k), id - BLOCK_SIZE*k);                    \
        }                                                                                                            \
        else
        FIND_OBJECT_TYPE_IN_SECTION(0)
        FIND_OBJECT_TYPE_IN_SECTION(1)
        FIND_OBJECT_TYPE_IN_SECTION(2)
        FIND_OBJECT_TYPE_IN_SECTION(3)
        FIND_OBJECT_TYPE_IN_SECTION(4)
        FIND_OBJECT_TYPE_IN_SECTION(5)
        FIND_OBJECT_TYPE_IN_SECTION(6)
        FIND_OBJECT_TYPE_IN_SECTION(7)
        FIND_OBJECT_TYPE_IN_SECTION(8)
        FIND_OBJECT_TYPE_IN_SECTION(9)
        FIND_OBJECT_TYPE_IN_SECTION(10)
        FIND_OBJECT_TYPE_IN_SECTION(11)
        FIND_OBJECT_TYPE_IN_SECTION(12)
        FIND_OBJECT_TYPE_IN_SECTION(13)
        FIND_OBJECT_TYPE_IN_SECTION(14)
        FIND_OBJECT_TYPE_IN_SECTION(15)
        FIND_OBJECT_TYPE_IN_SECTION(16)
        FIND_OBJECT_TYPE_IN_SECTION(17)
        FIND_OBJECT_TYPE_IN_SECTION(18)
        FIND_OBJECT_TYPE_IN_SECTION(19)
        FIND_OBJECT_TYPE_IN_SECTION(20)
        FIND_OBJECT_TYPE_IN_SECTION(21)
        FIND_OBJECT_TYPE_IN_SECTION(22)
        FIND_OBJECT_TYPE_IN_SECTION(23)
        FIND_OBJECT_TYPE_IN_SECTION(24)
        FIND_OBJECT_TYPE_IN_SECTION(25)
        FIND_OBJECT_TYPE_IN_SECTION(26)
        FIND_OBJECT_TYPE_IN_SECTION(27)
        FIND_OBJECT_TYPE_IN_SECTION(28)
        FIND_OBJECT_TYPE_IN_SECTION(29)
        FIND_OBJECT_TYPE_IN_SECTION(30)
        FIND_OBJECT_TYPE_IN_SECTION(31)
        FIND_OBJECT_TYPE_IN_SECTION(32)
        FIND_OBJECT_TYPE_IN_SECTION(33)
        FIND_OBJECT_TYPE_IN_SECTION(34)
        FIND_OBJECT_TYPE_IN_SECTION(35)
        FIND_OBJECT_TYPE_IN_SECTION(36)
        FIND_OBJECT_TYPE_IN_SECTION(37)
        FIND_OBJECT_TYPE_IN_SECTION(38)
        FIND_OBJECT_TYPE_IN_SECTION(39)
        FIND_OBJECT_TYPE_IN_SECTION(40)
        FIND_OBJECT_TYPE_IN_SECTION(41)
        FIND_OBJECT_TYPE_IN_SECTION(42)
        FIND_OBJECT_TYPE_IN_SECTION(43)
        FIND_OBJECT_TYPE_IN_SECTION(44)
        FIND_OBJECT_TYPE_IN_SECTION(45)
        FIND_OBJECT_TYPE_IN_SECTION(46)
        FIND_OBJECT_TYPE_IN_SECTION(47)
        FIND_OBJECT_TYPE_IN_SECTION(48)
        FIND_OBJECT_TYPE_IN_SECTION(49)
        // clang-format on
        {
            return false;
        }
    }
};
}
}
