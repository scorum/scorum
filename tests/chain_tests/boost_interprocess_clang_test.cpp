#include <boost/test/unit_test.hpp>
#include <iostream>
#include <functional>
#include <boost/container/string.hpp>
#include <boost/interprocess/managed_mapped_file.hpp>
#include <boost/interprocess/allocators/allocator.hpp>
#include <boost/multi_index_container.hpp>
#include <boost/multi_index/member.hpp>
#include <boost/multi_index/ordered_index.hpp>
#include <boost/multi_index/mem_fun.hpp>

namespace {
namespace bip = boost::interprocess;
namespace bmi = boost::multi_index;

template <typename T> using shared_allocator = bip::allocator<T, bip::managed_mapped_file::segment_manager>;
using shared_string = boost::container::basic_string<char, std::char_traits<char>, shared_allocator<char>>;
template <typename T, typename... Args>
using shared_multi_index_container = boost::multi_index_container<T, Args..., shared_allocator<T>>;

struct person
{
    int id;
    shared_string name;

    person(const shared_allocator<char>& alloc)
        : id(0)
        , name(alloc)
    {
    }
};

struct by_id;
using person_idx
    = shared_multi_index_container<person,
                                   bmi::indexed_by<bmi::ordered_unique<bmi::tag<by_id>,
                                                                       bmi::member<person, int, &person::id>>>>;

#define ENABLE_CLANG_TESTS 0
#if ENABLE_CLANG_TESTS

BOOST_AUTO_TEST_SUITE(clang_migration_tests)

BOOST_AUTO_TEST_CASE(boodt_multiindex_test)
{
    bip::managed_mapped_file seg(bip::open_or_create, "./file.bin", 65536);
    person_idx* idx = seg.find_or_construct<person_idx>("person_idx")(
        person_idx::ctor_args_list(), person_idx::allocator_type(seg.get_segment_manager()));

    auto found_it = idx->find(1);

    BOOST_CHECK(found_it == idx->end());
}

BOOST_AUTO_TEST_SUITE_END()

#endif
}
