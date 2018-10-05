#include <boost/test/unit_test.hpp>

#include "budget_check_common.hpp"

namespace budget_security_tests {

using namespace budget_check_common;

struct budget_security_check_fixture : public budget_check_fixture
{
    budget_security_check_fixture()
        : alice("alice")
        , bob("bob")
        , eva("eva")
    {
        actor(initdelegate).create_account(alice);
        actor(initdelegate).give_scr(alice, BUDGET_BALANCE_DEFAULT * 100);
        actor(initdelegate).create_account(bob);
        actor(initdelegate).give_scr(bob, BUDGET_BALANCE_DEFAULT * 200);
        actor(initdelegate).create_account(eva);
        actor(initdelegate).give_scr(eva, 5);

        skip_flags() &= ~database::skip_authority_check;
    }

    template <typename Object, typename Service> void test_close_alice_budget(Service& service, const budget_type type)
    {
        create_budget(alice, type);

        const Object& alice_budget = service.get(0);

        BOOST_REQUIRE_EQUAL(alice_budget.owner, alice.name);

        BOOST_TEST_MESSAGE("Eva try to close alien budget with valid authority");

        close_budget_operation op;
        op.owner = eva.name;
        op.type = type;
        op.budget_id = alice_budget.id._id;

        BOOST_CHECK_NO_THROW(op.validate());

        // Eva doesn't has any budget
        SCORUM_REQUIRE_THROW(push_operation_only(op, eva.private_key), fc::exception);

        op.owner = alice.name;

        // but Alice has
        BOOST_REQUIRE_NO_THROW(push_operation_only(op, alice.private_key));
    }

    template <typename Object, typename Service> void test_update_alice_budget(Service& service, const budget_type type)
    {
        create_budget(alice, type);

        const Object& alice_budget = service.get(0);

        BOOST_REQUIRE_EQUAL(alice_budget.owner, alice.name);

        BOOST_TEST_MESSAGE("Eva try to update alien budget with valid authority");

        update_budget_operation op;
        op.owner = eva.name;
        op.type = type;
        op.budget_id = alice_budget.id._id;
        op.json_metadata = R"j({"valid": false})j";

        BOOST_CHECK_NO_THROW(op.validate());

        // Eva doesn't has any budget
        SCORUM_REQUIRE_THROW(push_operation_only(op, eva.private_key), fc::exception);

        op.owner = alice.name;
        op.json_metadata = R"j({"valid": true})j";

        // but Alice has
        BOOST_REQUIRE_NO_THROW(push_operation_only(op, alice.private_key));
    }

    Actor alice;
    Actor bob;
    Actor eva;
};

BOOST_FIXTURE_TEST_SUITE(budget_security_check, budget_security_check_fixture)

SCORUM_TEST_CASE(invalid_key_check)
{
    create_budget(alice, budget_type::post);

    const post_budget_object& alice_budget = post_budget_service.get(0);

    {
        BOOST_TEST_MESSAGE("Eva try to reset json metada in budget with invalid authority");

        update_budget_operation op;
        op.owner = alice.name;
        op.type = budget_type::post;
        op.budget_id = alice_budget.id._id;
        op.json_metadata = "{}";

        BOOST_CHECK_NO_THROW(op.validate());

        SCORUM_REQUIRE_THROW(push_operation_only(op, eva.private_key), fc::exception);
    }

    {
        BOOST_TEST_MESSAGE("Eva try to close budget with invalid authority");

        close_budget_operation op;
        op.owner = alice.name;
        op.type = budget_type::post;
        op.budget_id = alice_budget.id._id;

        BOOST_CHECK_NO_THROW(op.validate());

        SCORUM_REQUIRE_THROW(push_operation_only(op, eva.private_key), fc::exception);
    }
}

SCORUM_TEST_CASE(miss_close_alien_budget_check)
{
    create_budget(alice, budget_type::post);
    create_budget(bob, budget_type::banner);

    const post_budget_object& alice_budget = post_budget_service.get(0);
    const banner_budget_object& bob_budget = banner_budget_service.get(0);

    {
        BOOST_TEST_MESSAGE("Alice use invalid budget type with valid authority");

        close_budget_operation op;
        op.owner = alice.name;
        op.type = budget_type::banner;
        op.budget_id = alice_budget.id._id;

        BOOST_CHECK_NO_THROW(op.validate());

        SCORUM_REQUIRE_THROW(push_operation_only(op, alice.private_key), fc::exception);

        op.type = budget_type::post;

        BOOST_REQUIRE_NO_THROW(push_operation_only(op, alice.private_key));
    }

    {
        BOOST_TEST_MESSAGE("Bob use invalid budget type with valid authority");

        close_budget_operation op;
        op.owner = bob.name;
        op.type = budget_type::post;
        op.budget_id = bob_budget.id._id;

        BOOST_CHECK_NO_THROW(op.validate());

        SCORUM_REQUIRE_THROW(push_operation_only(op, bob.private_key), fc::exception);

        op.type = budget_type::banner;

        BOOST_REQUIRE_NO_THROW(push_operation_only(op, bob.private_key));
    }
}

SCORUM_TEST_CASE(try_close_alien_budget_check)
{
    test_close_alice_budget<post_budget_object>(post_budget_service, budget_type::post);
    test_close_alice_budget<banner_budget_object>(banner_budget_service, budget_type::banner);
}

SCORUM_TEST_CASE(try_update_alien_budget_check)
{
    test_update_alice_budget<post_budget_object>(post_budget_service, budget_type::post);
    test_update_alice_budget<banner_budget_object>(banner_budget_service, budget_type::banner);
}

BOOST_AUTO_TEST_SUITE_END()
}
