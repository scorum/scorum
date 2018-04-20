#include <boost/test/unit_test.hpp>

#include <scorum/chain/services/account.hpp>
#include <scorum/chain/services/withdraw_scorumpower.hpp>
#include <scorum/chain/services/dev_pool.hpp>

#include <scorum/chain/schema/account_objects.hpp>
#include <scorum/chain/schema/dev_committee_object.hpp>
#include <scorum/chain/schema/withdraw_scorumpower_objects.hpp>

#include "withdraw_scorumpower_check_common.hpp"

using namespace database_fixture;

BOOST_AUTO_TEST_SUITE(withdraw_scorumpower_route_from_account_to_dev_pool_tests)

struct withdraw_scorumpower_route_from_account_to_dev_pool_tests_fixture : public withdraw_scorumpower_check_fixture
{
    withdraw_scorumpower_route_from_account_to_dev_pool_tests_fixture()
    {
        set_dev_pool_balance(ASSET_SP(1000), ASSET_SCR(2000));

        ACTOR(alice);
        alice_key = alice_private_key;

        alice_to_withdraw_sp = ASSET_SP(1e+4) * SCORUM_VESTING_WITHDRAW_INTERVALS;
        alice_to_withdraw_scr = asset(alice_to_withdraw_sp.amount, SCORUM_SYMBOL);
        vest("alice", alice_to_withdraw_scr);

        generate_block();
    }

    private_key_type alice_key;
    asset alice_to_withdraw_sp;
    asset alice_to_withdraw_scr;
};

BOOST_FIXTURE_TEST_CASE(withdrawal_route_scr_check, withdraw_scorumpower_route_from_account_to_dev_pool_tests_fixture)
{
    static const int pool_scr_pie_percent = 10;
    static const int alice_withdrawal_percent = 100 - pool_scr_pie_percent;

    const auto& alice = account_service.get_account("alice");
    const auto& pool = pool_service.get();

    asset old_alice_balance_scr = alice.balance;
    asset old_pool_balance_scr = pool.scr_balance;

    withdraw_scorumpower_operation op_wv;
    op_wv.account = alice.name;
    op_wv.scorumpower = alice_to_withdraw_sp;

    set_withdraw_scorumpower_route_to_dev_pool_operation op_wvr_scr;
    op_wvr_scr.percent = pool_scr_pie_percent * SCORUM_1_PERCENT;
    op_wvr_scr.from_account = alice.name;
    op_wvr_scr.auto_vest = false; // route to SCR (default)

    signed_transaction tx;
    tx.operations.push_back(op_wv);
    tx.operations.push_back(op_wvr_scr);

    tx.set_expiration(db.head_block_time() + SCORUM_MAX_TIME_UNTIL_EXPIRATION);
    tx.sign(alice_key, db.get_chain_id());
    db.push_transaction(tx, 0);

    BOOST_REQUIRE(withdraw_scorumpower_service.is_exists(alice.id));

    validate_database();

    fc::time_point_sec start_time = db.head_block_time();

    auto next_withdrawal = db.head_block_time() + SCORUM_VESTING_WITHDRAW_INTERVAL_SECONDS;

    generate_blocks(next_withdrawal + (SCORUM_BLOCK_INTERVAL / 2), true);

    BOOST_CHECK_EQUAL(alice.balance - old_alice_balance_scr,
                      alice_to_withdraw_scr * alice_withdrawal_percent / 100 / SCORUM_VESTING_WITHDRAW_INTERVALS);
    BOOST_CHECK_EQUAL(pool.scr_balance - old_pool_balance_scr,
                      alice_to_withdraw_scr * pool_scr_pie_percent / 100 / SCORUM_VESTING_WITHDRAW_INTERVALS);

    BOOST_REQUIRE(withdraw_scorumpower_service.is_exists(alice.id));

    for (uint32_t ci = 1; ci < SCORUM_VESTING_WITHDRAW_INTERVALS; ++ci)
    {
        next_withdrawal = db.head_block_time() + SCORUM_VESTING_WITHDRAW_INTERVAL_SECONDS;
        generate_blocks(next_withdrawal + (SCORUM_BLOCK_INTERVAL / 2), true);
    }

    BOOST_REQUIRE(!withdraw_scorumpower_service.is_exists(alice.id));

    BOOST_CHECK_EQUAL(alice.balance - old_alice_balance_scr, alice_to_withdraw_scr * alice_withdrawal_percent / 100);
    BOOST_CHECK_EQUAL(pool.scr_balance - old_pool_balance_scr, alice_to_withdraw_scr * pool_scr_pie_percent / 100);

    fc::time_point_sec end_time = db.head_block_time();

    BOOST_CHECK_EQUAL((end_time - start_time).to_seconds(),
                      SCORUM_VESTING_WITHDRAW_INTERVAL_SECONDS * SCORUM_VESTING_WITHDRAW_INTERVALS);
}

