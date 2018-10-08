#include <boost/test/unit_test.hpp>

#include "budget_check_common.hpp"

namespace {

using namespace budget_check_common;

class budget_owner_close_tests_fixture : public budget_check_fixture
{
public:
    budget_owner_close_tests_fixture()
        : account_service(db.account_service())
        , alice("alice")
    {
        actor(initdelegate).create_account(alice);
        actor(initdelegate).give_scr(alice, BUDGET_BALANCE_DEFAULT * 100);
    }

    account_service_i& account_service;

    Actor alice;
};

BOOST_FIXTURE_TEST_SUITE(budget_autoclose_check, budget_owner_close_tests_fixture)

SCORUM_TEST_CASE(owner_should_close_budget_test)
{
    int balance = 1000;
    int start_offset = 1;
    int deadline_offset = 15;

    create_budget(alice, budget_type::post, balance, start_offset, deadline_offset);
    generate_block();

    BOOST_REQUIRE(!post_budget_service.get_budgets(alice.name).empty());

    close_budget_operation op;
    op.type = budget_type::post;
    op.budget_id = 0;
    op.owner = alice.name;

    push_operation(op);

    BOOST_REQUIRE(post_budget_service.get_budgets(alice.name).empty());
}

SCORUM_TEST_CASE(non_owner_shouldnt_close_budget_test)
{
    int balance = 1000;
    int start_offset = 1;
    int deadline_offset = 15;

    create_budget(alice, budget_type::post, balance, start_offset, deadline_offset);
    generate_block();

    BOOST_REQUIRE(!post_budget_service.get_budgets(alice.name).empty());

    close_budget_operation op;
    op.type = budget_type::post;
    op.budget_id = 0;
    op.owner = initdelegate.name;

    BOOST_CHECK_THROW(push_operation(op), fc::assert_exception);
}

SCORUM_TEST_CASE(should_raise_closing_virt_op_test)
{
    auto was_raised = false;
    db.pre_apply_operation.connect([&](const operation_notification& op_notif) {
        op_notif.op.weak_visit([&](const budget_closing_operation&) { was_raised = true; });
    });

    int balance = 1000;
    int start_offset = 1;
    int deadline_offset = 15;

    create_budget(alice, budget_type::post, balance, start_offset, deadline_offset);
    generate_block();

    close_budget_operation op;
    op.type = budget_type::post;
    op.budget_id = 0;
    op.owner = alice.name;

    push_operation(op);

    BOOST_CHECK(was_raised);
}

BOOST_AUTO_TEST_SUITE_END()
}
