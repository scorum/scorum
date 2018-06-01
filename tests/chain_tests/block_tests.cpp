/*
 * Copyright (c) 2015 Cryptonomex, Inc., and contributors.
 *
 * The MIT License
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */
#include <boost/test/unit_test.hpp>

#include <scorum/protocol/exceptions.hpp>

#include <scorum/chain/database/database.hpp>
#include <scorum/chain/schema/scorum_objects.hpp>
#include <scorum/blockchain_history/schema/operation_objects.hpp>
#include <scorum/chain/genesis/genesis_state.hpp>
#include <scorum/chain/services/account.hpp>
#include <scorum/chain/services/dynamic_global_property.hpp>

#include <scorum/blockchain_history/blockchain_history_plugin.hpp>

#include <graphene/utilities/tempdir.hpp>

#include <fc/crypto/digest.hpp>

#include "database_default_integration.hpp"
#include "database_integration.hpp"

namespace {

using namespace scorum::chain;
using namespace scorum::protocol;

bool test_push_block(database& db, const signed_block& b, uint32_t skip_flags = 0)
{
    return db.push_block(b, skip_flags);
}

void test_push_transaction(database& db, const signed_transaction& tx, uint32_t skip_flags = 0)
{
    try
    {
        db.push_transaction(tx, skip_flags);
    }
    FC_CAPTURE_AND_RETHROW((tx))
}

} // namespace test

#define PUSH_TX test_push_transaction

#define PUSH_BLOCK test_push_block

using namespace database_fixture;

BOOST_AUTO_TEST_SUITE(block_tests)

void db_setup_and_open(database& db, const fc::path& path)
{
    genesis_state_type genesis;

    genesis = database_integration_fixture::create_default_genesis_state();

    db.open(path, path, TEST_SHARED_MEM_SIZE_10MB, chainbase::database::read_write, genesis);
}

BOOST_AUTO_TEST_CASE(generate_empty_blocks)
{
    try
    {
        fc::time_point_sec now(TEST_GENESIS_TIMESTAMP);
        fc::temp_directory data_dir(graphene::utilities::temp_directory_path());
        signed_block b;

        // TODO:  Don't generate this here
        auto init_account_priv_key = fc::ecc::private_key::regenerate(fc::sha256::hash(std::string(TEST_INIT_KEY)));
        signed_block cutoff_block;
        {
            database db(database::opt_default);
            db_setup_and_open(db, data_dir.path());
            b = db.generate_block(db.get_slot_time(1), db.get_scheduled_witness(1), init_account_priv_key,
                                  database::skip_nothing);

            // TODO:  Change this test when we correct #406
            // n.b. we generate SCORUM_MIN_UNDO_HISTORY+1 extra blocks which will be discarded on save
            for (uint32_t i = 1;; ++i)
            {
                BOOST_CHECK(db.head_block_id() == b.id());
                // witness_id_type prev_witness = b.witness;
                std::string cur_witness = db.get_scheduled_witness(1);
                // BOOST_CHECK( cur_witness != prev_witness );
                b = db.generate_block(db.get_slot_time(1), cur_witness, init_account_priv_key, database::skip_nothing);
                BOOST_CHECK(b.witness == cur_witness);
                uint32_t cutoff_height
                    = db.obtain_service<dbs_dynamic_global_property>().get().last_irreversible_block_num;
                if (cutoff_height >= 200)
                {
                    auto block = db.fetch_block_by_number(cutoff_height);
                    BOOST_REQUIRE(block.valid());
                    cutoff_block = *block;
                    break;
                }
            }
            db.close();
        }
        {
            database db(database::opt_default);
            db_setup_and_open(db, data_dir.path());
            BOOST_CHECK_EQUAL(db.head_block_num(), cutoff_block.block_num());
            b = cutoff_block;
            for (uint32_t i = 0; i < 200; ++i)
            {
                BOOST_CHECK(db.head_block_id() == b.id());
                // witness_id_type prev_witness = b.witness;
                std::string cur_witness = db.get_scheduled_witness(1);
                // BOOST_CHECK( cur_witness != prev_witness );
                b = db.generate_block(db.get_slot_time(1), cur_witness, init_account_priv_key, database::skip_nothing);
            }
            BOOST_CHECK_EQUAL(db.head_block_num(), cutoff_block.block_num() + 200);
        }
    }
    catch (fc::exception& e)
    {
        edump((e.to_detail_string()));
        throw;
    }
}

