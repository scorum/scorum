#include <boost/test/unit_test.hpp>

#include <type_traits>

#include <fc/log/logger.hpp>

#ifdef FUSION_MAX_VECTOR_SIZE
#undef FUSION_MAX_VECTOR_SIZE
#endif
#define FUSION_MAX_VECTOR_SIZE 20
#include <boost/fusion/container/vector.hpp>
#include <boost/fusion/sequence.hpp>

namespace any_cast_tests {

template <uint16_t TypeNumber> struct object
{
    static constexpr uint16_t type_id = TypeNumber;
};

struct empty_type
{
};

struct a_type : public object<1>
{
};

struct b_type : public object<5>
{
};

struct c_type : public object<9>
{
};

struct d_type : public object<10>
{
};

template <uint16_t Id> struct get_object_type
{
    typedef empty_type type;
};

template <> struct get_object_type<a_type::type_id>
{
    typedef a_type type;
};

template <> struct get_object_type<b_type::type_id>
{
    typedef b_type type;
};

template <> struct get_object_type<c_type::type_id>
{
    typedef c_type type;
};

template <> struct get_object_type<d_type::type_id>
{
    typedef d_type type;
};

template <typename T, typename U> struct decay_equiv : std::is_same<typename std::decay<T>::type, U>::type
{
};

static struct find_object_type
{
    boost::fusion::vector<get_object_type<0>,
                          get_object_type<1>,
                          get_object_type<2>,
                          get_object_type<3>,
                          get_object_type<4>,
                          get_object_type<5>,
                          get_object_type<6>,
                          get_object_type<7>,
                          get_object_type<8>,
                          get_object_type<9>,
                          get_object_type<10>,
                          get_object_type<11>,
                          get_object_type<12>,
                          get_object_type<13>,
                          get_object_type<14>,
                          get_object_type<15>,
                          get_object_type<16>,
                          get_object_type<17>,
                          get_object_type<18>,
                          get_object_type<19>>
        space_0_9;
    boost::fusion::vector<get_object_type<20>,
                          get_object_type<21>,
                          get_object_type<22>,
                          get_object_type<23>,
                          get_object_type<24>,
                          get_object_type<25>,
                          get_object_type<26>,
                          get_object_type<27>,
                          get_object_type<28>,
                          get_object_type<29>,
                          get_object_type<30>,
                          get_object_type<31>,
                          get_object_type<32>,
                          get_object_type<33>,
                          get_object_type<34>,
                          get_object_type<35>,
                          get_object_type<36>,
                          get_object_type<37>,
                          get_object_type<38>,
                          get_object_type<39>>
        space_10_19;

    template <typename Visitor, typename Container, int N, int End> struct find_object_type_impl
    {
        bool operator()(Visitor& v, Container& t, const int id) const
        {
            if (id == N)
            {
                auto& tp = boost::fusion::at_c<N>(t);
                using object_type = typename std::decay<decltype(tp)>::type;
                if ((decay_equiv<typename object_type::type, empty_type>::value))
                    return false;

                typename object_type::type* objp = nullptr;
                return v(objp);
            }
            return find_object_type_impl<Visitor, Container, N + 1, End>()(v, t, id);
        }
    };

    template <typename Visitor, typename Container, int N> struct find_object_type_impl<Visitor, Container, N, N>
    {
        bool operator()(Visitor&, Container&, const int) const
        {
            return false;
        }
    };

    template <typename Visitor> bool apply(Visitor& v, int id)
    {
        if (id < 10)
        {
            auto& space = space_0_9;
            return find_object_type_impl<Visitor, std::decay<decltype(space)>::type, 0,
                                         boost::fusion::result_of::size<std::decay<decltype(space)>::type>::type::
                                             value>()(v, space, id);
        }
        else if (id < 20)
        {
            auto& space = space_10_19;
            return find_object_type_impl<Visitor, std::decay<decltype(space)>::type, 0,
                                         boost::fusion::result_of::size<std::decay<decltype(space)>::type>::type::
                                             value>()(v, space, id - 10);
        }
        return false;
    }
} object_types;

struct object_visitor
{
    bool operator()(const empty_type*) const
    {
        return false;
    }

    template <class T> bool operator()(const T*) const
    {
        auto object_name = boost::core::demangle(typeid(T).name());
        auto id = T::type_id;
        ilog("${t}: ${id}", ("t", object_name)("id", id));
        return true;
    }
};

BOOST_AUTO_TEST_SUITE(any_cast_tests)

BOOST_AUTO_TEST_CASE(get_object_type_by_id)
{
    BOOST_REQUIRE((decay_equiv<get_object_type<1>::type, a_type>::value));
    BOOST_REQUIRE((decay_equiv<get_object_type<5>::type, b_type>::value));
    BOOST_REQUIRE((decay_equiv<get_object_type<9>::type, c_type>::value));
    BOOST_REQUIRE((decay_equiv<get_object_type<10>::type, d_type>::value));

    object_visitor v;

    BOOST_CHECK(!object_types.apply(v, 0));

    uint16_t id = 1;
    BOOST_CHECK(object_types.apply(v, id));

    id = 4;
    BOOST_CHECK(!object_types.apply(v, id));

    id = 5;
    BOOST_CHECK(object_types.apply(v, id));

    id = 9;
    BOOST_CHECK(object_types.apply(v, id));

    id = 10;
    BOOST_CHECK(object_types.apply(v, id));

    id = 1111;
    BOOST_CHECK(!object_types.apply(v, id));
}

BOOST_AUTO_TEST_SUITE_END()
}
