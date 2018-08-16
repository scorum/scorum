#include <boost/test/unit_test.hpp>
#include <scorum/chain/dba/db_accessor_i.hpp>
#include <scorum/chain/dba/db_accessor_factory.hpp>
#include <scorum/chain/schema/game_object.hpp>

#include "defines.hpp"
#include "database_default_integration.hpp"

namespace {
using namespace scorum::chain;
using namespace scorum::chain::dba;

BOOST_FIXTURE_TEST_SUITE(db_accessors_tests, database_fixture::database_default_integration_fixture)

BOOST_AUTO_TEST_CASE(db_accessors_tests)
{
    db_accessor_factory dba_factory{ db };
    db_accessor_i<game_object>& dba = dba_factory.get_dba<game_object>();

    BOOST_CHECK(dba.is_empty());

    const auto& o1 = dba.create([](game_object& o) {
        o.name = "2";
        o.moderator = "moder";
    });
    BOOST_CHECK_EQUAL(o1.moderator, "moder");

    dba.update([](game_object& o) { o.moderator = "cartman"; });
    BOOST_CHECK_EQUAL(o1.moderator, "cartman");

    dba.update(o1, [](game_object& o) { o.moderator = "moder"; });
    BOOST_CHECK_EQUAL(o1.moderator, "moder");

    BOOST_CHECK(!dba.is_empty());

    const auto& o3 = dba.get_by<by_name>(o1.name);
    BOOST_CHECK_EQUAL(o3.moderator, "moder");

    const auto* o4 = dba.find_by<by_name>(o1.name);
    BOOST_CHECK_EQUAL(o4->moderator, "moder");

    const auto* o5 = dba.find_by<by_name>(std::string("cartman"));
    BOOST_CHECK(o5 == nullptr);

    const auto& o6 = dba.create([](game_object& o) {
        o.name = "1";
        o.moderator = "moder";
    });
    {
        auto vec = dba.get_range_by<by_name>(::boost::multi_index::unbounded, ::boost::multi_index::unbounded);
        BOOST_CHECK_EQUAL(vec.size(), 2);
        BOOST_CHECK(vec[0].get().name == "1");
        BOOST_CHECK(vec[1].get().name == "2");
    }
    {
        dba.remove();
        auto vec = dba.get_range_by<by_name>(::boost::multi_index::unbounded, ::boost::multi_index::unbounded);
        BOOST_CHECK_EQUAL(vec.size(), 1);
        BOOST_CHECK(vec[0].get().name == "1");
    }

    dba.remove(o6);
    BOOST_CHECK(dba.is_empty());
}

BOOST_AUTO_TEST_SUITE_END()
}