BOOST_AUTO_TEST_CASE(undo_block)
{
    try
    {
        fc::temp_directory data_dir(graphene::utilities::temp_directory_path());
        {
            database db(database::opt_default);
            db_setup_and_open(db, data_dir.path());
            fc::time_point_sec now(TEST_GENESIS_TIMESTAMP);
            std::vector<fc::time_point_sec> time_stack;

            auto init_account_priv_key = fc::ecc::private_key::regenerate(fc::sha256::hash(std::string(TEST_INIT_KEY)));
            for (uint32_t i = 0; i < 5; ++i)
            {
                now = db.get_slot_time(1);
                time_stack.push_back(now);
                auto b = db.generate_block(now, db.get_scheduled_witness(1), init_account_priv_key,
                                           database::skip_nothing);
            }
            BOOST_CHECK(db.head_block_num() == 5);
            BOOST_CHECK(db.head_block_time() == now);
            db.pop_block();
            time_stack.pop_back();
            now = time_stack.back();
            BOOST_CHECK(db.head_block_num() == 4);
            BOOST_CHECK(db.head_block_time() == now);
            db.pop_block();
            time_stack.pop_back();
            now = time_stack.back();
            BOOST_CHECK(db.head_block_num() == 3);
            BOOST_CHECK(db.head_block_time() == now);
            db.pop_block();
            time_stack.pop_back();
            now = time_stack.back();
            BOOST_CHECK(db.head_block_num() == 2);
            BOOST_CHECK(db.head_block_time() == now);
            for (uint32_t i = 0; i < 5; ++i)
            {
                now = db.get_slot_time(1);
                time_stack.push_back(now);
                auto b = db.generate_block(now, db.get_scheduled_witness(1), init_account_priv_key,
                                           database::skip_nothing);
            }
            BOOST_CHECK(db.head_block_num() == 7);
        }
    }
    catch (fc::exception& e)
    {
        edump((e.to_detail_string()));
        throw;
    }
}

BOOST_AUTO_TEST_CASE(fork_blocks)
{
    try
    {
        fc::temp_directory data_dir1(graphene::utilities::temp_directory_path());
        fc::temp_directory data_dir2(graphene::utilities::temp_directory_path());

        // TODO This test needs 6-7 ish witnesses prior to fork

        database db1(database::opt_default);
        db_setup_and_open(db1, data_dir1.path());
        database db2(database::opt_default);
        db_setup_and_open(db2, data_dir2.path());

        auto init_account_priv_key = fc::ecc::private_key::regenerate(fc::sha256::hash(std::string(TEST_INIT_KEY)));
        for (uint32_t i = 0; i < 10; ++i)
        {
            auto b = db1.generate_block(db1.get_slot_time(1), db1.get_scheduled_witness(1), init_account_priv_key,
                                        database::skip_nothing);
            try
            {
                PUSH_BLOCK(db2, b);
            }
            FC_CAPTURE_AND_RETHROW(("db2"));
        }
        for (uint32_t i = 10; i < 13; ++i)
        {
            auto b = db1.generate_block(db1.get_slot_time(1), db1.get_scheduled_witness(1), init_account_priv_key,
                                        database::skip_nothing);
        }
        std::string db1_tip = db1.head_block_id().str();
        uint32_t next_slot = 3;
        for (uint32_t i = 13; i < 16; ++i)
        {
            auto b = db2.generate_block(db2.get_slot_time(next_slot), db2.get_scheduled_witness(next_slot),
                                        init_account_priv_key, database::skip_nothing);
            next_slot = 1;
            // notify both databases of the new block.
            // only db2 should switch to the new fork, db1 should not
            PUSH_BLOCK(db1, b);
            BOOST_CHECK_EQUAL(db1.head_block_id().str(), db1_tip);
            BOOST_CHECK_EQUAL(db2.head_block_id().str(), b.id().str());
        }

        // The two databases are on distinct forks now, but at the same height. Make a block on db2, make it invalid,
        // then
        // pass it to db1 and assert that db1 doesn't switch to the new fork.
        signed_block good_block;
        BOOST_CHECK_EQUAL(db1.head_block_num(), uint32_t(13));
        BOOST_CHECK_EQUAL(db2.head_block_num(), uint32_t(13));
        {
            auto b = db2.generate_block(db2.get_slot_time(1), db2.get_scheduled_witness(1), init_account_priv_key,
                                        database::skip_nothing);
            good_block = b;
            b.transactions.emplace_back(signed_transaction());
            b.transactions.back().operations.emplace_back(transfer_operation());
            b.sign(init_account_priv_key);
            BOOST_CHECK_EQUAL(b.block_num(), uint32_t(14));
            SCORUM_CHECK_THROW(PUSH_BLOCK(db1, b), fc::exception);
        }
        BOOST_CHECK_EQUAL(db1.head_block_num(), uint32_t(13));
        BOOST_CHECK_EQUAL(db1.head_block_id().str(), db1_tip);

        // assert that db1 switches to new fork with good block
        BOOST_CHECK_EQUAL(db2.head_block_num(), uint32_t(14));
        PUSH_BLOCK(db1, good_block);
        BOOST_CHECK_EQUAL(db1.head_block_id().str(), db2.head_block_id().str());
    }
    catch (fc::exception& e)
    {
        edump((e.to_detail_string()));
        throw;
    }
}

