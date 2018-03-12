#include <boost/test/unit_test.hpp>

#include <scorum/chain/services/account.hpp>
#include <scorum/chain/services/withdraw_scorumpower.hpp>

#include <scorum/chain/schema/account_objects.hpp>
#include <scorum/chain/schema/withdraw_scorumpower_objects.hpp>

#include "withdraw_scorumpower_check_common.hpp"

#include <scorum/chain/services/dynamic_global_property.hpp>

using namespace scorum::protocol;
using namespace scorum::chain;

BOOST_AUTO_TEST_SUITE(withdraw_scorumpower_route_from_account_to_account_tests)

struct withdraw_scorumpower_route_from_account_to_account_tests_fixture : public withdraw_scorumpower_check_fixture
{
    withdraw_scorumpower_route_from_account_to_account_tests_fixture()
    {
        ACTORS((alice)(bob)(sam));
        alice_key = alice_private_key;

        alice_to_withdraw_sp = ASSET_SP(1e+4) * SCORUM_VESTING_WITHDRAW_INTERVALS;
        alice_to_withdraw_scr = asset(alice_to_withdraw_sp.amount, SCORUM_SYMBOL);
        vest("alice", alice_to_withdraw_scr);

        generate_block();
    }

    void start_withdraw()
    {
        const auto& alice = account_service.get_account("alice");

        withdraw_scorumpower_operation op;
        op.account = alice.name;
        op.scorumpower = alice_to_withdraw_sp;

        signed_transaction tx;
        tx.operations.push_back(op);
        tx.set_expiration(db.head_block_time() + SCORUM_MAX_TIME_UNTIL_EXPIRATION);
        tx.sign(alice_key, db.get_chain_id());
        db.push_transaction(tx, 0);
    }

    private_key_type alice_key;
    asset alice_to_withdraw_sp;
    asset alice_to_withdraw_scr;
};

#ifdef LOCK_WITHDRAW_SCORUMPOWER_OPERATIONS
BOOST_FIXTURE_TEST_CASE(withdraw_is_locked_check, withdraw_scorumpower_route_from_account_to_account_tests_fixture)
{
    SCORUM_REQUIRE_THROW(start_withdraw(), fc::assert_exception);
}
BOOST_FIXTURE_TEST_CASE(withdraw_will_be_unlocked_check,
                        withdraw_scorumpower_route_from_account_to_account_tests_fixture)
{
    fc::time_point_sec time_until
        = fc::time_point_sec::from_iso_string(BOOST_PP_STRINGIZE(WITHDRAW_SCORUMPOWER_LOCK_UNTIL_DATE));
    generate_blocks(time_until + SCORUM_BLOCK_INTERVAL, true);
    dynamic_global_property_service_i& _dprops_service = db.dynamic_global_property_service();
    BOOST_REQUIRE_LT(time_until.sec_since_epoch(), _dprops_service.head_block_time().sec_since_epoch());

    BOOST_REQUIRE_NO_THROW(start_withdraw());
}
#else // LOCK_WITHDRAW_SCORUMPOWER_OPERATIONS
BOOST_FIXTURE_TEST_CASE(withdraw_all_no_rest_check, withdraw_scorumpower_route_from_account_to_account_tests_fixture)
{
    const auto& alice = account_service.get_account("alice");

    asset old_balance = alice.balance;

    start_withdraw();

    BOOST_REQUIRE(withdraw_scorumpower_service.is_exists(alice.id));

    validate_database();

    fc::time_point_sec start_time = db.head_block_time();

    auto next_withdrawal = db.head_block_time() + SCORUM_VESTING_WITHDRAW_INTERVAL_SECONDS;

    generate_blocks(next_withdrawal + (SCORUM_BLOCK_INTERVAL / 2), true);

    BOOST_CHECK_EQUAL(alice.balance - old_balance, alice_to_withdraw_scr / SCORUM_VESTING_WITHDRAW_INTERVALS);

    BOOST_REQUIRE(withdraw_scorumpower_service.is_exists(alice.id));

    for (int ci = 1; ci < SCORUM_VESTING_WITHDRAW_INTERVALS; ++ci)
    {
        next_withdrawal = db.head_block_time() + SCORUM_VESTING_WITHDRAW_INTERVAL_SECONDS;
        generate_blocks(next_withdrawal + (SCORUM_BLOCK_INTERVAL / 2), true);
    }

    BOOST_REQUIRE(!withdraw_scorumpower_service.is_exists(alice.id));

    BOOST_CHECK_EQUAL(alice.balance - old_balance, alice_to_withdraw_scr);

    fc::time_point_sec end_time = db.head_block_time();

    BOOST_CHECK_EQUAL((end_time - start_time).to_seconds(),
                      SCORUM_VESTING_WITHDRAW_INTERVAL_SECONDS * SCORUM_VESTING_WITHDRAW_INTERVALS);
}

