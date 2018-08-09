#include <boost/test/unit_test.hpp>

#include "common_fixtures.hpp"

#include <scorum/chain/evaluators/create_budget_evaluator.hpp>
#include <scorum/chain/evaluators/close_budget_evaluator.hpp>
#include <scorum/chain/evaluators/update_budget_evaluator.hpp>

namespace evaluators_tests {

using namespace common_fixtures;

struct services_for_budget_fixture : public account_budget_fixture
{
    data_service_factory_i* services = mocks.Mock<data_service_factory_i>();

    services_for_budget_fixture()
    {
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
        alice_close_budget_operation.budget_id = 1;

        alice_update_budget_operation.owner = alice.name;
        alice_update_budget_operation.budget_id = 1;
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
    SCORUM_REQUIRE_THROW(create_evaluator.do_apply(op), fc::assert_exception);
    SCORUM_CHECK_THROW(op.validate(), fc::assert_exception);
    op.start = alice_create_budget_operation.start;
    op.deadline = alice_create_budget_operation.deadline;

    op.owner = bob.name;
    SCORUM_REQUIRE_THROW(create_evaluator.do_apply(op), fc::assert_exception);
    op.owner = alice_create_budget_operation.owner;

    BOOST_REQUIRE_NO_THROW(create_evaluator.do_apply(op));

    BOOST_REQUIRE_EQUAL(post_budget_service_fixture.get().start.sec_since_epoch(),
                        alice_create_budget_operation.start.sec_since_epoch());
    BOOST_REQUIRE_EQUAL(post_budget_service_fixture.get().deadline.sec_since_epoch(),
                        alice_create_budget_operation.deadline.sec_since_epoch());
}

BOOST_FIXTURE_TEST_CASE(autoreset_start_to_headblock_time_wile_creation, evaluators_for_budget_fixture)
{
    create_budget_evaluator::operation_type op = alice_create_budget_operation;
    op.start = head_block_time - fc::seconds(SCORUM_BLOCK_INTERVAL * 10);

    BOOST_REQUIRE_NO_THROW(create_evaluator.do_apply(op));

    BOOST_REQUIRE_EQUAL(post_budget_service_fixture.get().start.sec_since_epoch(), head_block_time.sec_since_epoch());
    BOOST_REQUIRE_EQUAL(post_budget_service_fixture.get().deadline.sec_since_epoch(),
                        alice_create_budget_operation.deadline.sec_since_epoch());
}

BOOST_FIXTURE_TEST_CASE(budgets_limit_per_owner, evaluators_for_budget_fixture)
{
    create_budget_evaluator::operation_type op = alice_create_budget_operation;
    op.type = budget_type::post;

    auto half = SCORUM_BUDGETS_LIMIT_PER_OWNER / 2;
    for (size_t ci = 0; ci < half; ++ci)
    {
        BOOST_REQUIRE_NO_THROW(create_evaluator.do_apply(op));
    }

    op.type = budget_type::banner;

    half = SCORUM_BUDGETS_LIMIT_PER_OWNER - half;
    for (size_t ci = 0; ci < half; ++ci)
    {
        BOOST_REQUIRE_NO_THROW(create_evaluator.do_apply(op));
    }

    SCORUM_REQUIRE_THROW(create_evaluator.do_apply(op), fc::assert_exception);
}

struct update_evaluators_for_budget_fixture : public evaluators_for_budget_fixture
{
    update_evaluators_for_budget_fixture()
        : evaluators_for_budget_fixture()
    {
        create_budget_evaluator::operation_type op = alice_create_budget_operation;
        op.type = budget_type::post;
        BOOST_REQUIRE_NO_THROW(create_evaluator.do_apply(op));
        op.type = budget_type::banner;
        BOOST_REQUIRE_NO_THROW(create_evaluator.do_apply(op));
    }
};

BOOST_FIXTURE_TEST_CASE(budget_close, update_evaluators_for_budget_fixture)
{
    close_budget_evaluator::operation_type op = alice_close_budget_operation;
    op.type = budget_type::post;

    op.owner = bob.name;
    SCORUM_REQUIRE_THROW(close_evaluator.do_apply(op), fc::assert_exception);
    op.owner = alice_create_budget_operation.owner;

    BOOST_REQUIRE(!post_budget_service_fixture.empty());
    BOOST_REQUIRE(!banner_budget_service_fixture.empty());

    BOOST_REQUIRE_NO_THROW(close_evaluator.do_apply(op));

    BOOST_CHECK(post_budget_service_fixture.empty());

    op.type = budget_type::banner;

    BOOST_REQUIRE_NO_THROW(close_evaluator.do_apply(op));

    BOOST_CHECK(banner_budget_service_fixture.empty());
}

BOOST_FIXTURE_TEST_CASE(budget_upate, update_evaluators_for_budget_fixture)
{
    update_budget_evaluator::operation_type op = alice_update_budget_operation;
    op.type = budget_type::post;

    op.owner = bob.name;
    SCORUM_REQUIRE_THROW(update_evaluator.do_apply(op), fc::assert_exception);
    op.owner = alice_update_budget_operation.owner;

    op.json_metadata = R"j({{"wrong)j";
    SCORUM_REQUIRE_THROW(op.validate(), fc::exception);
    op.json_metadata = alice_update_budget_operation.json_metadata;

#ifndef IS_LOW_MEM
    BOOST_REQUIRE_EQUAL(fc::to_string(post_budget_service_fixture.get().json_metadata),
                        alice_create_budget_operation.json_metadata);
    BOOST_REQUIRE_EQUAL(fc::to_string(banner_budget_service_fixture.get().json_metadata),
                        alice_create_budget_operation.json_metadata);
#endif //! IS_LOW_MEM

    BOOST_REQUIRE_NO_THROW(update_evaluator.do_apply(op));

#ifndef IS_LOW_MEM
    BOOST_CHECK_EQUAL(fc::to_string(post_budget_service_fixture.get().json_metadata),
                      alice_update_budget_operation.json_metadata);
#endif //! IS_LOW_MEM

    op.type = budget_type::banner;

    BOOST_REQUIRE_NO_THROW(update_evaluator.do_apply(op));

#ifndef IS_LOW_MEM
    BOOST_CHECK_EQUAL(fc::to_string(banner_budget_service_fixture.get().json_metadata),
                      alice_update_budget_operation.json_metadata);
#endif //! IS_LOW_MEM
}
}
