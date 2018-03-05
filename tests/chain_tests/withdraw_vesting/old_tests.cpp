#include <boost/test/unit_test.hpp>

#include "database_default_integration.hpp"

#include <scorum/chain/services/account.hpp>
#include <scorum/chain/services/withdraw_vesting_route.hpp>
#include <scorum/chain/services/withdraw_vesting_route_statistic.hpp>
#include <scorum/chain/services/withdraw_vesting.hpp>

#include <scorum/chain/schema/account_objects.hpp>
#include <scorum/chain/schema/withdraw_vesting_objects.hpp>

#include <scorum/account_history/account_history_plugin.hpp>

using namespace scorum::chain;
using namespace scorum::protocol;

struct withdraw_vesting_tests_fixture : public database_default_integration_fixture
{
    withdraw_vesting_tests_fixture()
        : account_service(db.account_service())
        , withdraw_vesting_service(db.withdraw_vesting_service())
    {
    }

    account_service_i& account_service;
    withdraw_vesting_service_i& withdraw_vesting_service;
    private_key_type sign_key;
};

struct withdraw_vesting_apply_tests_fixture : public withdraw_vesting_tests_fixture
{
    withdraw_vesting_apply_tests_fixture()
    {
        ACTORS((alice))
        generate_block();
        vest("alice", ASSET_SCR(10e+3));

        sign_key = alice_private_key;

        generate_block();
        validate_database();
    }
};

BOOST_FIXTURE_TEST_SUITE(withdraw_vesting_apply_tests, withdraw_vesting_apply_tests_fixture)

SCORUM_TEST_CASE(withdraw_vesting_apply)
{
    const auto& alice = account_service.get_account("alice");

    BOOST_TEST_MESSAGE("--- Test withdraw of existing SP");

    withdraw_vesting_operation op;
    op.account = "alice";
    op.vesting_shares = asset(alice.vesting_shares.amount / 2, VESTS_SYMBOL);

    auto old_vesting_shares = alice.vesting_shares;

    signed_transaction tx;
    tx.operations.push_back(op);
    tx.set_expiration(db.head_block_time() + SCORUM_MAX_TIME_UNTIL_EXPIRATION);
    tx.sign(sign_key, db.get_chain_id());
    db.push_transaction(tx, 0);

    BOOST_REQUIRE(withdraw_vesting_service.is_exists(alice.id));

    const auto& alice_wvo = withdraw_vesting_service.get(alice.id);

    BOOST_REQUIRE(alice.vesting_shares.amount.value == old_vesting_shares.amount.value);
    BOOST_REQUIRE(alice_wvo.vesting_withdraw_rate.amount.value
                  == (old_vesting_shares.amount / (SCORUM_VESTING_WITHDRAW_INTERVALS * 2)).value);
    BOOST_REQUIRE(alice_wvo.to_withdraw == op.vesting_shares);
    BOOST_REQUIRE(alice_wvo.next_vesting_withdrawal == db.head_block_time() + SCORUM_VESTING_WITHDRAW_INTERVAL_SECONDS);
    validate_database();

    BOOST_TEST_MESSAGE("--- Test changing vesting withdrawal");
    tx.operations.clear();
    tx.signatures.clear();

    op.vesting_shares = asset(alice.vesting_shares.amount / 3, VESTS_SYMBOL);
    tx.operations.push_back(op);
    tx.set_expiration(db.head_block_time() + SCORUM_MAX_TIME_UNTIL_EXPIRATION);
    tx.sign(sign_key, db.get_chain_id());
    db.push_transaction(tx, 0);

    BOOST_REQUIRE(alice.vesting_shares.amount.value == old_vesting_shares.amount.value);
    BOOST_REQUIRE(alice_wvo.vesting_withdraw_rate.amount.value
                  == (old_vesting_shares.amount / (SCORUM_VESTING_WITHDRAW_INTERVALS * 3)).value);
    BOOST_REQUIRE(alice_wvo.to_withdraw == op.vesting_shares);
    BOOST_REQUIRE(alice_wvo.next_vesting_withdrawal == db.head_block_time() + SCORUM_VESTING_WITHDRAW_INTERVAL_SECONDS);
    validate_database();

    BOOST_TEST_MESSAGE("--- Test withdrawing more vests than available");

    tx.operations.clear();
    tx.signatures.clear();

    op.vesting_shares = asset(alice.vesting_shares.amount * 2, VESTS_SYMBOL);
    tx.operations.push_back(op);
    tx.set_expiration(db.head_block_time() + SCORUM_MAX_TIME_UNTIL_EXPIRATION);
    tx.sign(sign_key, db.get_chain_id());
    SCORUM_REQUIRE_THROW(db.push_transaction(tx, 0), fc::exception);

    BOOST_REQUIRE(alice.vesting_shares.amount.value == old_vesting_shares.amount.value);
    BOOST_REQUIRE(alice_wvo.vesting_withdraw_rate.amount.value
                  == (old_vesting_shares.amount / (SCORUM_VESTING_WITHDRAW_INTERVALS * 3)).value);
    BOOST_REQUIRE(alice_wvo.next_vesting_withdrawal == db.head_block_time() + SCORUM_VESTING_WITHDRAW_INTERVAL_SECONDS);
    validate_database();

    BOOST_TEST_MESSAGE("--- Test withdrawing 0 to reset vesting withdraw");
    tx.operations.clear();
    tx.signatures.clear();

    op.vesting_shares = ASSET_NULL_SP;
    tx.operations.push_back(op);
    tx.set_expiration(db.head_block_time() + SCORUM_MAX_TIME_UNTIL_EXPIRATION);
    tx.sign(sign_key, db.get_chain_id());
    db.push_transaction(tx, 0);

    BOOST_REQUIRE(alice.vesting_shares.amount.value == old_vesting_shares.amount.value);
    BOOST_REQUIRE(!withdraw_vesting_service.is_exists(alice.id));

    BOOST_TEST_MESSAGE("--- Test cancelling a withdraw when below the account creation fee");
    op.vesting_shares = alice.vesting_shares;
    tx.clear();
    tx.operations.push_back(op);
    tx.sign(sign_key, db.get_chain_id());
    db.push_transaction(tx, 0);
    generate_block();

    BOOST_REQUIRE(withdraw_vesting_service.is_exists(alice.id));

    tx.clear();
    op.account = "alice";
    op.vesting_shares = ASSET_NULL_SP;
    tx.operations.push_back(op);
    tx.set_expiration(db.head_block_time() + SCORUM_MAX_TIME_UNTIL_EXPIRATION);
    tx.sign(sign_key, db.get_chain_id());
    db.push_transaction(tx, 0);

    BOOST_REQUIRE(!withdraw_vesting_service.is_exists(alice.id));
    validate_database();
}

