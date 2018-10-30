#include <boost/test/unit_test.hpp>

#include <scorum/chain/dba/db_accessor.hpp>
#include <scorum/chain/dba/db_accessor_factory.hpp>
#include <scorum/chain/schema/game_object.hpp>
#include <scorum/chain/schema/comment_objects.hpp>
#include <scorum/utils/any_range.hpp>

#include "defines.hpp"
#include "database_default_integration.hpp"

#include <boost/range/adaptor/reversed.hpp>
#include <boost/uuid/uuid_io.hpp>

namespace {
using namespace std::string_literals;
using namespace scorum;
using namespace scorum::chain;
using namespace scorum::chain::dba;

BOOST_FIXTURE_TEST_SUITE(db_accessors_tests, database_fixture::database_default_integration_fixture)

BOOST_AUTO_TEST_CASE(db_accessors_tests)
{
    db_accessor_factory dba_factory{ static_cast<dba::db_index&>(db) };
    db_accessor<game_object>& dba = dba_factory.get_dba<game_object>();

    BOOST_CHECK(dba.is_empty());

    const auto& o1 = dba.create([](game_object& o) { o.uuid = { 1 }; });
    BOOST_CHECK_EQUAL(o1.uuid, uuid_type{ 1 });

    dba.update([](game_object& o) { o.uuid = { 2 }; });
    BOOST_CHECK_EQUAL(o1.uuid, uuid_type{ 2 });

    dba.update(o1, [](game_object& o) { o.uuid = { 1 }; });
    BOOST_CHECK_EQUAL(o1.uuid, uuid_type{ 1 });

    BOOST_CHECK(!dba.is_empty());

    const auto& o3 = dba.get_by<by_uuid>(o1.uuid);
    BOOST_CHECK_EQUAL(o3.uuid, uuid_type{ 1 });

    const auto* o4 = dba.find_by<by_uuid>(o1.uuid);
    BOOST_CHECK_EQUAL(o4->uuid, uuid_type{ 1 });

    const auto* o5 = dba.find_by<by_uuid>(uuid_type{ 2 });
    BOOST_CHECK(o5 == nullptr);

    const auto& o6 = dba.create([](game_object& o) { o.uuid = { 2 }; });

    dba.remove();
    dba.remove(o6);

    BOOST_CHECK(dba.is_empty());
}

BOOST_AUTO_TEST_CASE(get_range_by_bounds_tests)
{
    BOOST_TEST_MESSAGE("Preparing index data...");

    db_accessor_factory dba_factory{ static_cast<dba::db_index&>(db) };
    db_accessor<comment_object>& dba = dba_factory.get_dba<comment_object>();

    BOOST_CHECK(dba.is_empty());

    // clang-format off
    dba.create([&](comment_object& o) { o.author = "test_0"; fc::from_string(o.permlink, "pl_0"); });
    dba.create([&](comment_object& o) { o.author = "test_1"; fc::from_string(o.permlink, "pl_1"); });
    dba.create([&](comment_object& o) { o.author = "test_1"; fc::from_string(o.permlink, "pl_2"); });
    dba.create([&](comment_object& o) { o.author = "test_2"; fc::from_string(o.permlink, "pl_3"); });
    dba.create([&](comment_object& o) { o.author = "test_3"; fc::from_string(o.permlink, "pl_4"); });
    dba.create([&](comment_object& o) { o.author = "test_4"; fc::from_string(o.permlink, "pl_5"); });
    dba.create([&](comment_object& o) { o.author = "test_4"; fc::from_string(o.permlink, "pl_6"); });
    dba.create([&](comment_object& o) { o.author = "test_5"; fc::from_string(o.permlink, "pl_7"); });
    // clang-format on

    BOOST_TEST_MESSAGE("Checking...");

    auto check = [](utils::bidir_range<const comment_object> rng, const std::string& fst, const std::string& lst,
                    size_t size) {
        BOOST_TEST_MESSAGE("Checking: lower=" << fst << "; upper=" << lst << "; size=" << size << ";");
        BOOST_REQUIRE_EQUAL(std::distance(rng.begin(), rng.end()), size);
        BOOST_CHECK_EQUAL(rng.front().author, fst);
        BOOST_CHECK_EQUAL(rng.back().author, lst);
    };

    check(dba.get_range_by<by_permlink>("test_1"s < _x, _x < "test_4"s), "test_2", "test_3", 2);
    check(dba.get_range_by<by_permlink>("test_1"s <= _x, _x < "test_4"s), "test_1", "test_3", 4);
    check(dba.get_range_by<by_permlink>("test_1"s < _x, _x <= "test_4"s), "test_2", "test_4", 4);
    check(dba.get_range_by<by_permlink>("test_1"s <= _x, _x <= "test_4"s), "test_1", "test_4", 6);
    check(dba.get_range_by<by_permlink>("test_1"s < _x, dba::unbounded), "test_2", "test_5", 5);
    check(dba.get_range_by<by_permlink>("test_1"s <= _x, dba::unbounded), "test_1", "test_5", 7);
    check(dba.get_range_by<by_permlink>(dba::unbounded, _x < "test_4"s), "test_0", "test_3", 5);
    check(dba.get_range_by<by_permlink>(dba::unbounded, _x <= "test_4"s), "test_0", "test_4", 7);
    check(dba.get_range_by<by_permlink>(dba::unbounded, dba::unbounded), "test_0", "test_5", 8);

    check(dba.get_range_by<by_permlink>("test_1"s > _x, _x > "test_4"s), "test_2", "test_3", 2);
    check(dba.get_range_by<by_permlink>("test_1"s >= _x, _x > "test_4"s), "test_1", "test_3", 4);
    check(dba.get_range_by<by_permlink>("test_1"s > _x, _x >= "test_4"s), "test_2", "test_4", 4);
    check(dba.get_range_by<by_permlink>("test_1"s >= _x, _x >= "test_4"s), "test_1", "test_4", 6);
    check(dba.get_range_by<by_permlink>("test_1"s > _x, dba::unbounded), "test_2", "test_5", 5);
    check(dba.get_range_by<by_permlink>("test_1"s >= _x, dba::unbounded), "test_1", "test_5", 7);
    check(dba.get_range_by<by_permlink>(dba::unbounded, _x > "test_4"s), "test_0", "test_3", 5);
    check(dba.get_range_by<by_permlink>(dba::unbounded, _x >= "test_4"s), "test_0", "test_4", 7);
    check(dba.get_range_by<by_permlink>(dba::unbounded, dba::unbounded), "test_0", "test_5", 8);

    // NOTE: there is no difference between indexes ordered in asc or desc order. Each index type works with both
    // 'A < _x < B' and 'A > _x > B' even is the first one doesn't fit descending ordered indexes and the second one
    // doesn't fit ascending ordered indexes.
    //
    // The meaning of these cmp operators is not actually 'less'/'greather' or 'less or equal'/'greather or equal'
    // but 'do not include'/'include' the boundaries
}

BOOST_AUTO_TEST_SUITE_END()
}
