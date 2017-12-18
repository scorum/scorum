#ifdef IS_TEST_NET
#include <boost/test/unit_test.hpp>

#include <scorum/chain/scorum_objects.hpp>

#include "database_fixture.hpp"

using namespace scorum;
using namespace scorum::chain;
using namespace scorum::protocol;

//
// usage for all budget tests 'chain_test  -t registration_*'
//

class registration_committee_operation_check_fixture : public timed_blocks_database_fixture
{
public:
    registration_committee_operation_check_fixture()
    {
        account_committee_op.creator = "alice";
        account_committee_op.new_account_name = "andrew";
        account_committee_op.owner = authority(1, init_account_pub_key, 1);
        account_committee_op.active = authority(1, init_account_pub_key, 1);
        account_committee_op.posting = authority(1, init_account_pub_key, 1);
        account_committee_op.memo_key = init_account_pub_key;
        account_committee_op.json_metadata = "";
    }

    account_create_by_committee_operation account_committee_op;
};

BOOST_FIXTURE_TEST_SUITE(registration_committee_operation_check, registration_committee_operation_check_fixture)

SCORUM_TEST_CASE(create_account_by_committee_operation_check)
{
    BOOST_REQUIRE_NO_THROW(account_committee_op.validate());
}

SCORUM_TEST_CASE(any_check)
{
    // TODO
}

BOOST_AUTO_TEST_SUITE_END()

#endif