BOOST_AUTO_TEST_SUITE_END()

struct withdraw_vesting_withdrawals_tests_fixture : public database_trx_integration_fixture
{
    account_service_i& account_service;
    withdraw_vesting_service_i& withdraw_vesting_service;
    private_key_type sign_key;

    withdraw_vesting_withdrawals_tests_fixture()
        : account_service(db.account_service())
        , withdraw_vesting_service(db.withdraw_vesting_service())
    {
        boost::program_options::variables_map options;
        auto ahplugin = app.register_plugin<scorum::account_history::account_history_plugin>();
        ahplugin->plugin_initialize(options);

        open_database();

        ACTORS((alice))
        fund("alice", ASSET_SCR(100e+3));
        vest("alice", ASSET_SCR(100e+3));

        sign_key = alice_private_key;

        generate_block();
        validate_database();
    }
};

BOOST_FIXTURE_TEST_SUITE(withdraw_vesting_withdrawals_tests, withdraw_vesting_withdrawals_tests_fixture)

SCORUM_TEST_CASE(vesting_withdrawals)
{
    const auto& alice = account_service.get_account("alice");

    BOOST_TEST_MESSAGE("Setting up withdrawal");

    signed_transaction tx;
    withdraw_vesting_operation op;
    op.account = "alice";
    op.vesting_shares = asset(alice.vesting_shares.amount / 2, VESTS_SYMBOL);
    tx.set_expiration(db.head_block_time() + SCORUM_MAX_TIME_UNTIL_EXPIRATION);
    tx.operations.push_back(op);
    tx.sign(sign_key, db.get_chain_id());
    db.push_transaction(tx, 0);

    auto alice_id = alice.id;
    const auto& alice_wvo = withdraw_vesting_service.get(alice_id);

    auto next_withdrawal = db.head_block_time() + SCORUM_VESTING_WITHDRAW_INTERVAL_SECONDS;
    asset vesting_shares = alice.vesting_shares;
    asset to_withdraw = op.vesting_shares;
    asset original_vesting = vesting_shares;
    asset withdraw_rate = alice_wvo.vesting_withdraw_rate;

    BOOST_TEST_MESSAGE("Generating block up to first withdrawal");
    generate_blocks(next_withdrawal - (SCORUM_BLOCK_INTERVAL / 2), true);

    BOOST_REQUIRE(account_service.get_account("alice").vesting_shares.amount.value == vesting_shares.amount.value);

    BOOST_TEST_MESSAGE("Generating block to cause withdrawal");
    generate_block();

    auto fill_op = get_last_operations(1)[0].get<fill_vesting_withdraw_operation>();
    auto gpo = db.get_dynamic_global_properties();

    BOOST_REQUIRE(account_service.get_account("alice").vesting_shares.amount.value
                  == (vesting_shares - withdraw_rate).amount.value);
    BOOST_REQUIRE_LE(
        (ASSET_SCR(withdraw_rate.amount.value) - account_service.get_account("alice").balance).amount.value,
        (share_value_type)1);
    BOOST_REQUIRE(fill_op.from_account == "alice");
    BOOST_REQUIRE(fill_op.to_account == "alice");
    BOOST_REQUIRE(fill_op.withdrawn.amount.value == withdraw_rate.amount.value);
    validate_database();

    BOOST_TEST_MESSAGE("Generating the rest of the blocks in the withdrawal");

    vesting_shares = account_service.get_account("alice").vesting_shares;
    auto balance = account_service.get_account("alice").balance;
    auto old_next_vesting = withdraw_vesting_service.get(alice_id).next_vesting_withdrawal;

    for (int i = 1; i < SCORUM_VESTING_WITHDRAW_INTERVALS - 1; i++)
    {
        generate_blocks(db.head_block_time() + SCORUM_VESTING_WITHDRAW_INTERVAL_SECONDS);

        const auto& alice = account_service.get_account("alice");

        gpo = db.get_dynamic_global_properties();
        fill_op = get_last_operations(1)[0].get<fill_vesting_withdraw_operation>();

        BOOST_REQUIRE(alice.vesting_shares.amount.value == (vesting_shares - withdraw_rate).amount.value);
        BOOST_REQUIRE_LE(balance.amount.value + (ASSET_SCR(withdraw_rate.amount.value) - alice.balance).amount.value,
                         (share_value_type)1);
        BOOST_REQUIRE(fill_op.from_account == "alice");
        BOOST_REQUIRE(fill_op.to_account == "alice");
        BOOST_REQUIRE(fill_op.withdrawn.amount.value == withdraw_rate.amount.value);

        if (i == SCORUM_VESTING_WITHDRAW_INTERVALS - 1)
            BOOST_REQUIRE(!withdraw_vesting_service.is_exists(alice.id));
        else
            BOOST_REQUIRE(withdraw_vesting_service.get(alice.id).next_vesting_withdrawal.sec_since_epoch()
                          == (old_next_vesting + SCORUM_VESTING_WITHDRAW_INTERVAL_SECONDS).sec_since_epoch());

        validate_database();

        vesting_shares = alice.vesting_shares;
        balance = alice.balance;
        if (withdraw_vesting_service.is_exists(alice.id))
            old_next_vesting = withdraw_vesting_service.get(alice.id).next_vesting_withdrawal;
    }

    if (to_withdraw.amount.value % withdraw_rate.amount.value != 0)
    {
        BOOST_TEST_MESSAGE("Generating one more block to take care of remainder");
        generate_blocks(db.head_block_time() + SCORUM_VESTING_WITHDRAW_INTERVAL_SECONDS, true);
        fill_op = get_last_operations(1)[0].get<fill_vesting_withdraw_operation>();
        gpo = db.get_dynamic_global_properties();

        const auto& alice_wvo = withdraw_vesting_service.get(alice_id);

        BOOST_REQUIRE(alice_wvo.next_vesting_withdrawal.sec_since_epoch()
                      == (old_next_vesting + SCORUM_VESTING_WITHDRAW_INTERVAL_SECONDS).sec_since_epoch());
        BOOST_REQUIRE(fill_op.from_account == "alice");
        BOOST_REQUIRE(fill_op.to_account == "alice");
        BOOST_REQUIRE(fill_op.withdrawn.amount.value == withdraw_rate.amount.value);

        generate_blocks(db.head_block_time() + SCORUM_VESTING_WITHDRAW_INTERVAL_SECONDS, true);
        gpo = db.get_dynamic_global_properties();
        fill_op = get_last_operations(1)[0].get<fill_vesting_withdraw_operation>();

        BOOST_REQUIRE(alice_wvo.next_vesting_withdrawal.sec_since_epoch()
                      == fc::time_point_sec::maximum().sec_since_epoch());
        BOOST_REQUIRE(fill_op.to_account == "alice");
        BOOST_REQUIRE(fill_op.from_account == "alice");
        BOOST_REQUIRE(fill_op.withdrawn.amount.value == to_withdraw.amount.value % withdraw_rate.amount.value);

        validate_database();
    }
    else
    {
        generate_blocks(db.head_block_time() + SCORUM_VESTING_WITHDRAW_INTERVAL_SECONDS, true);

        BOOST_REQUIRE(!withdraw_vesting_service.is_exists(alice_id));

        fill_op = get_last_operations(1)[0].get<fill_vesting_withdraw_operation>();
        BOOST_REQUIRE(fill_op.from_account == "alice");
        BOOST_REQUIRE(fill_op.to_account == "alice");
        BOOST_REQUIRE(fill_op.withdrawn.amount.value == withdraw_rate.amount.value);
    }

    BOOST_REQUIRE(account_service.get_account("alice").vesting_shares.amount.value
                  == (original_vesting - op.vesting_shares).amount.value);
}