BOOST_FIXTURE_TEST_CASE(withdrawal_route_sp_check, withdraw_scorumpower_route_from_account_to_dev_pool_tests_fixture)
{
    static const int pool_sp_pie_percent = 20;
    static const int alice_withdrawal_percent = 100 - pool_sp_pie_percent;

    const auto& alice = account_service.get_account("alice");
    const auto& pool = pool_service.get();

    asset old_alice_balance_scr = alice.balance;
    asset old_pool_balance_sp = pool.sp_balance;

    withdraw_scorumpower_operation op_wv;
    op_wv.account = alice.name;
    op_wv.scorumpower = alice_to_withdraw_sp;

    set_withdraw_scorumpower_route_to_dev_pool_operation op_wvr_sp;
    op_wvr_sp.percent = pool_sp_pie_percent * SCORUM_1_PERCENT;
    op_wvr_sp.from_account = alice.name;
    op_wvr_sp.auto_vest = true; // route to SP

    signed_transaction tx;
    tx.operations.push_back(op_wv);
    tx.operations.push_back(op_wvr_sp);

    tx.set_expiration(db.head_block_time() + SCORUM_MAX_TIME_UNTIL_EXPIRATION);
    tx.sign(alice_key, db.get_chain_id());
    db.push_transaction(tx, 0);

    BOOST_REQUIRE(withdraw_scorumpower_service.is_exists(alice.id));

    validate_database();

    fc::time_point_sec start_time = db.head_block_time();

    auto next_withdrawal = db.head_block_time() + SCORUM_VESTING_WITHDRAW_INTERVAL_SECONDS;

    generate_blocks(next_withdrawal + (SCORUM_BLOCK_INTERVAL / 2), true);

    BOOST_CHECK_EQUAL(alice.balance - old_alice_balance_scr,
                      alice_to_withdraw_scr * alice_withdrawal_percent / 100 / SCORUM_VESTING_WITHDRAW_INTERVALS);
    asset pool_recived = alice_to_withdraw_scr * pool_sp_pie_percent / 100 / SCORUM_VESTING_WITHDRAW_INTERVALS;
    BOOST_CHECK_EQUAL(pool.sp_balance - old_pool_balance_sp, asset(pool_recived.amount, SP_SYMBOL));

    BOOST_REQUIRE(withdraw_scorumpower_service.is_exists(alice.id));

    for (uint32_t ci = 1; ci < SCORUM_VESTING_WITHDRAW_INTERVALS; ++ci)
    {
        next_withdrawal = db.head_block_time() + SCORUM_VESTING_WITHDRAW_INTERVAL_SECONDS;
        generate_blocks(next_withdrawal + (SCORUM_BLOCK_INTERVAL / 2), true);
    }

    BOOST_REQUIRE(!withdraw_scorumpower_service.is_exists(alice.id));

    BOOST_CHECK_EQUAL(alice.balance - old_alice_balance_scr, alice_to_withdraw_scr * alice_withdrawal_percent / 100);
    pool_recived = alice_to_withdraw_scr * pool_sp_pie_percent / 100;
    BOOST_CHECK_EQUAL(pool.sp_balance - old_pool_balance_sp, asset(pool_recived.amount, SP_SYMBOL));

    fc::time_point_sec end_time = db.head_block_time();

    BOOST_CHECK_EQUAL((end_time - start_time).to_seconds(),
                      SCORUM_VESTING_WITHDRAW_INTERVAL_SECONDS * SCORUM_VESTING_WITHDRAW_INTERVALS);
}

BOOST_AUTO_TEST_SUITE_END()