BOOST_AUTO_TEST_CASE(switch_forks_undo_create)
{
    try
    {
        fc::temp_directory dir1(graphene::utilities::temp_directory_path());
        fc::temp_directory dir2(graphene::utilities::temp_directory_path());

        database db1(database::opt_default);
        db_setup_and_open(db1, dir1.path());
        database db2(database::opt_default);
        db_setup_and_open(db2, dir2.path());

        auto init_account_priv_key = fc::ecc::private_key::regenerate(fc::sha256::hash(std::string(TEST_INIT_KEY)));
        public_key_type init_account_pub_key = init_account_priv_key.get_public_key();
        db1.get_index<account_index>();

        //*
        signed_transaction trx;
        account_create_operation cop;
        cop.new_account_name = "alice";
        cop.creator = TEST_INIT_DELEGATE_NAME;
        cop.owner = authority(1, init_account_pub_key, 1);
        cop.active = cop.owner;
        cop.fee = SUFFICIENT_FEE;
        trx.operations.push_back(cop);
        trx.set_expiration(db1.head_block_time() + SCORUM_MAX_TIME_UNTIL_EXPIRATION);
        trx.sign(init_account_priv_key, db1.get_chain_id());
        PUSH_TX(db1, trx);
        //*/
        // generate blocks
        // db1 : A
        // db2 : B C D

        auto b = db1.generate_block(db1.get_slot_time(1), db1.get_scheduled_witness(1), init_account_priv_key,
                                    database::skip_nothing);

        auto alice_id = db1.obtain_service<dbs_account>().get_account("alice").id;
        BOOST_CHECK(db1.get(alice_id).name == "alice");

        b = db2.generate_block(db2.get_slot_time(1), db2.get_scheduled_witness(1), init_account_priv_key,
                               database::skip_nothing);
        db1.push_block(b);
        b = db2.generate_block(db2.get_slot_time(1), db2.get_scheduled_witness(1), init_account_priv_key,
                               database::skip_nothing);
        db1.push_block(b);
        SCORUM_REQUIRE_THROW(db2.get(alice_id), std::exception);
        db1.get(alice_id); /// it should be included in the pending state
        db1.clear_pending(); // clear it so that we can verify it was properly removed from pending state.
        SCORUM_REQUIRE_THROW(db1.get(alice_id), std::exception);

        PUSH_TX(db2, trx);

        b = db2.generate_block(db2.get_slot_time(1), db2.get_scheduled_witness(1), init_account_priv_key,
                               database::skip_nothing);
        db1.push_block(b);

        BOOST_CHECK(db1.get(alice_id).name == "alice");
        BOOST_CHECK(db2.get(alice_id).name == "alice");
    }
    catch (fc::exception& e)
    {
        edump((e.to_detail_string()));
        throw;
    }
}