BOOST_AUTO_TEST_SUITE_END()

struct withdraw_vesting_withdraw_route_tests_fixture : public withdraw_vesting_tests_fixture
{
    withdraw_vesting_withdraw_route_tests_fixture()
    {
        ACTORS((alice)(bob)(sam))

        auto original_vesting = alice.vesting_shares;

        fund("alice", ASSET_SCR(104e+4));
        vest("alice", ASSET_SCR(104e+4));

        sign_key = alice_private_key;
        withdraw_amount = alice.vesting_shares - original_vesting;

        generate_block();
        validate_database();
    }

    asset withdraw_amount = asset(0, VESTS_SYMBOL);
};

BOOST_FIXTURE_TEST_SUITE(withdraw_vesting_withdraw_route_tests, withdraw_vesting_withdraw_route_tests_fixture)

SCORUM_TEST_CASE(vesting_withdraw_route)
{
    const auto& alice = account_service.get_account("alice");
    const auto& bob = account_service.get_account("bob");
    const auto& sam = account_service.get_account("sam");

    BOOST_TEST_MESSAGE("Setup vesting withdraw");
    withdraw_vesting_operation wv;
    wv.account = "alice";
    wv.vesting_shares = withdraw_amount;

    signed_transaction tx;
    tx.set_expiration(db.head_block_time() + SCORUM_MAX_TIME_UNTIL_EXPIRATION);
    tx.operations.push_back(wv);
    tx.sign(sign_key, db.get_chain_id());
    db.push_transaction(tx, 0);

    tx.operations.clear();
    tx.signatures.clear();

    BOOST_TEST_MESSAGE("Setting up bob destination");
    set_withdraw_vesting_route_to_account_operation op;
    op.from_account = "alice";
    op.to_account = "bob";
    op.percent = SCORUM_1_PERCENT * 50;
    op.auto_vest = true;
    tx.operations.push_back(op);

    BOOST_TEST_MESSAGE("Setting up sam destination");
    op.to_account = "sam";
    op.percent = SCORUM_1_PERCENT * 30;
    op.auto_vest = false;
    tx.operations.push_back(op);
    tx.sign(sign_key, db.get_chain_id());
    db.push_transaction(tx, 0);

    BOOST_TEST_MESSAGE("Setting up first withdraw");

    auto alice_id = alice.id;
    const auto& alice_wvo = withdraw_vesting_service.get(alice_id);

    auto vesting_withdraw_rate = alice_wvo.vesting_withdraw_rate;
    auto old_alice_balance = alice.balance;
    auto old_alice_vesting = alice.vesting_shares;
    auto old_bob_balance = bob.balance;
    auto old_bob_vesting = bob.vesting_shares;
    auto old_sam_balance = sam.balance;
    auto old_sam_vesting = sam.vesting_shares;
    generate_blocks(alice_wvo.next_vesting_withdrawal, true);

    {
        const auto& alice = account_service.get_account("alice");
        const auto& bob = account_service.get_account("bob");
        const auto& sam = account_service.get_account("sam");

        BOOST_REQUIRE(alice.vesting_shares == old_alice_vesting - vesting_withdraw_rate);
        BOOST_REQUIRE(
            alice.balance
            == old_alice_balance
                + asset((vesting_withdraw_rate.amount * SCORUM_1_PERCENT * 20) / SCORUM_100_PERCENT, SCORUM_SYMBOL));
        BOOST_REQUIRE(
            bob.vesting_shares
            == old_bob_vesting
                + asset((vesting_withdraw_rate.amount * SCORUM_1_PERCENT * 50) / SCORUM_100_PERCENT, VESTS_SYMBOL));
        BOOST_REQUIRE(bob.balance == old_bob_balance);
        BOOST_REQUIRE(sam.vesting_shares == old_sam_vesting);
        BOOST_REQUIRE(
            sam.balance
            == old_sam_balance
                + asset((vesting_withdraw_rate.amount * SCORUM_1_PERCENT * 30) / SCORUM_100_PERCENT, SCORUM_SYMBOL));

        old_alice_balance = alice.balance;
        old_alice_vesting = alice.vesting_shares;
        old_bob_balance = bob.balance;
        old_bob_vesting = bob.vesting_shares;
        old_sam_balance = sam.balance;
        old_sam_vesting = sam.vesting_shares;
    }

    BOOST_TEST_MESSAGE("Test failure with greater than 100% destination assignment");

    tx.operations.clear();
    tx.signatures.clear();

    op.to_account = "sam";
    op.percent = SCORUM_1_PERCENT * 50 + 1;
    tx.operations.push_back(op);
    tx.set_expiration(db.head_block_time() + SCORUM_MAX_TIME_UNTIL_EXPIRATION);
    tx.sign(sign_key, db.get_chain_id());
    SCORUM_REQUIRE_THROW(db.push_transaction(tx, 0), fc::exception);

    BOOST_TEST_MESSAGE("Test from_account receiving no withdraw");

    tx.operations.clear();
    tx.signatures.clear();

    op.to_account = "sam";
    op.percent = SCORUM_1_PERCENT * 50;
    tx.operations.push_back(op);
    tx.sign(sign_key, db.get_chain_id());
    db.push_transaction(tx, 0);

    generate_blocks(withdraw_vesting_service.get(alice_id).next_vesting_withdrawal, true);
    {
        const auto& alice = account_service.get_account("alice");
        const auto& bob = account_service.get_account("bob");
        const auto& sam = account_service.get_account("sam");

        BOOST_REQUIRE(alice.vesting_shares == old_alice_vesting - vesting_withdraw_rate);
        BOOST_REQUIRE(alice.balance == old_alice_balance);
        BOOST_REQUIRE(
            bob.vesting_shares
            == old_bob_vesting
                + asset((vesting_withdraw_rate.amount * SCORUM_1_PERCENT * 50) / SCORUM_100_PERCENT, VESTS_SYMBOL));
        BOOST_REQUIRE(bob.balance == old_bob_balance);
        BOOST_REQUIRE(sam.vesting_shares == old_sam_vesting);
        BOOST_REQUIRE(
            sam.balance
            == old_sam_balance
                + asset((vesting_withdraw_rate.amount * SCORUM_1_PERCENT * 50) / SCORUM_100_PERCENT, SCORUM_SYMBOL));
    }
}

BOOST_AUTO_TEST_SUITE_END()
