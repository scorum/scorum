#ifdef IS_TEST_NET
#include <boost/test/unit_test.hpp>

#include <scorum/chain/scorum_objects.hpp>
#include <scorum/chain/dbs_registration_committee.hpp>

#include "database_fixture.hpp"

using namespace scorum;
using namespace scorum::chain;
using namespace scorum::protocol;

namespace {
genesis_state_type create_registration_genesis()
{
    const std::string genesis_str = R"json(
    {
            "accounts": [
            {
                    "name": "alice",
                    "recovery_account": "",
                    "public_key": "SCR1111111111111111111111111111111114T1Anm",
                    "scr_amount": 0,
                    "sp_amount": 0
            },
            {
                    "name": "bob",
                    "recovery_account": "",
                    "public_key": "SCR1111111111111111111111111111111114T1Anm",
                    "scr_amount": 0,
                    "sp_amount": 0
            }],
            "registration_committee": ["alice", "bob"]
    })json";

    return fc::json::from_string(genesis_str).as<genesis_state_type>();
}
}

class registration_committee_service_check_fixture : public timed_blocks_database_fixture
{
public:
    registration_committee_service_check_fixture()
        : timed_blocks_database_fixture(create_registration_genesis())
        , registration_committee_service(db.obtain_service<dbs_registration_committee>())
    {
    }

    void insure_committee_exists()
    {
        if (registration_committee_service.get_committee().empty())
        {
            // if object has not created in basic fixture
            registration_committee_service.create_committee(genesis_state);
        }
    }

    dbs_registration_committee& registration_committee_service;
};

BOOST_FIXTURE_TEST_SUITE(registration_committee_service_check, registration_committee_service_check_fixture)

SCORUM_TEST_CASE(create_invalid_genesis_state_check)
{
    genesis_state_type invalid_genesis_state = genesis_state;
    invalid_genesis_state.registration_committee.clear();
    ;

    BOOST_CHECK_THROW(registration_committee_service.create_committee(invalid_genesis_state), fc::assert_exception);
}

SCORUM_TEST_CASE(create_check)
{
    insure_committee_exists();

    using committee_members = dbs_registration_committee::registration_committee_member_refs_type;
    const committee_members& members = registration_committee_service.get_committee();

    BOOST_REQUIRE_EQUAL(members.size(), 2);

    const char* input[] = { "alice", "bob" };
    std::size_t ci = 0;
    for (const registration_committee_member_object& member : members)
    {
        BOOST_CHECK_EQUAL(member.account, input[ci]);

        ++ci;
    }
}

SCORUM_TEST_CASE(create_double_check)
{
    insure_committee_exists();

    BOOST_REQUIRE_THROW(registration_committee_service.create_committee(genesis_state), fc::assert_exception);
}

BOOST_AUTO_TEST_SUITE_END()

#endif
