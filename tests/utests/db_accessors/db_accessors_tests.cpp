#ifdef DEBUG // hippomocks doesn't work properly in release build in this case

#include <boost/test/unit_test.hpp>
#include <scorum/chain/dba/db_accessor.hpp>
#include <scorum/chain/dba/db_accessor_factory.hpp>
#include <scorum/chain/schema/game_object.hpp>
#include <scorum/chain/database/database.hpp>
#include <scorum/utils/any_range.hpp>

#include <hippomocks.h>

#include "defines.hpp"
#include "object_wrapper.hpp"

namespace {
using namespace std::string_literals;
using namespace scorum;
using namespace scorum::chain;
using namespace scorum::chain::dba;
using namespace scorum::chain::dba::detail;

BOOST_FIXTURE_TEST_SUITE(db_accessors_tests, shared_memory_fixture)

BOOST_AUTO_TEST_CASE(db_accessors_mock_tests)
{
    MockRepository mocks;
    auto obj = create_object<game_object>(shm, [](game_object& o) { fc::from_string(o.name, "name"); });
    std::vector<std::reference_wrapper<game_object>> vec = { obj };

    bool get_was_mocked = false;
    mocks.OnCallFunc(dba::detail::get_single<game_object>).Do([&](db_index&) -> const game_object& {
        get_was_mocked = true;
        return (obj);
    });

    bool create_was_mocked = false;
    mocks.OnCallFunc(dba::detail::create<game_object>)
        .Do([&](db_index&, db_accessor<game_object>::modifier_type) -> const game_object& {
            create_was_mocked = true;
            return (obj);
        });

    bool update1_was_mocked = false;
    mocks.OnCallFunc(dba::detail::update<game_object>)
        .Do([&](db_index&, const game_object&, db_accessor<game_object>::modifier_type) -> const game_object& {
            update1_was_mocked = true;
            return (obj);
        });

    bool update2_was_mocked = false;
    mocks.OnCallFunc(dba::detail::update_single<game_object>)
        .Do([&](db_index&, db_accessor<game_object>::modifier_type) -> const game_object& {
            update2_was_mocked = true;
            return (obj);
        });

    bool remove1_was_mocked = false;
    mocks.OnCallFunc(dba::detail::remove<game_object>).Do([&](db_index&, const game_object&) {
        remove1_was_mocked = true;
    });

    bool remove2_was_mocked = false;
    mocks.OnCallFunc(dba::detail::remove_single<game_object>).Do([&](db_index&) { remove2_was_mocked = true; });

    bool is_empty_was_mocked = false;
    mocks.OnCallFunc(dba::detail::is_empty<game_object>).Do([&](db_index&) {
        is_empty_was_mocked = true;
        return false;
    });

    bool get_by_was_mocked = false;
    mocks.OnCallFunc((dba::detail::get_by<game_object, by_name, fc::shared_string>))
        .Do([&](db_index&, const fc::shared_string&) -> const game_object& {
            get_by_was_mocked = true;
            return obj;
        });

    bool find_by_was_mocked = false;
    mocks.OnCallFunc((dba::detail::find_by<game_object, by_name, fc::shared_string>))
        .Do([&](db_index&, const fc::shared_string&) {
            find_by_was_mocked = true;
            return &obj;
        });

    bool get_range_by_was_mocked1 = false;
    using by_name_key = dba::index_key_type<game_object, by_name>;
    mocks.OnCallFunc((get_range_by<game_object, by_name, by_name_key>))
        .Do([&](db_index&, const bound<by_name_key>&, const bound<by_name_key>&) {
            get_range_by_was_mocked1 = true;
            return utils::bidir_range<game_object>{ vec };
        });

    bool get_range_by_was_mocked2 = false;
    mocks.OnCallFunc((get_range_by<game_object, by_name, std::string>))
        .Do([&](db_index&, const bound<std::string>&, const bound<std::string>&) {
            get_range_by_was_mocked2 = true;
            return utils::bidir_range<game_object>{ vec };
        });

    db_accessor<game_object> dba{ *mocks.Mock<database>() };

    const auto& o1 = dba.create([](game_object&) {});
    dba.update([](game_object&) {});
    dba.update(obj, [](game_object&) {});
    dba.remove();
    dba.remove(obj);
    dba.is_empty();
    const auto& o2 = dba.get();
    fc::shared_string s("", shm.get_allocator<fc::shared_string>());
    const auto& o3 = dba.get_by<by_name>(s);
    const auto* o4 = dba.find_by<by_name>(s);
    auto os5 = dba.get_range_by<by_name>(dba::unbounded, dba::unbounded);
    auto os6 = dba.get_range_by<by_name>("test"s <= _x, dba::unbounded);

    BOOST_CHECK(create_was_mocked);
    BOOST_CHECK(update1_was_mocked);
    BOOST_CHECK(update2_was_mocked);
    BOOST_CHECK(remove1_was_mocked);
    BOOST_CHECK(remove2_was_mocked);
    BOOST_CHECK(is_empty_was_mocked);
    BOOST_CHECK(get_was_mocked);
    BOOST_CHECK(get_by_was_mocked);
    BOOST_CHECK(find_by_was_mocked);
    BOOST_CHECK(get_range_by_was_mocked1);
    BOOST_CHECK(get_range_by_was_mocked2);

    BOOST_CHECK_EQUAL(o1.name, "name");
    BOOST_CHECK_EQUAL(o2.name, "name");
    BOOST_CHECK_EQUAL(o3.name, "name");
    BOOST_CHECK_EQUAL(o4->name, "name");
    BOOST_CHECK_EQUAL(os5.begin()->name, "name");
    BOOST_CHECK_EQUAL(os6.begin()->name, "name");
}

BOOST_AUTO_TEST_SUITE_END()
}

#endif
