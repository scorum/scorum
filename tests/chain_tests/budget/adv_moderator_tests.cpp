#include <boost/test/unit_test.hpp>
#include <scorum/chain/services/development_committee.hpp>
#include <scorum/chain/schema/dev_committee_object.hpp>

#include <boost/uuid/uuid_generators.hpp>

#include "budget_check_common.hpp"

namespace advertising_moderator_tests {

using namespace budget_check_common;

class advertising_moderator_fixture : public budget_check_fixture
{
public:
    advertising_moderator_fixture()
        : account_service(db.account_service())
        , dev_committee_svc(db.development_committee_service())
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

    boost::uuids::uuid ns_uuid = boost::uuids::string_generator()("00000000-0000-0000-0000-000000000001");
    boost::uuids::name_generator uuid_gen = boost::uuids::name_generator(ns_uuid);

    development_committee_service_i& dev_committee_svc;
};

BOOST_FIXTURE_TEST_SUITE(advertising_moderator_tests, advertising_moderator_fixture)

SCORUM_TEST_CASE(should_close_post_budget)
{
    BOOST_REQUIRE_EQUAL(post_budget_service.get_budgets().size(), 0u);

    auto uuid = uuid_gen("alice");
    create_budget(uuid, alice, budget_type::post);

    BOOST_REQUIRE_EQUAL(post_budget_service.get_budgets().size(), 1u);

    empower_advertising_moderator(moder);

    close_budget_by_advertising_moderator_operation close_budget_op;
    close_budget_op.moderator = moder.name;
    close_budget_op.uuid = uuid;
    close_budget_op.type = budget_type::post;

    push_operation(close_budget_op, moder.private_key);

    BOOST_REQUIRE_EQUAL(post_budget_service.get_budgets().size(), 0u);
}

SCORUM_TEST_CASE(should_close_banner_budget)
{
    BOOST_REQUIRE_EQUAL(banner_budget_service.get_budgets().size(), 0u);

    auto uuid = uuid_gen("alice");
    create_budget(uuid, alice, budget_type::banner);

    BOOST_REQUIRE_EQUAL(banner_budget_service.get_budgets().size(), 1u);

    empower_advertising_moderator(moder);

    close_budget_by_advertising_moderator_operation close_budget_op;
    close_budget_op.moderator = moder.name;
    close_budget_op.uuid = uuid;
    close_budget_op.type = budget_type::banner;

    push_operation(close_budget_op, moder.private_key);

    BOOST_REQUIRE_EQUAL(banner_budget_service.get_budgets().size(), 0u);
}

SCORUM_TEST_CASE(should_immidiately_return_the_rest_to_owner)
{
    const auto& alice_acc = account_service.get_account(alice.name);
    const auto& dev_committee = dev_committee_svc.get();

    BOOST_REQUIRE_EQUAL(banner_budget_service.get_budgets().size(), 0u);

    auto start = 1;
    auto deadline = 5;
    auto uuid = uuid_gen("alice");

    create_budget(uuid, alice, budget_type::banner, 1000, start, deadline);
    auto alice_balance_before = alice_acc.balance;
    auto dev_committee_balance_before = dev_committee.scr_balance;

    BOOST_REQUIRE_EQUAL(banner_budget_service.get_budgets().size(), 1u);
    BOOST_REQUIRE_EQUAL(banner_budget_service.get(0).balance.amount, 1000u);
    BOOST_REQUIRE_EQUAL(banner_budget_service.get(0).per_block.amount, 200u);
    BOOST_REQUIRE_EQUAL(banner_budget_service.get(0).budget_pending_outgo.amount, 0u);
    BOOST_REQUIRE_EQUAL(banner_budget_service.get(0).owner_pending_income.amount, 0u);

    generate_block();

    BOOST_CHECK_EQUAL(banner_budget_service.get(0).balance.amount, 800u);
    BOOST_CHECK_EQUAL(banner_budget_service.get(0).budget_pending_outgo.amount, 200u);
    BOOST_CHECK_EQUAL(alice_acc.balance, alice_balance_before);
    BOOST_CHECK_EQUAL(dev_committee.scr_balance, dev_committee_balance_before);

    empower_advertising_moderator(moder); // 2 blocks generated here

    BOOST_CHECK_EQUAL(banner_budget_service.get(0).balance.amount, 400u);
    BOOST_CHECK_EQUAL(banner_budget_service.get(0).budget_pending_outgo.amount, 600u);

    close_budget_by_advertising_moderator_operation close_budget_op;
    close_budget_op.moderator = moder.name;
    close_budget_op.uuid = uuid;
    close_budget_op.type = budget_type::banner;

    push_operation(close_budget_op, moder.private_key);

    BOOST_CHECK_EQUAL(banner_budget_service.get_budgets().size(), 0u);
    BOOST_CHECK_EQUAL(alice_acc.balance, alice_balance_before + 400u);
    BOOST_CHECK_EQUAL(dev_committee.scr_balance,
                      dev_committee_balance_before
                          + SCORUM_DEV_TEAM_PER_BLOCK_REWARD_PERCENT * 600u / SCORUM_100_PERCENT);
}

SCORUM_TEST_CASE(check_moderator_changing)
{
    BOOST_REQUIRE_EQUAL(banner_budget_service.get_budgets().size(), 0u);

    auto uuid0 = uuid_gen("alice0");
    create_budget(uuid0, alice, budget_type::banner, BUDGET_BALANCE_DEFAULT,
                  100 * BUDGET_DEADLINE_IN_BLOCKS_DEFAULT); // budget 0

    auto uuid1 = uuid_gen("alice1");
    create_budget(uuid1, alice, budget_type::banner, BUDGET_BALANCE_DEFAULT,
                  100 * BUDGET_DEADLINE_IN_BLOCKS_DEFAULT); // budget 1

    BOOST_REQUIRE_EQUAL(banner_budget_service.get_budgets().size(), 2u);

    empower_advertising_moderator(moder);

    close_budget_by_advertising_moderator_operation close_budget_op;
    close_budget_op.moderator = bob.name;
    close_budget_op.uuid = uuid0;
    close_budget_op.type = budget_type::banner;

    BOOST_REQUIRE_THROW(push_operation(close_budget_op, bob.private_key),
                        fc::assert_exception); // 'bob' is not a moderator

    close_budget_op.moderator = moder.name;
    push_operation(close_budget_op, moder.private_key); // 'moder' is moderator

    BOOST_REQUIRE_EQUAL(banner_budget_service.get_budgets().size(), 1u);

    empower_advertising_moderator(bob); // now 'bob' became moderator ('moder' is not)

    close_budget_op.uuid = uuid1;
    close_budget_op.moderator = moder.name;
    BOOST_REQUIRE_THROW(push_operation(close_budget_op, moder.private_key), fc::assert_exception);

    close_budget_op.moderator = bob.name;
    push_operation(close_budget_op, bob.private_key);

    BOOST_REQUIRE_EQUAL(banner_budget_service.get_budgets().size(), 0u);
}

SCORUM_TEST_CASE(should_throw_moderator_not_exists)
{
    auto uuid = uuid_gen("alice");
    create_budget(uuid, alice, budget_type::post);

    close_budget_by_advertising_moderator_operation op;
    op.moderator = moder.name;
    op.uuid = uuid;
    op.type = budget_type::post;

    BOOST_REQUIRE_THROW(push_operation(op, moder.private_key), fc::assert_exception);
}

SCORUM_TEST_CASE(should_throw_moder_is_not_a_moderator)
{
    auto uuid = uuid_gen("alice");
    create_budget(uuid, alice, budget_type::post);

    empower_advertising_moderator(alice);

    close_budget_by_advertising_moderator_operation op;
    op.moderator = moder.name;
    op.uuid = uuid;
    op.type = budget_type::post;

    BOOST_REQUIRE_THROW(push_operation(op, moder.private_key), fc::assert_exception);
}

SCORUM_TEST_CASE(should_throw_usual_user_trying_close_budget)
{
    auto uuid = uuid_gen("alice");
    create_budget(uuid, alice, budget_type::post);

    empower_advertising_moderator(moder);

    close_budget_by_advertising_moderator_operation op;
    op.moderator = initdelegate.name;
    op.uuid = uuid;
    op.type = budget_type::post;

    BOOST_REQUIRE_THROW(push_operation(op, initdelegate.private_key), fc::assert_exception);
}

SCORUM_TEST_CASE(should_throw_active_key_required)
{
    auto uuid = uuid_gen("alice");
    create_budget(uuid, alice, budget_type::post);

    empower_advertising_moderator(moder);

    close_budget_by_advertising_moderator_operation op;
    op.moderator = moder.name;
    op.uuid = uuid;
    op.type = budget_type::post;

    signed_transaction tx;
    tx.operations.push_back(op);
    tx.set_expiration(db.head_block_time() + SCORUM_MAX_TIME_UNTIL_EXPIRATION);
    tx.sign(moder.post_key, db.get_chain_id());

    SCORUM_REQUIRE_THROW(db.push_transaction(tx, 0), tx_missing_active_auth);
}

SCORUM_TEST_CASE(single_auction_coeff_test)
{
    empower_advertising_moderator(initdelegate);

    development_committee_change_banner_budgets_auction_properties_operation inner_op;
    inner_op.auction_coefficients = { 50 };

    proposal_create_operation op;
    op.operation = inner_op;
    op.creator = initdelegate.name;
    op.lifetime_sec = SCORUM_PROPOSAL_LIFETIME_MIN_SECONDS;

    BOOST_REQUIRE_NO_THROW(push_operation(op, initdelegate.private_key));
}

SCORUM_TEST_CASE(should_raise_closing_virt_op_after_force_closing_by_moder)
{
    auto was_raised = false;
    db.pre_apply_operation.connect([&](const operation_notification& op_notif) {
        op_notif.op.weak_visit([&](const budget_closing_operation&) { was_raised = true; });
    });

    auto uuid = uuid_gen("alice");
    create_budget(uuid, alice, budget_type::post, 1000, 1, 10);

    generate_blocks(3);

    empower_advertising_moderator(moder);

    BOOST_CHECK(!was_raised);

    close_budget_by_advertising_moderator_operation op;
    op.moderator = moder.name;
    op.uuid = uuid;
    op.type = budget_type::post;

    push_operation(op);

    BOOST_CHECK(was_raised);
}

BOOST_AUTO_TEST_SUITE_END()
}