BOOST_AUTO_TEST_CASE(duplicate_transactions)
{
    try
    {
        fc::temp_directory dir1(graphene::utilities::temp_directory_path());
        fc::temp_directory dir2(graphene::utilities::temp_directory_path());

        database db1(database::opt_default);
        db_setup_and_open(db1, dir1.path());
        database db2(database::opt_default);
        db_setup_and_open(db2, dir2.path());

        BOOST_CHECK(db1.get_chain_id() == db2.get_chain_id());

        auto skip_sigs = database::skip_transaction_signatures | database::skip_authority_check;

        auto init_account_priv_key = fc::ecc::private_key::regenerate(fc::sha256::hash(std::string(TEST_INIT_KEY)));
        public_key_type init_account_pub_key = init_account_priv_key.get_public_key();

        signed_transaction trx;
        account_create_operation cop;
        cop.new_account_name = "alice";
        cop.creator = TEST_INIT_DELEGATE_NAME;
        cop.owner = authority(1, init_account_pub_key, 1);
        cop.fee = SUFFICIENT_FEE;
        cop.active = cop.owner;
        trx.operations.push_back(cop);
        trx.set_expiration(db1.head_block_time() + SCORUM_MAX_TIME_UNTIL_EXPIRATION);
        trx.sign(init_account_priv_key, db1.get_chain_id());
        PUSH_TX(db1, trx, skip_sigs);

        trx = decltype(trx)();
        transfer_operation t;
        t.from = TEST_INIT_DELEGATE_NAME;
        t.to = "alice";
        t.amount = asset(500, SCORUM_SYMBOL);
        trx.operations.push_back(t);
        trx.set_expiration(db1.head_block_time() + SCORUM_MAX_TIME_UNTIL_EXPIRATION);
        trx.sign(init_account_priv_key, db1.get_chain_id());
        PUSH_TX(db1, trx, skip_sigs);

        SCORUM_CHECK_THROW(PUSH_TX(db1, trx, skip_sigs), fc::exception);

        auto b
            = db1.generate_block(db1.get_slot_time(1), db1.get_scheduled_witness(1), init_account_priv_key, skip_sigs);
        PUSH_BLOCK(db2, b, skip_sigs);

        SCORUM_CHECK_THROW(PUSH_TX(db1, trx, skip_sigs), fc::exception);
        SCORUM_CHECK_THROW(PUSH_TX(db2, trx, skip_sigs), fc::exception);
        BOOST_CHECK_EQUAL(db1.obtain_service<dbs_account>().get_account("alice").balance.amount, 500);
        BOOST_CHECK_EQUAL(db2.obtain_service<dbs_account>().get_account("alice").balance.amount, 500);
    }
    catch (fc::exception& e)
    {
        edump((e.to_detail_string()));
        throw;
    }
}

BOOST_AUTO_TEST_CASE(tapos)
{
    try
    {
        fc::temp_directory dir1(graphene::utilities::temp_directory_path());

        database db1(database::opt_default);
        db_setup_and_open(db1, dir1.path());

        auto init_account_priv_key = fc::ecc::private_key::regenerate(fc::sha256::hash(std::string(TEST_INIT_KEY)));
        public_key_type init_account_pub_key = init_account_priv_key.get_public_key();

        auto b = db1.generate_block(db1.get_slot_time(1), db1.get_scheduled_witness(1), init_account_priv_key,
                                    database::skip_nothing);

        BOOST_TEST_MESSAGE("Creating a transaction with reference block");
        idump((db1.head_block_id()));
        signed_transaction trx;
        // This transaction must be in the next block after its reference, or it is invalid.
        trx.set_reference_block(db1.head_block_id());

        account_create_operation cop;
        cop.new_account_name = "alice";
        cop.creator = TEST_INIT_DELEGATE_NAME;
        cop.owner = authority(1, init_account_pub_key, 1);
        cop.active = cop.owner;
        cop.fee = SUFFICIENT_FEE;
        trx.operations.push_back(cop);
        trx.set_expiration(db1.head_block_time() + SCORUM_MAX_TIME_UNTIL_EXPIRATION);
        trx.sign(init_account_priv_key, db1.get_chain_id());

        BOOST_TEST_MESSAGE("Pushing Pending Transaction");
        idump((trx));
        db1.push_transaction(trx);
        BOOST_TEST_MESSAGE("Generating a block");
        b = db1.generate_block(db1.get_slot_time(1), db1.get_scheduled_witness(1), init_account_priv_key,
                               database::skip_nothing);
        trx.clear();

        transfer_operation t;
        t.from = TEST_INIT_DELEGATE_NAME;
        t.to = "alice";
        t.amount = asset(50, SCORUM_SYMBOL);
        trx.operations.push_back(t);
        trx.set_expiration(db1.head_block_time() + fc::seconds(2));
        trx.sign(init_account_priv_key, db1.get_chain_id());
        idump((trx)(db1.head_block_time()));
        b = db1.generate_block(db1.get_slot_time(1), db1.get_scheduled_witness(1), init_account_priv_key,
                               database::skip_nothing);
        idump((b));
        b = db1.generate_block(db1.get_slot_time(1), db1.get_scheduled_witness(1), init_account_priv_key,
                               database::skip_nothing);
        trx.signatures.clear();
        trx.sign(init_account_priv_key, db1.get_chain_id());
        BOOST_REQUIRE_THROW(
            db1.push_transaction(trx, 0 /*database::skip_transaction_signatures | database::skip_authority_check*/),
            fc::exception);
    }
    catch (fc::exception& e)
    {
        edump((e.to_detail_string()));
        throw;
    }
}

