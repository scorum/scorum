#ifdef IS_TEST_NET
#include <boost/test/unit_test.hpp>

#include <scorum/chain/scorum_objects.hpp>
#include <scorum/chain/dbs_registration_committee.hpp>

#include "database_fixture.hpp"

using namespace scorum;
using namespace scorum::chain;
using namespace scorum::protocol;

BOOST_FIXTURE_TEST_SUITE(registration_committee_service_check, clean_database_fixture)

SCORUM_TEST_CASE(any_check)
{
    //TODO
}

BOOST_AUTO_TEST_SUITE_END()

#endif
