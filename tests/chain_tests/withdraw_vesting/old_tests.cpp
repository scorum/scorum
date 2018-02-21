#include <boost/test/unit_test.hpp>

#include "database_default_integration.hpp"

#include <scorum/chain/services/account.hpp>
#include <scorum/chain/services/withdraw_vesting_route.hpp>
#include <scorum/chain/services/withdraw_vesting_route_statistic.hpp>
#include <scorum/chain/services/withdraw_vesting.hpp>

#include <scorum/chain/schema/account_objects.hpp>
#include <scorum/chain/schema/withdraw_vesting_objects.hpp>

using namespace scorum::chain;
using namespace scorum::protocol;

#ifdef TODO

BOOST_AUTO_TEST_SUITE(withdraw_vesting_tests)

BOOST_FIXTURE_TEST_CASE(withdraw_vesting_apply, database_default_integration_fixture)
{
    try
    {
        BOOST_TEST_MESSAGE("Testing: withdraw_vesting_apply");

        ACTORS((alice))
        generate_block();
        vest("alice", ASSET_SCR(10e+3));

        generate_block();
        validate_database();
        BOOST_TEST_MESSAGE("--- Test withdraw of existing SP");

        {
            const auto& alice = db.obtain_service<dbs_account>().get_account("alice");

            withdraw_vesting_operation op;
            op.account = "alice";
            op.vesting_shares = asset(alice.vesting_shares.amount / 2, VESTS_SYMBOL);

            auto old_vesting_shares = alice.vesting_shares;

            signed_transaction tx;
            tx.operations.push_back(op);
            tx.set_expiration(db.head_block_time() + SCORUM_MAX_TIME_UNTIL_EXPIRATION);
            tx.sign(alice_private_key, db.get_chain_id());
            db.push_transaction(tx, 0);

            BOOST_REQUIRE(alice.vesting_shares.amount.value == old_vesting_shares.amount.value);
            BOOST_REQUIRE(alice.vesting_withdraw_rate.amount.value
                          == (old_vesting_shares.amount / (SCORUM_VESTING_WITHDRAW_INTERVALS * 2)).value);
            BOOST_REQUIRE(alice.to_withdraw == op.vesting_shares);
            BOOST_REQUIRE(alice.next_vesting_withdrawal
                          == db.head_block_time() + SCORUM_VESTING_WITHDRAW_INTERVAL_SECONDS);
            validate_database();

            BOOST_TEST_MESSAGE("--- Test changing vesting withdrawal");
            tx.operations.clear();
            tx.signatures.clear();

            op.vesting_shares = asset(alice.vesting_shares.amount / 3, VESTS_SYMBOL);
            tx.operations.push_back(op);
            tx.set_expiration(db.head_block_time() + SCORUM_MAX_TIME_UNTIL_EXPIRATION);
            tx.sign(alice_private_key, db.get_chain_id());
            db.push_transaction(tx, 0);

            BOOST_REQUIRE(alice.vesting_shares.amount.value == old_vesting_shares.amount.value);
            BOOST_REQUIRE(alice.vesting_withdraw_rate.amount.value
                          == (old_vesting_shares.amount / (SCORUM_VESTING_WITHDRAW_INTERVALS * 3)).value);
            BOOST_REQUIRE(alice.to_withdraw == op.vesting_shares);
            BOOST_REQUIRE(alice.next_vesting_withdrawal
                          == db.head_block_time() + SCORUM_VESTING_WITHDRAW_INTERVAL_SECONDS);
            validate_database();

            BOOST_TEST_MESSAGE("--- Test withdrawing more vests than available");

            tx.operations.clear();
            tx.signatures.clear();

            op.vesting_shares = asset(alice.vesting_shares.amount * 2, VESTS_SYMBOL);
            tx.operations.push_back(op);
            tx.set_expiration(db.head_block_time() + SCORUM_MAX_TIME_UNTIL_EXPIRATION);
            tx.sign(alice_private_key, db.get_chain_id());
            SCORUM_REQUIRE_THROW(db.push_transaction(tx, 0), fc::exception);

            BOOST_REQUIRE(alice.vesting_shares.amount.value == old_vesting_shares.amount.value);
            BOOST_REQUIRE(alice.vesting_withdraw_rate.amount.value
                          == (old_vesting_shares.amount / (SCORUM_VESTING_WITHDRAW_INTERVALS * 3)).value);
            BOOST_REQUIRE(alice.next_vesting_withdrawal
                          == db.head_block_time() + SCORUM_VESTING_WITHDRAW_INTERVAL_SECONDS);
            validate_database();

            BOOST_TEST_MESSAGE("--- Test withdrawing 0 to reset vesting withdraw");
            tx.operations.clear();
            tx.signatures.clear();

            op.vesting_shares = asset(0, VESTS_SYMBOL);
            tx.operations.push_back(op);
            tx.set_expiration(db.head_block_time() + SCORUM_MAX_TIME_UNTIL_EXPIRATION);
            tx.sign(alice_private_key, db.get_chain_id());
            db.push_transaction(tx, 0);

            BOOST_REQUIRE(alice.vesting_shares.amount.value == old_vesting_shares.amount.value);
            BOOST_REQUIRE(alice.vesting_withdraw_rate.amount.value == 0);
            BOOST_REQUIRE(alice.to_withdraw == asset(0, VESTS_SYMBOL));
            BOOST_REQUIRE(alice.next_vesting_withdrawal == fc::time_point_sec::maximum());

            BOOST_TEST_MESSAGE("--- Test cancelling a withdraw when below the account creation fee");
            op.vesting_shares = alice.vesting_shares;
            tx.clear();
            tx.operations.push_back(op);
            tx.sign(alice_private_key, db.get_chain_id());
            db.push_transaction(tx, 0);
            generate_block();
        }

        withdraw_vesting_operation op;
        signed_transaction tx;
        op.account = "alice";
        op.vesting_shares = ASSET_SP(0);
        tx.operations.push_back(op);
        tx.set_expiration(db.head_block_time() + SCORUM_MAX_TIME_UNTIL_EXPIRATION);
        tx.sign(alice_private_key, db.get_chain_id());
        db.push_transaction(tx, 0);

        BOOST_REQUIRE(db.obtain_service<dbs_account>().get_account("alice").vesting_withdraw_rate == ASSET_SP(0));
        validate_database();
    }
    FC_LOG_AND_RETHROW()
}

