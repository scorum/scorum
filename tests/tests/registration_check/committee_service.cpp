#ifdef IS_TEST_NET
#include <boost/test/unit_test.hpp>

#include <scorum/chain/scorum_objects.hpp>
#include <scorum/chain/dbs_registration_committee.hpp>

#include "database_fixture.hpp"

using namespace scorum;
using namespace scorum::chain;
using namespace scorum::protocol;

class registration_committee_service_check_fixture : public timed_blocks_database_fixture
{
public:
    registration_committee_service_check_fixture():
          timed_blocks_database_fixture(test::create_registration_genesis()),
          registration_committee_service(db.obtain_service<dbs_registration_committee>())
    {
    }

    dbs_registration_committee& registration_committee_service;
};

BOOST_FIXTURE_TEST_SUITE(registration_committee_service_check, registration_committee_service_check_fixture)

SCORUM_TEST_CASE(create_invalid_genesis_state_check)
{
    genesis_state_type invalid_genesis_state = genesis_state;
    invalid_genesis_state.registration_committee.clear();;

    BOOST_CHECK_THROW(registration_committee_service.create_committee(invalid_genesis_state), fc::assert_exception);
}

BOOST_AUTO_TEST_SUITE_END()

#endif
