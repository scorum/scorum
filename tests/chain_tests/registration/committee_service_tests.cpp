#ifdef IS_TEST_NET
#include <boost/test/unit_test.hpp>

#include "registration_check_common.hpp"

using namespace database_fixture;

class registration_committee_service_check_fixture : public registration_check_fixture
{
public:
    registration_committee_service_check_fixture()
        : registration_committee_service(db.obtain_service<dbs_registration_committee>())
    {
        genesis_state = create_registration_genesis();
        create_registration_objects(genesis_state);
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
    using committee_members = dbs_registration_committee::member_object_cref_type;
    const committee_members& members = registration_committee_service.get_committee();

    BOOST_REQUIRE_EQUAL(members.size(), size_t(2));

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