BOOST_FIXTURE_TEST_CASE(optional_tapos, database_default_integration_fixture)
{
    try
    {
        idump((db.obtain_service<dbs_account>().get_account(TEST_INIT_DELEGATE_NAME)));
        ACTORS((alice)(bob));

        generate_block();

        BOOST_TEST_MESSAGE("Create transaction");

        transfer(TEST_INIT_DELEGATE_NAME, "alice", asset(1000000, SCORUM_SYMBOL));
        transfer_operation op;
        op.from = "alice";
        op.to = "bob";
        op.amount = asset(1000, SCORUM_SYMBOL);
        signed_transaction tx;
        tx.operations.push_back(op);

        BOOST_TEST_MESSAGE("ref_block_num=0, ref_block_prefix=0");

        tx.ref_block_num = 0;
        tx.ref_block_prefix = 0;
        tx.signatures.clear();
        tx.set_expiration(db.head_block_time() + SCORUM_MAX_TIME_UNTIL_EXPIRATION);
        tx.sign(alice_private_key, db.get_chain_id());
        PUSH_TX(db, tx);

        BOOST_TEST_MESSAGE("proper ref_block_num, ref_block_prefix");

        tx.signatures.clear();
        tx.set_expiration(db.head_block_time() + SCORUM_MAX_TIME_UNTIL_EXPIRATION);
        tx.sign(alice_private_key, db.get_chain_id());
        PUSH_TX(db, tx, database::skip_transaction_dupe_check);

        BOOST_TEST_MESSAGE("ref_block_num=0, ref_block_prefix=12345678");

        tx.ref_block_num = 0;
        tx.ref_block_prefix = 0x12345678;
        tx.signatures.clear();
        tx.set_expiration(db.head_block_time() + SCORUM_MAX_TIME_UNTIL_EXPIRATION);
        tx.sign(alice_private_key, db.get_chain_id());
        SCORUM_REQUIRE_THROW(PUSH_TX(db, tx, database::skip_transaction_dupe_check), fc::exception);

        BOOST_TEST_MESSAGE("ref_block_num=1, ref_block_prefix=12345678");

        tx.ref_block_num = 1;
        tx.ref_block_prefix = 0x12345678;
        tx.signatures.clear();
        tx.set_expiration(db.head_block_time() + SCORUM_MAX_TIME_UNTIL_EXPIRATION);
        tx.sign(alice_private_key, db.get_chain_id());
        SCORUM_REQUIRE_THROW(PUSH_TX(db, tx, database::skip_transaction_dupe_check), fc::exception);

        BOOST_TEST_MESSAGE("ref_block_num=9999, ref_block_prefix=12345678");

        tx.ref_block_num = 9999;
        tx.ref_block_prefix = 0x12345678;
        tx.signatures.clear();
        tx.set_expiration(db.head_block_time() + SCORUM_MAX_TIME_UNTIL_EXPIRATION);
        tx.sign(alice_private_key, db.get_chain_id());
        SCORUM_REQUIRE_THROW(PUSH_TX(db, tx, database::skip_transaction_dupe_check), fc::exception);
    }
    catch (fc::exception& e)
    {
        edump((e.to_detail_string()));
        throw;
    }
}

BOOST_FIXTURE_TEST_CASE(double_sign_check, database_default_integration_fixture)
{
    try
    {
        generate_block();
        ACTOR(bob);
        share_type amount = 1000;

        transfer_operation t;
        t.from = TEST_INIT_DELEGATE_NAME;
        t.to = "bob";
        t.amount = asset(amount, SCORUM_SYMBOL);
        trx.operations.push_back(t);
        trx.set_expiration(db.head_block_time() + SCORUM_MAX_TIME_UNTIL_EXPIRATION);
        trx.validate();

        db.push_transaction(trx, ~0);

        trx.operations.clear();
        t.from = "bob";
        t.to = TEST_INIT_DELEGATE_NAME;
        t.amount = asset(amount, SCORUM_SYMBOL);
        trx.operations.push_back(t);
        trx.validate();

        BOOST_TEST_MESSAGE("Verify that not-signing causes an exception");
        SCORUM_REQUIRE_THROW(db.push_transaction(trx, 0), fc::exception);

        BOOST_TEST_MESSAGE("Verify that double-signing causes an exception");
        trx.sign(bob_private_key, db.get_chain_id());
        trx.sign(bob_private_key, db.get_chain_id());
        SCORUM_REQUIRE_THROW(db.push_transaction(trx, 0), tx_duplicate_sig);

        BOOST_TEST_MESSAGE("Verify that signing with an extra, unused key fails");
        trx.signatures.pop_back();
        trx.sign(generate_private_key("bogus"), db.get_chain_id());
        SCORUM_REQUIRE_THROW(db.push_transaction(trx, 0), tx_irrelevant_sig);

        BOOST_TEST_MESSAGE("Verify that signing once with the proper key passes");
        trx.signatures.pop_back();
        db.push_transaction(trx, 0);
        trx.sign(bob_private_key, db.get_chain_id());
    }
    FC_LOG_AND_RETHROW()
}

