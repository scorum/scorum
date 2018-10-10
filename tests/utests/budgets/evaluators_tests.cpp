#include <boost/test/unit_test.hpp>

#include "common_fixtures.hpp"

#include <scorum/chain/evaluators/create_budget_evaluator.hpp>
#include <scorum/chain/evaluators/close_budget_evaluator.hpp>
#include <scorum/chain/evaluators/update_budget_evaluator.hpp>

#include <boost/uuid/uuid_generators.hpp>

namespace evaluators_tests {

using namespace common_fixtures;

struct services_for_budget_fixture : public account_budget_fixture
{
    data_service_factory_i* services = mocks.Mock<data_service_factory_i>();
    database_virtual_operations_emmiter_i* virt_op_emmiter = mocks.Mock<database_virtual_operations_emmiter_i>();

    services_for_budget_fixture()
    {
        mocks.OnCall(virt_op_emmiter, database_virtual_operations_emmiter_i::push_virtual_operation);
        mocks.OnCall(services, data_service_factory_i::account_service).ReturnByRef(account_service_fixture.service());
        mocks.OnCall(services, data_service_factory_i::post_budget_service)
            .ReturnByRef(post_budget_service_fixture.service());
        mocks.OnCall(services, data_service_factory_i::banner_budget_service)
            .ReturnByRef(banner_budget_service_fixture.service());
        mocks.OnCall(services, data_service_factory_i::dynamic_global_property_service)
            .ReturnByRef(dgp_service_fixture.service());
    }
};

struct evaluators_for_budget_fixture : public services_for_budget_fixture
{
    Actor bob;

    create_budget_evaluator create_evaluator;
    close_budget_evaluator close_evaluator;
    update_budget_evaluator update_evaluator;

    create_budget_evaluator::operation_type alice_create_budget_operation;
    close_budget_evaluator::operation_type alice_close_budget_operation;
    update_budget_evaluator::operation_type alice_update_budget_operation;

    scorum::uuid_type alice_uuid = boost::uuids::string_generator()("00000000-0000-0000-0000-000000000001");

    evaluators_for_budget_fixture()
        : services_for_budget_fixture()
        , bob("bob")
        , create_evaluator(*services)
        , close_evaluator(*services)
        , update_evaluator(*services)
    {
        alice_create_budget_operation.owner = alice.name;
        alice_create_budget_operation.balance = asset(balance, SCORUM_SYMBOL);
        alice_create_budget_operation.start = start;
        alice_create_budget_operation.deadline = deadline;
        alice_create_budget_operation.json_metadata = R"j({"company": "adidas"})j";

        alice_close_budget_operation.owner = alice.name;
        alice_close_budget_operation.uuid = alice_uuid;

        alice_update_budget_operation.owner = alice.name;
        alice_update_budget_operation.uuid = alice_uuid;
        alice_update_budget_operation.json_metadata = R"j({"fake_company": "abibas"})j";
    }
};

BOOST_FIXTURE_TEST_CASE(budget_creation, evaluators_for_budget_fixture)
{
    create_budget_evaluator::operation_type op = alice_create_budget_operation;
    op.type = budget_type::post;

    BOOST_REQUIRE_NO_THROW(create_evaluator.do_apply(op));

    op.type = budget_type::banner;

    BOOST_REQUIRE_NO_THROW(create_evaluator.do_apply(op));
}

BOOST_FIXTURE_TEST_CASE(budget_creation_same_uuid_should_throw, evaluators_for_budget_fixture)
{
    create_budget_evaluator::operation_type op = alice_create_budget_operation;
    op.type = budget_type::post;

    BOOST_REQUIRE_NO_THROW(create_evaluator.do_apply(op));
    BOOST_REQUIRE_THROW(create_evaluator.do_apply(op), fc::assert_exception);
}

BOOST_FIXTURE_TEST_CASE(asserts_in_budget_creation, evaluators_for_budget_fixture)
{
    create_budget_evaluator::operation_type op = alice_create_budget_operation;
    op.type = budget_type::post;
    op.owner = SCORUM_ROOT_POST_PARENT_ACCOUNT;
    SCORUM_REQUIRE_THROW(create_evaluator.do_apply(op), fc::assert_exception);
    SCORUM_CHECK_THROW(op.validate(), fc::assert_exception);
    op.owner = alice_create_budget_operation.owner;

    op.json_metadata = R"j({"wrong)j";
    SCORUM_REQUIRE_THROW(op.validate(), fc::exception);
    op.json_metadata = alice_create_budget_operation.json_metadata;

    op.balance = asset(op.balance.amount, SP_SYMBOL);
    SCORUM_REQUIRE_THROW(create_evaluator.do_apply(op), fc::assert_exception);
    SCORUM_CHECK_THROW(op.validate(), fc::assert_exception);
    op.balance = alice_create_budget_operation.balance;

    op.start = deadline;
    op.deadline = start;
    SCORUM_CHECK_THROW(op.validate(), fc::assert_exception);

    op.owner = bob.name;
    SCORUM_REQUIRE_THROW(create_evaluator.do_apply(op), fc::assert_exception);
    op.owner = alice_create_budget_operation.owner;

    op.start = deadline;
    op.deadline = start;
    BOOST_REQUIRE_THROW(create_evaluator.do_apply(op), fc::assert_exception);

    op.start = start;
    op.deadline = deadline;
    BOOST_REQUIRE_NO_THROW(create_evaluator.do_apply(op));
}

BOOST_FIXTURE_TEST_CASE(budgets_limit_per_owner, evaluators_for_budget_fixture)
{
    create_budget_evaluator::operation_type op = alice_create_budget_operation;
    op.type = budget_type::post;

    auto uuid_gen = boost::uuids::random_generator();

    auto half = SCORUM_BUDGETS_LIMIT_PER_OWNER / 2;
    for (size_t ci = 0; ci < half; ++ci)
    {
        op.uuid = uuid_gen();
        BOOST_REQUIRE_NO_THROW(create_evaluator.do_apply(op));
    }

    op.type = budget_type::banner;

    half = SCORUM_BUDGETS_LIMIT_PER_OWNER - half;
    for (size_t ci = 0; ci < half; ++ci)
    {
        op.uuid = uuid_gen();
        BOOST_REQUIRE_NO_THROW(create_evaluator.do_apply(op));
    }

    SCORUM_REQUIRE_THROW(create_evaluator.do_apply(op), fc::assert_exception);
}
}
