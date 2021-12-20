#include <boost/test/unit_test.hpp>

#include <scorum/chain/schema/account_objects.hpp>
#include <scorum/chain/schema/dynamic_global_property_object.hpp>
#include <scorum/chain/schema/nft_object.hpp>

#include <scorum/chain/dba/db_accessor.hpp>
#include <scorum/chain/services/dynamic_global_property.hpp>
#include <scorum/chain/services/account.hpp>
#include <scorum/chain/services/hardfork_property.hpp>


#include <scorum/chain/evaluators/create_nft_evaluator.hpp>
#include <scorum/chain/evaluators/update_nft_meta_evaluator.hpp>
#include <scorum/chain/evaluators/increase_nft_power_evaluator.hpp>

#include <db_mock.hpp>
#include <hippomocks.h>
#include <defines.hpp>
#include "detail.hpp"

namespace {

using namespace scorum;
using namespace scorum::chain;
using namespace scorum::protocol;

struct nft_evaluator_fixture
{
    db_mock db;
    MockRepository mocks;

    data_service_factory_i* services = mocks.Mock<data_service_factory_i>();
    hardfork_property_service_i* hardfork_svc = mocks.Mock<hardfork_property_service_i>();

    dba::db_accessor<nft_object> nft_dba;
    dba::db_accessor<account_object> account_dba;

    dbs_dynamic_global_property dprop_service;