BOOST_FIXTURE_TEST_CASE(pop_block_twice, database_default_integration_fixture)
{
    try
    {
        uint32_t skip_flags = (database::skip_witness_signature | database::skip_transaction_signatures
                               | database::skip_authority_check);

        // Sam is the creator of accounts
        auto init_account_priv_key = fc::ecc::private_key::regenerate(fc::sha256::hash(std::string(TEST_INIT_KEY)));
        private_key_type sam_key = generate_private_key("sam");
        account_object sam_account_object = account_create("sam", sam_key.get_public_key());

        // Get a sane head block time
        generate_block(skip_flags);

        transaction tx;
        signed_transaction ptx;

        db.obtain_service<dbs_account>().get_account(TEST_INIT_DELEGATE_NAME);
        // transfer from committee account to Sam account
        transfer(TEST_INIT_DELEGATE_NAME, "sam", asset(100000, SCORUM_SYMBOL));

        generate_block(skip_flags);

        account_create("alice", generate_private_key("alice").get_public_key());
        generate_block(skip_flags);
        account_create("bob", generate_private_key("bob").get_public_key());
        generate_block(skip_flags);

        db.pop_block();
        db.pop_block();
    }
    catch (const fc::exception& e)
    {
        edump((e.to_detail_string()));
        throw;
    }
}

