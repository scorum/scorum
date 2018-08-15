#include <boost/test/unit_test.hpp>
#include <scorum/chain/dba/db_accessor_i.hpp>
#include <scorum/chain/dba/db_accessor_factory.hpp>
#include <scorum/chain/schema/game_object.hpp>
#include <scorum/chain/database/database.hpp>

#include <hippomocks.h>

#include "defines.hpp"
#include "object_wrapper.hpp"

namespace {
using namespace scorum::chain;
using namespace scorum::chain::dba;

BOOST_FIXTURE_TEST_SUITE(db_accessors_tests, shared_memory_fixture)

BOOST_AUTO_TEST_CASE(db_accessors_factory_mock_tests)
{
    // Preparing dba_factory
    MockRepository mocks;
    db_accessor_factory dba_factory{ *mocks.Mock<database>() };

    // Mocking db_accessor
    db_accessor_i<game_object> dba_i{ db_accessor_mock<game_object>{} };
    mocks.OnCallFunc(get_db_accessor<game_object>).ReturnByRef(dba_i);

    // Mocking is_empty for a particular db_accessor
    auto& dba_mock = dba_i.get_accessor_inst<db_accessor_mock<game_object>>();
    dba_mock.mock(&db_accessor_mock<game_object>::is_empty, [] { return true; });

    // Emulating dba_factory usage
    db_accessor_i<game_object>& dba = dba_factory.get_dba<game_object>();
    auto result = dba.is_empty();
    BOOST_CHECK(result);
}

BOOST_AUTO_TEST_CASE(db_accessors_mock_tests)
{
    using mock_t = db_accessor_mock<game_object>;
    db_accessor_i<game_object> dba{ mock_t{} };
    auto& dba_mock = dba.get_accessor_inst<mock_t>();

    const auto& obj = create_object<game_object>(shm, [](game_object& o) { o.moderator = "moder"; });

    bool get_was_mocked = false;
    dba_mock.mock(&mock_t::get, [&]() -> decltype(auto) {
        get_was_mocked = true;
        return (obj);
    });

    bool create_was_mocked = false;
    dba_mock.mock(&mock_t::create, [&](const mock_t::modifier_type& m) -> decltype(auto) {
        create_was_mocked = true;
        return (obj);
    });

    bool update1_was_mocked = false;
    dba_mock.mock((void (mock_t::*)(const mock_t::modifier_type&)) & mock_t::update,
                  [&](const mock_t::modifier_type&) { update1_was_mocked = true; });

    bool update2_was_mocked = false;
    dba_mock.mock((void (mock_t::*)(const mock_t::object_type&, const mock_t::modifier_type&)) & mock_t::update,
                  [&](const mock_t::object_type&, const mock_t::modifier_type&) { update2_was_mocked = true; });

    bool remove1_was_mocked = false;
    dba_mock.mock((void (mock_t::*)()) & mock_t::remove, [&] { //
        remove1_was_mocked = true;
    });

    bool remove2_was_mocked = false;
    dba_mock.mock((void (mock_t::*)(const mock_t::object_type&)) & mock_t::remove,
                  [&](const mock_t::object_type&) { remove2_was_mocked = true; });

    bool is_empty_was_mocked = false;
    dba_mock.mock(&mock_t::is_empty, [&] {
        is_empty_was_mocked = true;
        return false;
    });

    bool get_by_was_mocked = false;
    dba_mock.mock(&mock_t::get_by<by_name, fc::shared_string>, [&](const fc::shared_string& key) -> decltype(auto) {
        get_by_was_mocked = true;
        return (obj);
    });

    bool find_by_was_mocked = false;
    dba_mock.mock(&mock_t::find_by<by_name, fc::shared_string>, [&](const fc::shared_string& key) -> decltype(auto) {
        find_by_was_mocked = true;
        return &obj;
    });

    using unbounded_type
        = boost::multi_index::detail::unbounded_helper (&)(boost::multi_index::detail::unbounded_helper);

    bool get_range_by_was_mocked = false;
    dba_mock.mock(&mock_t::get_range_by<by_name, unbounded_type, unbounded_type>, [&](auto&&, auto&&) {
        get_range_by_was_mocked = true;
        return std::vector<mock_t::object_cref_type>{ std::cref(obj) };
    });

    const auto& o1 = dba.create([](mock_t::object_type&) {});
    dba.update([](mock_t::object_type&) {});
    dba.update(obj, [](mock_t::object_type&) {});
    dba.remove();
    dba.remove(obj);
    dba.is_empty();
    const auto& o2 = dba.get();
    fc::shared_string s("", shm.get_allocator<fc::shared_string>());
    const auto& o3 = dba.get_by<by_name>(s);
    const auto* o4 = dba.find_by<by_name>(s);
    auto os5 = dba.get_range_by<by_name>(::boost::multi_index::unbounded, ::boost::multi_index::unbounded);

    BOOST_CHECK(create_was_mocked);
    BOOST_CHECK(update1_was_mocked);
    BOOST_CHECK(update2_was_mocked);
    BOOST_CHECK(remove1_was_mocked);
    BOOST_CHECK(remove2_was_mocked);
    BOOST_CHECK(is_empty_was_mocked);
    BOOST_CHECK(get_was_mocked);
    BOOST_CHECK(get_by_was_mocked);
    BOOST_CHECK(find_by_was_mocked);
    BOOST_CHECK(get_range_by_was_mocked);

    BOOST_CHECK_EQUAL(o1.moderator, "moder");
    BOOST_CHECK_EQUAL(o2.moderator, "moder");
    BOOST_CHECK_EQUAL(o3.moderator, "moder");
    BOOST_CHECK_EQUAL(o4->moderator, "moder");
    BOOST_CHECK_EQUAL(os5[0].get().moderator, "moder");
}

BOOST_AUTO_TEST_SUITE_END()
}