    nft_evaluator_fixture()
        : nft_dba(db)
        , account_dba(db)
        , dprop_service(db)
    {
        db.add_index<account_index>();
        db.add_index<nft_index>();
        db.add_index<dynamic_global_property_index>();
        db.add_index<hardfork_property_index>();

        mocks.OnCall(services, data_service_factory_i::dynamic_global_property_service).ReturnByRef(dprop_service);
        mocks.OnCall(services, data_service_factory_i::hardfork_property_service).ReturnByRef(*hardfork_svc);
        mocks.OnCall(hardfork_svc, hardfork_property_service_i::has_hardfork).Return(true);
    }
};

BOOST_FIXTURE_TEST_SUITE(create_nft_evaluator_tests, nft_evaluator_fixture)

SCORUM_TEST_CASE(create_nft)
{
    dprop_service.create([&](auto& dprop) { dprop.time = fc::time_point_sec::from_iso_string("2018-01-01T00:00:00"); });
    account_dba.create([&](auto& obj) {
        obj.name = "user";
        obj.scorumpower = asset(10, SP_SYMBOL);
    });

    create_nft_evaluator ev(*services, account_dba, nft_dba);

    create_nft_operation op;
    op.name = "plane";
    op.owner = "user";
    op.power = 9;
    op.json_metadata = "{}";

    BOOST_REQUIRE_NO_THROW(ev.do_apply(op));

    auto& nft = nft_dba.get_by<by_name>(op.name);
    BOOST_REQUIRE_EQUAL(op.name, nft.name);
    BOOST_REQUIRE_EQUAL(op.owner, nft.owner);
    BOOST_REQUIRE_EQUAL(op.power, nft.power);
    BOOST_REQUIRE_EQUAL("2018-01-01T00:00:00", nft.created.to_iso_string());

#ifndef IS_LOW_MEM
    BOOST_REQUIRE_EQUAL("{}", nft.json_metadata);
#endif

    auto& user = account_dba.get_by<by_name>("user");
    BOOST_REQUIRE_EQUAL(9, user.nft_spend_scorumpower.amount);
    BOOST_REQUIRE_EQUAL(10, user.scorumpower.amount);
}

SCORUM_TEST_CASE(create_nft_fail_when_nft_exists)
{
    nft_dba.create([&](auto& nft) { nft.name = "plane"; });

    create_nft_evaluator ev(*services, account_dba, nft_dba);

    create_nft_operation op;
    op.name = "plane";

    SCORUM_CHECK_EXCEPTION(ev.do_apply(op), fc::assert_exception, R"(NFT with name "plane" already exists.)")
}

SCORUM_TEST_CASE(create_nft_fail_when_account_does_not_exists)
{
    create_nft_evaluator ev(*services, account_dba, nft_dba);

    create_nft_operation op;
    op.name = "plane";
    op.owner = "user";

    SCORUM_CHECK_EXCEPTION(ev.do_apply(op), fc::assert_exception, R"(Account "user" must exist.)")
}

SCORUM_TEST_CASE(create_nft_fail_when_not_enough_scorumpower)
{
    account_dba.create([&](auto& account) {
        account.name = "user";
        account.scorumpower = asset(10, SP_SYMBOL);
    });

    create_nft_evaluator ev(*services, account_dba, nft_dba);

    create_nft_operation op;
    op.name = "plane";
    op.owner = "user";
    op.power = 11;

    SCORUM_CHECK_EXCEPTION(ev.do_apply(op), fc::assert_exception,
                           R"(Account available power "10" is less than requested "11".)")
}

SCORUM_TEST_CASE(create_second_nft_fail_when_not_enough_scorumpower)
{
    dprop_service.create([&](auto& dprop) { dprop.time = fc::time_point_sec::from_iso_string("2018-01-01T00:00:00"); });
    account_dba.create([&](auto& obj) {
        obj.name = "user";
        obj.scorumpower = asset(10, SP_SYMBOL);
    });

    create_nft_evaluator ev(*services, account_dba, nft_dba);

    create_nft_operation op;
    op.uuid = gen_uuid("plane");
    op.name = "plane";
    op.owner = "user";
    op.power = 9;
    op.json_metadata = "{}";

    BOOST_REQUIRE_NO_THROW(ev.do_apply(op));

    create_nft_operation op2;
    op2.uuid = gen_uuid("plane2");
    op2.name = "plane2";
    op2.owner = "user";
    op2.power = 2;
    op2.json_metadata = "{}";

    SCORUM_CHECK_EXCEPTION(ev.do_apply(op2), fc::assert_exception, R"(Account available power "1" is less than requested "2".)")

    auto& user = account_dba.get_by<by_name>("user");
    BOOST_REQUIRE_EQUAL(9, user.nft_spend_scorumpower.amount);
    BOOST_REQUIRE_EQUAL(10, user.scorumpower.amount);
}

SCORUM_TEST_CASE(create_second_nft)
{
    dprop_service.create([&](auto& dprop) { dprop.time = fc::time_point_sec::from_iso_string("2018-01-01T00:00:00"); });
    account_dba.create([&](auto& obj) {
        obj.name = "user";
        obj.scorumpower = asset(10, SP_SYMBOL);
    });

    create_nft_evaluator ev(*services, account_dba, nft_dba);

    create_nft_operation op;
    op.uuid = gen_uuid("plane");
    op.name = "plane";
    op.owner = "user";
    op.power = 9;
    op.json_metadata = "{}";

    BOOST_REQUIRE_NO_THROW(ev.do_apply(op));

    create_nft_operation op2;
    op2.uuid = gen_uuid("plane2");
    op2.name = "plane2";
    op2.owner = "user";
    op2.power = 1;
    op2.json_metadata = "{}";

    BOOST_REQUIRE_NO_THROW(ev.do_apply(op2));

    auto& user = account_dba.get_by<by_name>("user");
    BOOST_REQUIRE_EQUAL(10, user.nft_spend_scorumpower.amount);
    BOOST_REQUIRE_EQUAL(10, user.scorumpower.amount);
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_FIXTURE_TEST_SUITE(update_nft_meta_evaluator_test, nft_evaluator_fixture)

SCORUM_TEST_CASE(update_nft_meta)
{
    auto expected_nft = nft_dba.create([&](auto& nft) {
        nft.uuid = gen_uuid("nft");
        nft.name = "plane";
        nft.owner = "user";
    });

    account_dba.create([&](auto& obj) {
        obj.name = "moderator";
        obj.scorumpower = asset(10, SP_SYMBOL);
    });

    update_nft_meta_evaluator ev(*services, account_dba, nft_dba);

    update_nft_meta_operation op;
    op.uuid = expected_nft.uuid;
    op.moderator = "moderator";
    op.json_metadata = "metadata";

    BOOST_REQUIRE_NO_THROW(ev.do_apply(op));

    auto& nft = nft_dba.get_by<by_uuid>(op.uuid);
    BOOST_REQUIRE_EQUAL("plane", nft.name);
    BOOST_REQUIRE_EQUAL("user", nft.owner);

#ifndef IS_LOW_MEM
    BOOST_REQUIRE_EQUAL("metadata", nft.json_metadata);
#endif
}

SCORUM_TEST_CASE(update_nft_meta_fail_when_nft_does_not_exists)
{
    update_nft_meta_evaluator ev(*services, account_dba, nft_dba);

    update_nft_meta_operation op;
    op.uuid = gen_uuid("nft");

    SCORUM_CHECK_EXCEPTION(ev.do_apply(op), fc::assert_exception,
                           R"(NFT with uuid "de53e30d-d502-5cc0-b25f-a6f0579f271c" must exist.)")
}

SCORUM_TEST_CASE(update_nft_meta_fail_when_account_does_not_exists)
{
    auto expected_nft = nft_dba.create([&](auto& nft) {
        nft.uuid = gen_uuid("nft");
        nft.name = "plane";
        nft.owner = "user";
    });
    update_nft_meta_evaluator ev(*services, account_dba, nft_dba);

    update_nft_meta_operation op;
    op.uuid = expected_nft.uuid;
    op.moderator = "moderator";
#ifndef IS_LOW_MEM
    op.json_metadata = "metadata";
#endif

    SCORUM_CHECK_EXCEPTION(ev.do_apply(op), fc::assert_exception, R"(Account "moderator" must exist.)")
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_FIXTURE_TEST_SUITE(increase_nft_power_evaluator_test, nft_evaluator_fixture)

SCORUM_TEST_CASE(increase_nft_power)
{
    auto expected_nft = nft_dba.create([&](auto& nft) {
        nft.uuid = gen_uuid("nft");
        nft.name = "plane";
        nft.owner = "user";
    });

    account_dba.create([&](auto& obj) {
        obj.name = "moderator";
        obj.scorumpower = asset(10, SP_SYMBOL);
    });

    increase_nft_power_evaluator ev(*services, account_dba, nft_dba);

    increase_nft_power_operation op;
    op.uuid = expected_nft.uuid;
    op.moderator = "moderator";
    op.power = 100;

    BOOST_REQUIRE_NO_THROW(ev.do_apply(op));

    auto& nft = nft_dba.get_by<by_uuid>(op.uuid);
    BOOST_REQUIRE_EQUAL("plane", nft.name);
    BOOST_REQUIRE_EQUAL("user", nft.owner);
    BOOST_REQUIRE_EQUAL(100, nft.power);
}

SCORUM_TEST_CASE(increase_nft_power_fail_when_nft_does_not_exists)
{
    increase_nft_power_evaluator ev(*services, account_dba, nft_dba);
    increase_nft_power_operation op;

    SCORUM_CHECK_EXCEPTION(ev.do_apply(op), fc::assert_exception,
                           R"(NFT with uuid "00000000-0000-0000-0000-000000000000" must exist.)")

    op.uuid = gen_uuid("nft");

    SCORUM_CHECK_EXCEPTION(ev.do_apply(op), fc::assert_exception,
                           R"(NFT with uuid "de53e30d-d502-5cc0-b25f-a6f0579f271c" must exist.)")
}

SCORUM_TEST_CASE(increase_nft_power_fail_when_account_does_not_exists)
{
    auto& expected_nft = nft_dba.create([&](auto& nft) {
        nft.uuid = gen_uuid("nft");
        nft.name = "plane";
        nft.owner = "user";
    });
    increase_nft_power_evaluator ev(*services, account_dba, nft_dba);

    increase_nft_power_operation op;
    op.uuid = expected_nft.uuid;
    op.moderator = "moderator";

    SCORUM_CHECK_EXCEPTION(ev.do_apply(op), fc::assert_exception, R"(Account "moderator" must exist.)")
}

BOOST_AUTO_TEST_SUITE_END()

} // namespace