BOOST_FIXTURE_TEST_CASE(withdraw_all_with_rest_check, withdraw_scorumpower_route_from_account_to_account_tests_fixture)
{
    // add rest
    asset rest_sp = alice_to_withdraw_sp / SCORUM_VESTING_WITHDRAW_INTERVALS / 2;
    alice_to_withdraw_sp += rest_sp;
    asset rest_scr = asset(rest_sp.amount, SCORUM_SYMBOL);
    alice_to_withdraw_scr += rest_scr;
    vest("alice", rest_scr);

    generate_block();

    const auto& alice = account_service.get_account("alice");

    asset old_balance = alice.balance;

    start_withdraw();

    BOOST_REQUIRE(withdraw_scorumpower_service.is_exists(alice.id));

    validate_database();

    fc::time_point_sec start_time = db.head_block_time();

    auto next_withdrawal = db.head_block_time() + SCORUM_VESTING_WITHDRAW_INTERVAL_SECONDS;

    generate_blocks(next_withdrawal + (SCORUM_BLOCK_INTERVAL / 2), true);

    BOOST_CHECK_EQUAL(alice.balance - old_balance, alice_to_withdraw_scr / SCORUM_VESTING_WITHDRAW_INTERVALS);

    BOOST_REQUIRE(withdraw_scorumpower_service.is_exists(alice.id));

    for (int ci = 1; ci < SCORUM_VESTING_WITHDRAW_INTERVALS + 1; ++ci)
    {
        next_withdrawal = db.head_block_time() + SCORUM_VESTING_WITHDRAW_INTERVAL_SECONDS;
        generate_blocks(next_withdrawal + (SCORUM_BLOCK_INTERVAL / 2), true);
    }

    BOOST_REQUIRE(!withdraw_scorumpower_service.is_exists(alice.id));

    BOOST_CHECK_EQUAL(alice.balance - old_balance, alice_to_withdraw_scr);

    fc::time_point_sec end_time = db.head_block_time();

    BOOST_CHECK_EQUAL((end_time - start_time).to_seconds(),
                      SCORUM_VESTING_WITHDRAW_INTERVAL_SECONDS * (SCORUM_VESTING_WITHDRAW_INTERVALS + 1));
}

BOOST_FIXTURE_TEST_CASE(withdrawal_tree_check, withdraw_scorumpower_route_from_account_to_account_tests_fixture)
{
    static const int bob_pie_percent = 10;
    static const int sam_pie_percent = 20;
    static const int alice_withdrawal_percent = 100 - (bob_pie_percent + sam_pie_percent);

    const auto& alice = account_service.get_account("alice");
    const auto& bob = account_service.get_account("bob");
    const auto& sam = account_service.get_account("sam");

    asset old_alice_balance_scr = alice.balance;
    asset old_bob_balance_scr = bob.balance;
    asset old_sam_balance_sp = sam.scorumpower;

    withdraw_scorumpower_operation op_wv;
    op_wv.account = alice.name;
    op_wv.scorumpower = alice_to_withdraw_sp;

    set_withdraw_scorumpower_route_to_account_operation op_wvr_bob;
    op_wvr_bob.percent = bob_pie_percent * SCORUM_1_PERCENT;
    op_wvr_bob.from_account = alice.name;
    op_wvr_bob.to_account = bob.name;
    op_wvr_bob.auto_vest = false; // route to SCR (default)

    set_withdraw_scorumpower_route_to_account_operation op_wvr_sam;
    op_wvr_sam.percent = sam_pie_percent * SCORUM_1_PERCENT;
    op_wvr_sam.from_account = alice.name;
    op_wvr_sam.to_account = sam.name;
    op_wvr_sam.auto_vest = true; // route to SP

    signed_transaction tx;
    tx.operations.push_back(op_wv);
    tx.operations.push_back(op_wvr_bob);
    tx.operations.push_back(op_wvr_sam);

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
    BOOST_CHECK_EQUAL(bob.balance - old_bob_balance_scr,
                      alice_to_withdraw_scr * bob_pie_percent / 100 / SCORUM_VESTING_WITHDRAW_INTERVALS);
    asset sam_recived = alice_to_withdraw_scr * sam_pie_percent / 100 / SCORUM_VESTING_WITHDRAW_INTERVALS;
    BOOST_CHECK_EQUAL(sam.scorumpower - old_sam_balance_sp, asset(sam_recived.amount, SP_SYMBOL));

    BOOST_REQUIRE(withdraw_scorumpower_service.is_exists(alice.id));

    for (int ci = 1; ci < SCORUM_VESTING_WITHDRAW_INTERVALS; ++ci)
    {
        next_withdrawal = db.head_block_time() + SCORUM_VESTING_WITHDRAW_INTERVAL_SECONDS;
        generate_blocks(next_withdrawal + (SCORUM_BLOCK_INTERVAL / 2), true);
    }

    BOOST_REQUIRE(!withdraw_scorumpower_service.is_exists(alice.id));

    BOOST_CHECK_EQUAL(alice.balance - old_alice_balance_scr, alice_to_withdraw_scr * alice_withdrawal_percent / 100);
    BOOST_CHECK_EQUAL(bob.balance - old_bob_balance_scr, alice_to_withdraw_scr * bob_pie_percent / 100);
    sam_recived = alice_to_withdraw_scr * sam_pie_percent / 100;
    BOOST_CHECK_EQUAL(sam.scorumpower - old_sam_balance_sp, asset(sam_recived.amount, SP_SYMBOL));

    fc::time_point_sec end_time = db.head_block_time();

    BOOST_CHECK_EQUAL((end_time - start_time).to_seconds(),
                      SCORUM_VESTING_WITHDRAW_INTERVAL_SECONDS * SCORUM_VESTING_WITHDRAW_INTERVALS);
}

#endif //! LOCK_WITHDRAW_SCORUMPOWER_OPERATIONS
BOOST_AUTO_TEST_SUITE_END()