BOOST_FIXTURE_TEST_CASE(vesting_withdrawals, database_default_integration_fixture)
{
    try
    {
        ACTORS((alice))
        fund("alice", ASSET_SCR(100e+3));
        vest("alice", ASSET_SCR(100e+3));

        const auto& new_alice = db.obtain_service<dbs_account>().get_account("alice");

        BOOST_TEST_MESSAGE("Setting up withdrawal");

        signed_transaction tx;
        withdraw_vesting_operation op;
        op.account = "alice";
        op.vesting_shares = asset(new_alice.vesting_shares.amount / 2, VESTS_SYMBOL);
        tx.set_expiration(db.head_block_time() + SCORUM_MAX_TIME_UNTIL_EXPIRATION);
        tx.operations.push_back(op);
        tx.sign(alice_private_key, db.get_chain_id());
        db.push_transaction(tx, 0);

        auto next_withdrawal = db.head_block_time() + SCORUM_VESTING_WITHDRAW_INTERVAL_SECONDS;
        asset vesting_shares = new_alice.vesting_shares;
        asset to_withdraw = op.vesting_shares;
        asset original_vesting = vesting_shares;
        asset withdraw_rate = new_alice.vesting_withdraw_rate;

        BOOST_TEST_MESSAGE("Generating block up to first withdrawal");
        generate_blocks(next_withdrawal - (SCORUM_BLOCK_INTERVAL / 2), true);

        BOOST_REQUIRE(db.obtain_service<dbs_account>().get_account("alice").vesting_shares.amount.value
                      == vesting_shares.amount.value);

        BOOST_TEST_MESSAGE("Generating block to cause withdrawal");
        generate_block();

        auto fill_op = get_last_operations(1)[0].get<fill_vesting_withdraw_operation>();
        auto gpo = db.get_dynamic_global_properties();

        BOOST_REQUIRE(db.obtain_service<dbs_account>().get_account("alice").vesting_shares.amount.value
                      == (vesting_shares - withdraw_rate).amount.value);
        BOOST_REQUIRE_LE(
            (ASSET_SCR(withdraw_rate.amount.value) - db.obtain_service<dbs_account>().get_account("alice").balance)
                .amount.value,
            (share_value_type)1);
        BOOST_REQUIRE(fill_op.from_account == "alice");
        BOOST_REQUIRE(fill_op.to_account == "alice");
        BOOST_REQUIRE(fill_op.withdrawn.amount.value == withdraw_rate.amount.value);
        validate_database();

        BOOST_TEST_MESSAGE("Generating the rest of the blocks in the withdrawal");

        vesting_shares = db.obtain_service<dbs_account>().get_account("alice").vesting_shares;
        auto balance = db.obtain_service<dbs_account>().get_account("alice").balance;
        auto old_next_vesting = db.obtain_service<dbs_account>().get_account("alice").next_vesting_withdrawal;

        for (int i = 1; i < SCORUM_VESTING_WITHDRAW_INTERVALS - 1; i++)
        {
            generate_blocks(db.head_block_time() + SCORUM_VESTING_WITHDRAW_INTERVAL_SECONDS);

            const auto& alice = db.obtain_service<dbs_account>().get_account("alice");

            gpo = db.get_dynamic_global_properties();
            fill_op = get_last_operations(1)[0].get<fill_vesting_withdraw_operation>();

            BOOST_REQUIRE(alice.vesting_shares.amount.value == (vesting_shares - withdraw_rate).amount.value);
            BOOST_REQUIRE_LE(balance.amount.value
                                 + (ASSET_SCR(withdraw_rate.amount.value) - alice.balance).amount.value,
                             (share_value_type)1);
            BOOST_REQUIRE(fill_op.from_account == "alice");
            BOOST_REQUIRE(fill_op.to_account == "alice");
            BOOST_REQUIRE(fill_op.withdrawn.amount.value == withdraw_rate.amount.value);

            if (i == SCORUM_VESTING_WITHDRAW_INTERVALS - 1)
                BOOST_REQUIRE(alice.next_vesting_withdrawal == fc::time_point_sec::maximum());
            else
                BOOST_REQUIRE(alice.next_vesting_withdrawal.sec_since_epoch()
                              == (old_next_vesting + SCORUM_VESTING_WITHDRAW_INTERVAL_SECONDS).sec_since_epoch());

            validate_database();

            vesting_shares = alice.vesting_shares;
            balance = alice.balance;
            old_next_vesting = alice.next_vesting_withdrawal;
        }

        if (to_withdraw.amount.value % withdraw_rate.amount.value != 0)
        {
            BOOST_TEST_MESSAGE("Generating one more block to take care of remainder");
            generate_blocks(db.head_block_time() + SCORUM_VESTING_WITHDRAW_INTERVAL_SECONDS, true);
            fill_op = get_last_operations(1)[0].get<fill_vesting_withdraw_operation>();
            gpo = db.get_dynamic_global_properties();

            BOOST_REQUIRE(
                db.obtain_service<dbs_account>().get_account("alice").next_vesting_withdrawal.sec_since_epoch()
                == (old_next_vesting + SCORUM_VESTING_WITHDRAW_INTERVAL_SECONDS).sec_since_epoch());
            BOOST_REQUIRE(fill_op.from_account == "alice");
            BOOST_REQUIRE(fill_op.to_account == "alice");
            BOOST_REQUIRE(fill_op.withdrawn.amount.value == withdraw_rate.amount.value);

            generate_blocks(db.head_block_time() + SCORUM_VESTING_WITHDRAW_INTERVAL_SECONDS, true);
            gpo = db.get_dynamic_global_properties();
            fill_op = get_last_operations(1)[0].get<fill_vesting_withdraw_operation>();

            BOOST_REQUIRE(
                db.obtain_service<dbs_account>().get_account("alice").next_vesting_withdrawal.sec_since_epoch()
                == fc::time_point_sec::maximum().sec_since_epoch());
            BOOST_REQUIRE(fill_op.to_account == "alice");
            BOOST_REQUIRE(fill_op.from_account == "alice");
            BOOST_REQUIRE(fill_op.withdrawn.amount.value == to_withdraw.amount.value % withdraw_rate.amount.value);

            validate_database();
        }
        else
        {
            generate_blocks(db.head_block_time() + SCORUM_VESTING_WITHDRAW_INTERVAL_SECONDS, true);

            BOOST_REQUIRE(
                db.obtain_service<dbs_account>().get_account("alice").next_vesting_withdrawal.sec_since_epoch()
                == fc::time_point_sec::maximum().sec_since_epoch());

            fill_op = get_last_operations(1)[0].get<fill_vesting_withdraw_operation>();
            BOOST_REQUIRE(fill_op.from_account == "alice");
            BOOST_REQUIRE(fill_op.to_account == "alice");
            BOOST_REQUIRE(fill_op.withdrawn.amount.value == withdraw_rate.amount.value);
        }

        BOOST_REQUIRE(db.obtain_service<dbs_account>().get_account("alice").vesting_shares.amount.value
                      == (original_vesting - op.vesting_shares).amount.value);
    }
    FC_LOG_AND_RETHROW()
}

