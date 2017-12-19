#ifdef IS_TEST_NET
#include <boost/test/unit_test.hpp>

#include <scorum/chain/scorum_objects.hpp>
#include <scorum/chain/dbs_registration_committee.hpp>

#include "database_fixture.hpp"

#include <vector>

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
} // namespace

class registration_committee_service_check_fixture : public timed_blocks_database_fixture
{
public:
    registration_committee_service_check_fixture()
        : timed_blocks_database_fixture(create_registration_genesis())
        , registration_committee_service(db.obtain_service<dbs_registration_committee>())
    {
    }

    using account_names_type = std::vector<account_name_type>;

    account_names_type get_registration_committee()
    {
        account_names_type ret;
        for (const auto& member : genesis_state.registration_committee)
        {
            ret.emplace_back(member);
        }
        return ret;
    }

    dbs_registration_committee& registration_committee_service;
};

BOOST_FIXTURE_TEST_SUITE(registration_committee_service_check, registration_committee_service_check_fixture)

SCORUM_TEST_CASE(create_invalid_genesis_state_check)
{
    account_names_type empty_committee;

    BOOST_CHECK_THROW(registration_committee_service.create_committee(empty_committee), fc::assert_exception);
}

SCORUM_TEST_CASE(create_check)
{
    using committee_members = dbs_registration_committee::registration_committee_member_refs_type;
    const committee_members& members = registration_committee_service.get_committee();

    BOOST_REQUIRE_EQUAL(members.size(), (size_t)2);

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
    BOOST_REQUIRE_THROW(registration_committee_service.create_committee(get_registration_committee()),
                        fc::assert_exception);
}

BOOST_AUTO_TEST_SUITE_END()

#endif