BOOST_FIXTURE_TEST_CASE(rsf_missed_blocks, database_default_integration_fixture)
{
    try
    {
        generate_block();

        auto rsf = [&]() -> std::string {
            fc::uint128 rsf = db.obtain_service<dbs_dynamic_global_property>().get().recent_slots_filled;
            std::string result = "";
            result.reserve(128);
            for (int i = 0; i < 128; i++)
            {
                result += ((rsf.lo & 1) == 0) ? '0' : '1';
                rsf >>= 1;
            }
            return result;
        };

        auto pct = [](uint32_t x) -> uint32_t { return uint64_t(SCORUM_100_PERCENT) * x / 128; };

        BOOST_TEST_MESSAGE("checking initial participation rate");
        BOOST_CHECK_EQUAL(rsf(), "1111111111111111111111111111111111111111111111111111111111111111"
                                 "1111111111111111111111111111111111111111111111111111111111111111");
        BOOST_CHECK_EQUAL(db.witness_participation_rate(), uint32_t(SCORUM_100_PERCENT));

        BOOST_TEST_MESSAGE("Generating a block skipping 1");
        generate_block(~database::skip_fork_db, initdelegate.private_key, 1);
        BOOST_CHECK_EQUAL(rsf(), "0111111111111111111111111111111111111111111111111111111111111111"
                                 "1111111111111111111111111111111111111111111111111111111111111111");
        BOOST_CHECK_EQUAL(db.witness_participation_rate(), pct(127));

        BOOST_TEST_MESSAGE("Generating a block skipping 1");
        generate_block(~database::skip_fork_db, initdelegate.private_key, 1);
        BOOST_CHECK_EQUAL(rsf(), "0101111111111111111111111111111111111111111111111111111111111111"
                                 "1111111111111111111111111111111111111111111111111111111111111111");
        BOOST_CHECK_EQUAL(db.witness_participation_rate(), pct(126));

        BOOST_TEST_MESSAGE("Generating a block skipping 2");
        generate_block(~database::skip_fork_db, initdelegate.private_key, 2);
        BOOST_CHECK_EQUAL(rsf(), "0010101111111111111111111111111111111111111111111111111111111111"
                                 "1111111111111111111111111111111111111111111111111111111111111111");
        BOOST_CHECK_EQUAL(db.witness_participation_rate(), pct(124));

        BOOST_TEST_MESSAGE("Generating a block for skipping 3");
        generate_block(~database::skip_fork_db, initdelegate.private_key, 3);
        BOOST_CHECK_EQUAL(rsf(), "0001001010111111111111111111111111111111111111111111111111111111"
                                 "1111111111111111111111111111111111111111111111111111111111111111");
        BOOST_CHECK_EQUAL(db.witness_participation_rate(), pct(121));

        BOOST_TEST_MESSAGE("Generating a block skipping 5");
        generate_block(~database::skip_fork_db, initdelegate.private_key, 5);
        BOOST_CHECK_EQUAL(rsf(), "0000010001001010111111111111111111111111111111111111111111111111"
                                 "1111111111111111111111111111111111111111111111111111111111111111");
        BOOST_CHECK_EQUAL(db.witness_participation_rate(), pct(116));

        BOOST_TEST_MESSAGE("Generating a block skipping 8");
        generate_block(~database::skip_fork_db, initdelegate.private_key, 8);
        BOOST_CHECK_EQUAL(rsf(), "0000000010000010001001010111111111111111111111111111111111111111"
                                 "1111111111111111111111111111111111111111111111111111111111111111");
        BOOST_CHECK_EQUAL(db.witness_participation_rate(), pct(108));

        BOOST_TEST_MESSAGE("Generating a block skipping 13");
        generate_block(~database::skip_fork_db, initdelegate.private_key, 13);
        BOOST_CHECK_EQUAL(rsf(), "0000000000000100000000100000100010010101111111111111111111111111"
                                 "1111111111111111111111111111111111111111111111111111111111111111");
        BOOST_CHECK_EQUAL(db.witness_participation_rate(), pct(95));

        BOOST_TEST_MESSAGE("Generating a block skipping none");
        generate_block();
        BOOST_CHECK_EQUAL(rsf(), "1000000000000010000000010000010001001010111111111111111111111111"
                                 "1111111111111111111111111111111111111111111111111111111111111111");
        BOOST_CHECK_EQUAL(db.witness_participation_rate(), pct(95));

        BOOST_TEST_MESSAGE("Generating a block");
        generate_block();
        BOOST_CHECK_EQUAL(rsf(), "1100000000000001000000001000001000100101011111111111111111111111"
                                 "1111111111111111111111111111111111111111111111111111111111111111");
        BOOST_CHECK_EQUAL(db.witness_participation_rate(), pct(95));

        generate_block();
        BOOST_CHECK_EQUAL(rsf(), "1110000000000000100000000100000100010010101111111111111111111111"
                                 "1111111111111111111111111111111111111111111111111111111111111111");
        BOOST_CHECK_EQUAL(db.witness_participation_rate(), pct(95));

        generate_block();
        BOOST_CHECK_EQUAL(rsf(), "1111000000000000010000000010000010001001010111111111111111111111"
                                 "1111111111111111111111111111111111111111111111111111111111111111");
        BOOST_CHECK_EQUAL(db.witness_participation_rate(), pct(95));

        generate_block(~database::skip_fork_db, initdelegate.private_key, 64);
        BOOST_CHECK_EQUAL(rsf(), "0000000000000000000000000000000000000000000000000000000000000000"
                                 "1111100000000000001000000001000001000100101011111111111111111111");
        BOOST_CHECK_EQUAL(db.witness_participation_rate(), pct(31));

        generate_block(~database::skip_fork_db, initdelegate.private_key, 32);
        BOOST_CHECK_EQUAL(rsf(), "0000000000000000000000000000000010000000000000000000000000000000"
                                 "0000000000000000000000000000000001111100000000000001000000001000");
        BOOST_CHECK_EQUAL(db.witness_participation_rate(), pct(8));
    }
    FC_LOG_AND_RETHROW()
}

BOOST_FIXTURE_TEST_CASE(skip_block, database_default_integration_fixture)
{
    try
    {
        BOOST_TEST_MESSAGE("Skipping blocks through db");

        int init_block_num = db.head_block_num();
        int miss_blocks = fc::minutes(1).to_seconds() / SCORUM_BLOCK_INTERVAL;
        auto witness = db.get_scheduled_witness(miss_blocks);
        auto block_time = db.get_slot_time(miss_blocks);
        db.generate_block(block_time, witness, initdelegate.private_key, 0);

        BOOST_CHECK_EQUAL(db.head_block_num(), uint32_t(init_block_num + 1));
        BOOST_CHECK(db.head_block_time() == block_time);

        BOOST_TEST_MESSAGE("Generating a block through fixture");
        generate_block();

        BOOST_CHECK_EQUAL(db.head_block_num(), uint32_t(init_block_num + 2));
        BOOST_CHECK(db.head_block_time() == block_time + SCORUM_BLOCK_INTERVAL);
    }
    FC_LOG_AND_RETHROW();
}

