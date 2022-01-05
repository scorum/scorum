#include <boost/test/unit_test.hpp>

#include <scorum/chain/schema/account_objects.hpp>
#include <scorum/chain/schema/dynamic_global_property_object.hpp>
#include <scorum/chain/schema/nft_object.hpp>

#include <scorum/chain/dba/db_accessor.hpp>
#include <scorum/chain/services/dynamic_global_property.hpp>
#include <scorum/chain/services/account.hpp>
#include <scorum/chain/services/hardfork_property.hpp>

#include <scorum/chain/evaluators/create_game_round_evaluator.hpp>
#include <scorum/chain/evaluators/game_round_result_evaluator.hpp>

#include <db_mock.hpp>
#include <hippomocks.h>
#include <defines.hpp>
#include "detail.hpp"

namespace {

using namespace scorum;
using namespace scorum::chain;
using namespace scorum::protocol;

struct game_round_evaluator_fixture
{
    db_mock db;
    MockRepository mocks;

    data_service_factory_i* services = mocks.Mock<data_service_factory_i>();
    hardfork_property_service_i* hardfork_svc = mocks.Mock<hardfork_property_service_i>();

    dba::db_accessor<game_round_object> game_round_dba;
    dba::db_accessor<account_object> account_dba;


    game_round_evaluator_fixture()
        : game_round_dba(db)
        , account_dba(db)
    {
        db.add_index<account_index>();
        db.add_index<game_round_index>();
        db.add_index<hardfork_property_index>();

        mocks.OnCall(services, data_service_factory_i::hardfork_property_service).ReturnByRef(*hardfork_svc);
        mocks.OnCall(hardfork_svc, hardfork_property_service_i::has_hardfork).Return(true);
    }
};

BOOST_FIXTURE_TEST_SUITE(game_round_evaluator_tests, game_round_evaluator_fixture)

SCORUM_TEST_CASE(create_game_round_fail_when_account_does_not_exist)
{
    account_dba.create([&](auto& obj) {
        obj.name = "operator";
    });

    create_game_round_evaluator ev(*services, account_dba, game_round_dba);

    create_game_round_operation op;

    SCORUM_CHECK_EXCEPTION(ev.do_apply(op), fc::assert_exception, R"(Account "" must exist.)")
}

SCORUM_TEST_CASE(create_game_round_fail_when_round_already_exist)
{
    account_dba.create([&](auto& obj) {
        obj.name = "operator";
    });

    game_round_dba.create([&](auto& obj){
       obj.uuid = gen_uuid("first");
    });

    create_game_round_evaluator ev(*services, account_dba, game_round_dba);

    create_game_round_operation op;
    op.owner = "operator";
    op.uuid = gen_uuid("first");

    SCORUM_CHECK_EXCEPTION(ev.do_apply(op), fc::assert_exception, R"(Round with uuid "aa3b2bdc-176e-5e8a-9b48-7dba3aa10044" already exist.)")
}

SCORUM_TEST_CASE(create_game_round)
{
    account_dba.create([&](auto& obj) {
        obj.name = "operator";
    });

    create_game_round_evaluator ev(*services, account_dba, game_round_dba);

    create_game_round_operation op;
    op.owner = "operator";
    op.uuid = gen_uuid("first");
    op.seed = "052d8bec6d55f8c489e837c19fa372e00e3433ebfe0068af658e36b0bd1eb722";
    op.verification_key = "038b2fbf4e4f066f309991b9c30cb8f887853e54c76dc705f5ece736ead6c856";

    BOOST_REQUIRE_NO_THROW(ev.do_apply(op));

    auto& round = game_round_dba.get_by<by_uuid>(op.uuid);
    BOOST_REQUIRE_EQUAL(true, op.uuid == round.uuid);
    BOOST_REQUIRE_EQUAL(op.owner, round.owner);
    BOOST_REQUIRE_EQUAL("052d8bec6d55f8c489e837c19fa372e00e3433ebfe0068af658e36b0bd1eb722", round.seed);
    BOOST_REQUIRE_EQUAL("038b2fbf4e4f066f309991b9c30cb8f887853e54c76dc705f5ece736ead6c856", round.verification_key);
}

SCORUM_TEST_CASE(game_round_result_fail_when_round_does_not_exist)
{
    game_round_result_evaluator ev(*services, account_dba, game_round_dba);
    game_round_result_operation op;

    SCORUM_CHECK_EXCEPTION(ev.do_apply(op), fc::assert_exception, R"(Round "00000000-0000-0000-0000-000000000000" must exist.)")

    op.uuid = gen_uuid("first");

    SCORUM_CHECK_EXCEPTION(ev.do_apply(op), fc::assert_exception, R"(Round "aa3b2bdc-176e-5e8a-9b48-7dba3aa10044" must exist.)")
}

SCORUM_TEST_CASE(game_round_result_fail_when_owner_does_not_exist)
{
    game_round_dba.create([&](auto& obj){
        obj.uuid = gen_uuid("first");
    });

    game_round_result_evaluator ev(*services, account_dba, game_round_dba);
    game_round_result_operation op;
    op.uuid = gen_uuid("first");

    SCORUM_CHECK_EXCEPTION(ev.do_apply(op), fc::assert_exception, R"(Account "" must exist.)")

    op.owner = "operator";

    SCORUM_CHECK_EXCEPTION(ev.do_apply(op), fc::assert_exception, R"(Account "operator" must exist.)")
}

BOOST_AUTO_TEST_SUITE_END()

} // namespace