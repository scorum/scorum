#include <boost/test/unit_test.hpp>
#include "budget_check_common.hpp"

namespace advertising_moderator_tests {

using namespace budget_check_common;

class advertising_moderator_fixture : public budget_check_fixture
{
public:
    advertising_moderator_fixture()
        : account_service(db.account_service())
    {
        actor(initdelegate).create_account(alice);
        actor(initdelegate).create_account(bob);
        actor(initdelegate).create_account(moder);
        actor(initdelegate).give_scr(alice, BUDGET_BALANCE_DEFAULT * 100);
    }

    void empower_advertising_moderator(const Actor& moderator_acc)
    {
        development_committee_empower_advertising_moderator_operation empower_moder_op;
        empower_moder_op.account = moderator_acc.name;

        proposal_create_operation prop_create_op;
        prop_create_op.creator = initdelegate.name;
        prop_create_op.operation = empower_moder_op;
        prop_create_op.lifetime_sec = SCORUM_PROPOSAL_LIFETIME_MIN_SECONDS;

        push_operation(prop_create_op, alice.private_key);

        proposal_vote_operation prop_vote_op;
        prop_vote_op.proposal_id = proposal_no++;
        prop_vote_op.voting_account = initdelegate.name;

        push_operation(prop_vote_op, initdelegate.private_key);
    }

    account_service_i& account_service;

    Actor alice = "alice";
    Actor bob = "bob";
    Actor moder = "moder";

    int64_t proposal_no = 0;
};

BOOST_FIXTURE_TEST_SUITE(advertising_moderator_close_budget_tests, advertising_moderator_fixture)

SCORUM_TEST_CASE(should_close_post_budget)
{
    BOOST_REQUIRE_EQUAL(post_budget_service.get_budgets().size(), 0u);

    create_budget(alice, budget_type::post);

    BOOST_REQUIRE_EQUAL(post_budget_service.get_budgets().size(), 1u);

    empower_advertising_moderator(moder);

    close_budget_by_advertising_moderator_operation close_budget_op;
    close_budget_op.moderator = moder.name;
    close_budget_op.budget_id = 0;
    close_budget_op.type = budget_type::post;

    push_operation(close_budget_op, moder.private_key);

    BOOST_REQUIRE_EQUAL(post_budget_service.get_budgets().size(), 0u);
}

SCORUM_TEST_CASE(should_close_banner_budget)
{
    BOOST_REQUIRE_EQUAL(banner_budget_service.get_budgets().size(), 0u);

    create_budget(alice, budget_type::banner);

    BOOST_REQUIRE_EQUAL(banner_budget_service.get_budgets().size(), 1u);

    empower_advertising_moderator(moder);

    close_budget_by_advertising_moderator_operation close_budget_op;
    close_budget_op.moderator = moder.name;
    close_budget_op.budget_id = 0;
    close_budget_op.type = budget_type::banner;

    push_operation(close_budget_op, moder.private_key);

    BOOST_REQUIRE_EQUAL(banner_budget_service.get_budgets().size(), 0u);
}

SCORUM_TEST_CASE(check_moderator_changing)
{
    BOOST_REQUIRE_EQUAL(banner_budget_service.get_budgets().size(), 0u);

    create_budget(alice, budget_type::banner, BUDGET_BALANCE_DEFAULT,
                  100 * BUDGET_DEADLINE_IN_BLOCKS_DEFAULT); // budget 0
    create_budget(alice, budget_type::banner, BUDGET_BALANCE_DEFAULT,
                  100 * BUDGET_DEADLINE_IN_BLOCKS_DEFAULT); // budget 1

    BOOST_REQUIRE_EQUAL(banner_budget_service.get_budgets().size(), 2u);

    empower_advertising_moderator(moder);

    close_budget_by_advertising_moderator_operation close_budget_op;
    close_budget_op.moderator = bob.name;
    close_budget_op.budget_id = 0;
    close_budget_op.type = budget_type::banner;

    BOOST_REQUIRE_THROW(push_operation(close_budget_op, bob.private_key),
                        fc::assert_exception); // 'bob' is not a moderator

    close_budget_op.moderator = moder.name;
    push_operation(close_budget_op, moder.private_key); // 'moder' is moderator

    BOOST_REQUIRE_EQUAL(banner_budget_service.get_budgets().size(), 1u);

    empower_advertising_moderator(bob); // now 'bob' became moderator ('moder' is not)

    close_budget_op.budget_id = 1;
    close_budget_op.moderator = moder.name;
    BOOST_REQUIRE_THROW(push_operation(close_budget_op, moder.private_key), fc::assert_exception);

    close_budget_op.moderator = bob.name;
    push_operation(close_budget_op, bob.private_key);

    BOOST_REQUIRE_EQUAL(banner_budget_service.get_budgets().size(), 0u);
}

SCORUM_TEST_CASE(should_throw_moderator_not_exists)
{
    create_budget(alice, budget_type::post);

    close_budget_by_advertising_moderator_operation op;
    op.moderator = moder.name;
    op.budget_id = 0;
    op.type = budget_type::post;

    BOOST_REQUIRE_THROW(push_operation(op, moder.private_key), fc::assert_exception);
}

SCORUM_TEST_CASE(should_throw_moder_is_not_a_moderator)
{
    create_budget(alice, budget_type::post);

    empower_advertising_moderator(alice);

    close_budget_by_advertising_moderator_operation op;
    op.moderator = moder.name;
    op.budget_id = 0;
    op.type = budget_type::post;

    BOOST_REQUIRE_THROW(push_operation(op, moder.private_key), fc::assert_exception);
}

SCORUM_TEST_CASE(should_throw_usual_user_trying_close_budget)
{
    create_budget(alice, budget_type::post);

    empower_advertising_moderator(moder);

    close_budget_by_advertising_moderator_operation op;
    op.moderator = initdelegate.name;
    op.budget_id = 0;
    op.type = budget_type::post;

    BOOST_REQUIRE_THROW(push_operation(op, initdelegate.private_key), fc::assert_exception);
}

SCORUM_TEST_CASE(should_throw_active_key_required)
{
    create_budget(alice, budget_type::post);

    empower_advertising_moderator(moder);

    close_budget_by_advertising_moderator_operation op;
    op.moderator = moder.name;
    op.budget_id = 0;
    op.type = budget_type::post;

    signed_transaction tx;
    tx.operations.push_back(op);
    tx.set_expiration(db.head_block_time() + SCORUM_MAX_TIME_UNTIL_EXPIRATION);
    tx.sign(moder.post_key, db.get_chain_id());

    SCORUM_REQUIRE_THROW(db.push_transaction(tx, 0), tx_missing_active_auth);
}

BOOST_AUTO_TEST_SUITE_END()
}