#include <boost/test/unit_test.hpp>

#include <scorum/chain/services/account.hpp>
#include <scorum/chain/services/withdraw_vesting.hpp>

#include <scorum/chain/schema/account_objects.hpp>
#include <scorum/chain/schema/withdraw_vesting_objects.hpp>

#include "withdraw_vesting_check_common.hpp"

using namespace scorum::protocol;
using namespace scorum::chain;

BOOST_AUTO_TEST_SUITE(withdraw_vesting_route_from_account_to_account_tests)

struct withdraw_vesting_route_from_account_to_account_tests_fixture : public withdraw_vesting_check_fixture
{
    withdraw_vesting_route_from_account_to_account_tests_fixture()
    {
        ACTORS((alice)(bob)(sam));
        alice_key = alice_private_key;

        alice_to_withdraw_sp = ASSET_SP(1e+9) * SCORUM_VESTING_WITHDRAW_INTERVALS;
        alice_to_withdraw_scr = asset(alice_to_withdraw_sp.amount, SCORUM_SYMBOL);
        vest("alice", alice_to_withdraw_scr);

        generate_blocks(5);
    }

    private_key_type alice_key;
    asset alice_to_withdraw_sp;
    asset alice_to_withdraw_scr;
};

BOOST_FIXTURE_TEST_CASE(withdraw_all_no_rest_check, withdraw_vesting_route_from_account_to_account_tests_fixture)
{
    const auto& alice = account_service.get_account("alice");

    asset old_balance = alice.balance;

    withdraw_vesting_operation op;
    op.account = alice.name;
    op.vesting_shares = alice_to_withdraw_sp;

    signed_transaction tx;
    tx.operations.push_back(op);
    tx.set_expiration(db.head_block_time() + SCORUM_MAX_TIME_UNTIL_EXPIRATION);
    tx.sign(alice_key, db.get_chain_id());
    db.push_transaction(tx, 0);

    BOOST_REQUIRE(withdraw_vesting_service.is_exists(alice.id));

    validate_database();

    fc::time_point_sec start_time = db.head_block_time();

    auto next_withdrawal = db.head_block_time() + SCORUM_VESTING_WITHDRAW_INTERVAL_SECONDS;

    generate_blocks(next_withdrawal + (SCORUM_BLOCK_INTERVAL / 2), true);

    BOOST_REQUIRE_EQUAL(alice.balance - old_balance, alice_to_withdraw_scr / SCORUM_VESTING_WITHDRAW_INTERVALS);

    BOOST_REQUIRE(withdraw_vesting_service.is_exists(alice.id));

    for (int ci = 1; ci < SCORUM_VESTING_WITHDRAW_INTERVALS; ++ci)
    {
        next_withdrawal = db.head_block_time() + SCORUM_VESTING_WITHDRAW_INTERVAL_SECONDS;
        generate_blocks(next_withdrawal + (SCORUM_BLOCK_INTERVAL / 2), true);
    }

    BOOST_REQUIRE(!withdraw_vesting_service.is_exists(alice.id));

    BOOST_REQUIRE_EQUAL(alice.balance - old_balance, alice_to_withdraw_scr);

    fc::time_point_sec end_time = db.head_block_time();

    BOOST_REQUIRE_EQUAL((end_time - start_time).to_seconds(),
                        SCORUM_VESTING_WITHDRAW_INTERVAL_SECONDS * SCORUM_VESTING_WITHDRAW_INTERVALS);
}

BOOST_FIXTURE_TEST_CASE(withdraw_all_with_rest_check, withdraw_vesting_route_from_account_to_account_tests_fixture)
{
    asset rest_sp = alice_to_withdraw_sp / SCORUM_VESTING_WITHDRAW_INTERVALS / 2;
    alice_to_withdraw_sp += rest_sp;
    asset rest_scr = asset(rest_sp.amount, SCORUM_SYMBOL);
    alice_to_withdraw_scr += rest_scr;
    vest("alice", rest_scr);

    generate_block();

    const auto& alice = account_service.get_account("alice");

    asset old_balance = alice.balance;

    withdraw_vesting_operation op;
    op.account = alice.name;
    op.vesting_shares = alice_to_withdraw_sp;

    signed_transaction tx;
    tx.operations.push_back(op);
    tx.set_expiration(db.head_block_time() + SCORUM_MAX_TIME_UNTIL_EXPIRATION);
    tx.sign(alice_key, db.get_chain_id());
    db.push_transaction(tx, 0);

    BOOST_REQUIRE(withdraw_vesting_service.is_exists(alice.id));

    validate_database();

    fc::time_point_sec start_time = db.head_block_time();

    auto next_withdrawal = db.head_block_time() + SCORUM_VESTING_WITHDRAW_INTERVAL_SECONDS;

    generate_blocks(next_withdrawal + (SCORUM_BLOCK_INTERVAL / 2), true);

    BOOST_REQUIRE_EQUAL(alice.balance - old_balance, alice_to_withdraw_scr / SCORUM_VESTING_WITHDRAW_INTERVALS);

    BOOST_REQUIRE(withdraw_vesting_service.is_exists(alice.id));

    for (int ci = 1; ci < SCORUM_VESTING_WITHDRAW_INTERVALS + 1; ++ci)
    {
        next_withdrawal = db.head_block_time() + SCORUM_VESTING_WITHDRAW_INTERVAL_SECONDS;
        generate_blocks(next_withdrawal + (SCORUM_BLOCK_INTERVAL / 2), true);
    }

    BOOST_REQUIRE(!withdraw_vesting_service.is_exists(alice.id));

    BOOST_REQUIRE_EQUAL(alice.balance - old_balance, alice_to_withdraw_scr);

    fc::time_point_sec end_time = db.head_block_time();

    BOOST_REQUIRE_EQUAL((end_time - start_time).to_seconds(),
                        SCORUM_VESTING_WITHDRAW_INTERVAL_SECONDS * (SCORUM_VESTING_WITHDRAW_INTERVALS + 1));
}

BOOST_FIXTURE_TEST_CASE(withdrawal_tree_check, withdraw_vesting_route_from_account_to_account_tests_fixture)
{
    // TODO
}

BOOST_AUTO_TEST_SUITE_END()
