#include <boost/test/unit_test.hpp>

#include <scorum/chain/services/betting_property.hpp>
#include <scorum/chain/betting/betting_service.hpp>
#include <scorum/chain/dba/db_accessor.hpp>
#include <scorum/chain/dba/db_accessor_factory.hpp>
#include "betting_common.hpp"

namespace {

using namespace scorum::chain;
using namespace scorum::protocol;

struct betting_service_fixture : public betting_common::betting_service_fixture_impl
{
    MockRepository mocks;

    dba::db_index* db = mocks.Mock<dba::db_index>();
    chainbase::database_index<chainbase::segment_manager>* db_index
        = mocks.Mock<chainbase::database_index<chainbase::segment_manager>>();

    betting_service_fixture()
        : dba_factory(*db)
        , betting_prop_dba(*db_index)
        , betting_prop(
              create_object<betting_property_object>(shm, [&](betting_property_object& o) { o.moderator = moderator; }))
    {
        mocks.OnCallFunc(dba::detail::get_single<betting_property_object>).ReturnByRef(betting_prop);
    }

protected:
    dba::db_accessor_factory dba_factory;
    dba::db_accessor<betting_property_object> betting_prop_dba;
    betting_property_object betting_prop;
};

BOOST_FIXTURE_TEST_SUITE(betting_service_tests, betting_service_fixture)

SCORUM_TEST_CASE(budget_service_is_betting_moderator_check)
{
    betting_service service(*dbs_services, dba_factory);

    BOOST_CHECK(!service.is_betting_moderator("jack"));
    BOOST_CHECK(service.is_betting_moderator(moderator));
}

BOOST_AUTO_TEST_SUITE_END()
}