BOOST_FIXTURE_TEST_CASE(vesting_withdraw_route, database_default_integration_fixture)
{
    try
    {
        ACTORS((alice)(bob)(sam))

        auto original_vesting = alice.vesting_shares;

        fund("alice", ASSET_SCR(104e+4));
        vest("alice", ASSET_SCR(104e+4));

        auto withdraw_amount = alice.vesting_shares - original_vesting;

        BOOST_TEST_MESSAGE("Setup vesting withdraw");
        withdraw_vesting_operation wv;
        wv.account = "alice";
        wv.vesting_shares = withdraw_amount;

        signed_transaction tx;
        tx.set_expiration(db.head_block_time() + SCORUM_MAX_TIME_UNTIL_EXPIRATION);
        tx.operations.push_back(wv);
        tx.sign(alice_private_key, db.get_chain_id());
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
        tx.sign(alice_private_key, db.get_chain_id());
        db.push_transaction(tx, 0);

        BOOST_TEST_MESSAGE("Setting up first withdraw");

        auto vesting_withdraw_rate = alice.vesting_withdraw_rate;
        auto old_alice_balance = alice.balance;
        auto old_alice_vesting = alice.vesting_shares;
        auto old_bob_balance = bob.balance;
        auto old_bob_vesting = bob.vesting_shares;
        auto old_sam_balance = sam.balance;
        auto old_sam_vesting = sam.vesting_shares;
        generate_blocks(alice.next_vesting_withdrawal, true);

        {
            const auto& alice = db.obtain_service<dbs_account>().get_account("alice");
            const auto& bob = db.obtain_service<dbs_account>().get_account("bob");
            const auto& sam = db.obtain_service<dbs_account>().get_account("sam");

            BOOST_REQUIRE(alice.vesting_shares == old_alice_vesting - vesting_withdraw_rate);
            BOOST_REQUIRE(alice.balance
                          == old_alice_balance
                              + asset((vesting_withdraw_rate.amount * SCORUM_1_PERCENT * 20) / SCORUM_100_PERCENT,
                                      SCORUM_SYMBOL));
            BOOST_REQUIRE(
                bob.vesting_shares
                == old_bob_vesting
                    + asset((vesting_withdraw_rate.amount * SCORUM_1_PERCENT * 50) / SCORUM_100_PERCENT, VESTS_SYMBOL));
            BOOST_REQUIRE(bob.balance == old_bob_balance);
            BOOST_REQUIRE(sam.vesting_shares == old_sam_vesting);
            BOOST_REQUIRE(sam.balance
                          == old_sam_balance
                              + asset((vesting_withdraw_rate.amount * SCORUM_1_PERCENT * 30) / SCORUM_100_PERCENT,
                                      SCORUM_SYMBOL));

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
        tx.sign(alice_private_key, db.get_chain_id());
        SCORUM_REQUIRE_THROW(db.push_transaction(tx, 0), fc::exception);

        BOOST_TEST_MESSAGE("Test from_account receiving no withdraw");

        tx.operations.clear();
        tx.signatures.clear();

        op.to_account = "sam";
        op.percent = SCORUM_1_PERCENT * 50;
        tx.operations.push_back(op);
        tx.sign(alice_private_key, db.get_chain_id());
        db.push_transaction(tx, 0);

        generate_blocks(db.obtain_service<dbs_account>().get_account("alice").next_vesting_withdrawal, true);
        {
            const auto& alice = db.obtain_service<dbs_account>().get_account("alice");
            const auto& bob = db.obtain_service<dbs_account>().get_account("bob");
            const auto& sam = db.obtain_service<dbs_account>().get_account("sam");

            BOOST_REQUIRE(alice.vesting_shares == old_alice_vesting - vesting_withdraw_rate);
            BOOST_REQUIRE(alice.balance == old_alice_balance);
            BOOST_REQUIRE(
                bob.vesting_shares
                == old_bob_vesting
                    + asset((vesting_withdraw_rate.amount * SCORUM_1_PERCENT * 50) / SCORUM_100_PERCENT, VESTS_SYMBOL));
            BOOST_REQUIRE(bob.balance == old_bob_balance);
            BOOST_REQUIRE(sam.vesting_shares == old_sam_vesting);
            BOOST_REQUIRE(sam.balance
                          == old_sam_balance
                              + asset((vesting_withdraw_rate.amount * SCORUM_1_PERCENT * 50) / SCORUM_100_PERCENT,
                                      SCORUM_SYMBOL));
        }
    }
    FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_SUITE_END()

#endif // TODO