/*

BOOST_FIXTURE_TEST_CASE( hardfork_test, database_integration_fixture )
{
   try
   {
      try {
      int argc = boost::unit_test::framework::master_test_suite().argc;
      char** argv = boost::unit_test::framework::master_test_suite().argv;
      for( int i=1; i<argc; i++ )
      {
         const std::string arg = argv[i];
         if( arg == "--record-assert-trip" )
            fc::enable_record_assert_trip = true;
         if( arg == "--show-test-names" )
            std::cout << "running test " << boost::unit_test::framework::current_test_case().p_name << std::endl;
      }
      auto ahplugin = app.register_plugin< scorum::blockchain_history::account_history_plugin >();
      db_plugin = app.register_plugin< scorum::plugin::debug_node::debug_node_plugin >();
      init_account_pub_key = init_account_priv_key.get_public_key();

      boost::program_options::variables_map options;

      ahplugin->plugin_initialize( options );
      db_plugin->plugin_initialize( options );

      open_database();

      generate_blocks( 2 );

      ahplugin->plugin_startup();
      db_plugin->plugin_startup();

      vest( "initdelegate", 10000 );

      // Fill up the rest of the required miners
      for( int i = SCORUM_NUM_INIT_MINERS; i < SCORUM_MAX_WITNESSES; i++ )
      {
         account_create( SCORUM_INIT_MINER_NAME + fc::to_string( i ), init_account_pub_key );
         fund( SCORUM_INIT_MINER_NAME + fc::to_string( i ), SCORUM_MIN_PRODUCER_REWARD.amount.value );
         witness_create( SCORUM_INIT_MINER_NAME + fc::to_string( i ), init_account_priv_key, "foo.bar",
init_account_pub_key, SCORUM_MIN_PRODUCER_REWARD.amount );
      }

      validate_database();
      } catch ( const fc::exception& e )
      {
         edump( (e.to_detail_string()) );
         throw;
      }

      BOOST_TEST_MESSAGE( "Check hardfork not applied at genesis" );
      BOOST_REQUIRE( db.has_hardfork( 0 ) );
      BOOST_REQUIRE( !db.has_hardfork( SCORUM_HARDFORK_0_1 ) );

      BOOST_TEST_MESSAGE( "Generate blocks up to the hardfork time and check hardfork still not applied" );
      generate_blocks( fc::time_point_sec( SCORUM_HARDFORK_0_1_TIME - SCORUM_BLOCK_INTERVAL ), true );

      BOOST_REQUIRE( db.has_hardfork( 0 ) );
      BOOST_REQUIRE( !db.has_hardfork( SCORUM_HARDFORK_0_1 ) );

      BOOST_TEST_MESSAGE( "Generate a block and check hardfork is applied" );
      generate_block();

      std::string op_msg = "Testnet: Hardfork applied";
      auto itr = db.get_index< account_operations_full_history_index >().indices().get< by_id >().end();
      itr--;

      BOOST_REQUIRE( db.has_hardfork( 0 ) );
      BOOST_REQUIRE( db.has_hardfork( SCORUM_HARDFORK_0_1 ) );
      BOOST_REQUIRE( get_last_operations( 1 )[0].get< custom_operation >().data == vector< char >( op_msg.begin(),
op_msg.end() ) );
      BOOST_REQUIRE( db.get(itr->op).timestamp == db.head_block_time() );

      BOOST_TEST_MESSAGE( "Testing hardfork is only applied once" );
      generate_block();

      itr = db.get_index< account_operations_full_history_index >().indices().get< by_id >().end();
      itr--;

      BOOST_REQUIRE( db.has_hardfork( 0 ) );
      BOOST_REQUIRE( db.has_hardfork( SCORUM_HARDFORK_0_1 ) );
      BOOST_REQUIRE( get_last_operations( 1 )[0].get< custom_operation >().data == vector< char >( op_msg.begin(),
op_msg.end() ) );
      BOOST_REQUIRE( db.get(itr->op).timestamp == db.head_block_time() - SCORUM_BLOCK_INTERVAL );
   }
   FC_LOG_AND_RETHROW()
}
*/

BOOST_AUTO_TEST_SUITE_END()
