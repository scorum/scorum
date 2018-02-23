#ifdef IS_TEST_NET
#include <boost/test/unit_test.hpp>

#include <scorum/protocol/exceptions.hpp>

#include <scorum/chain/database.hpp>
#include <scorum/chain/database_exceptions.hpp>
#include <scorum/chain/hardfork.hpp>
#include <scorum/chain/schema/scorum_objects.hpp>

#include <scorum/chain/util/reward.hpp>

#include <scorum/witness/witness_objects.hpp>

#include <fc/crypto/digest.hpp>

#include "database_default_integration.hpp"

#include <scorum/chain/services/account.hpp>
#include <scorum/chain/services/comment_vote.hpp>
#include <scorum/chain/services/witness.hpp>

#include <cmath>
#include <iostream>
#include <stdexcept>

using namespace scorum;
using namespace scorum::chain;
using namespace scorum::protocol;
using fc::string;

BOOST_AUTO_TEST_SUITE(test_account_create_operation_get_authorities)

BOOST_AUTO_TEST_CASE(there_is_no_owner_authority)
{
    try
    {
        account_create_operation op;
        op.creator = "alice";
        op.new_account_name = "bob";

        flat_set<account_name_type> authorities;

        op.get_required_owner_authorities(authorities);

        BOOST_CHECK(authorities.empty() == true);
    }
    FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE(there_is_no_posting_authority)
{
    try
    {
        account_create_operation op;
        op.creator = "alice";
        op.new_account_name = "bob";

        flat_set<account_name_type> authorities;

        op.get_required_posting_authorities(authorities);

        BOOST_CHECK(authorities.empty() == true);
    }
    FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE(creator_have_active_authority)
{
    try
    {
        account_create_operation op;
        op.creator = "alice";
        op.new_account_name = "bob";

        flat_set<account_name_type> authorities;

        op.get_required_active_authorities(authorities);

        const flat_set<account_name_type> expected = { "alice" };

        BOOST_CHECK(authorities == expected);
    }
    FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_FIXTURE_TEST_SUITE(operation_tests, database_default_integration_fixture)

BOOST_AUTO_TEST_CASE(account_create_apply)
{
    try
    {
        BOOST_TEST_MESSAGE("Testing: account_create_apply");

        generate_blocks(SCORUM_BLOCKS_PER_HOUR);

        private_key_type priv_key = generate_private_key("alice");

        const account_object& init = db.obtain_service<dbs_account>().get_account(TEST_INIT_DELEGATE_NAME);
        asset init_starting_balance = init.balance;

        account_create_operation op;

        op.fee = SUFFICIENT_FEE;
        op.new_account_name = "alice";
        op.creator = TEST_INIT_DELEGATE_NAME;
        op.owner = authority(1, priv_key.get_public_key(), 1);
        op.active = authority(2, priv_key.get_public_key(), 2);
        op.memo_key = priv_key.get_public_key();
        op.json_metadata = "{\"foo\":\"bar\"}";

        BOOST_TEST_MESSAGE("--- Test normal account creation");
        signed_transaction tx;
        tx.set_expiration(db.head_block_time() + SCORUM_MAX_TIME_UNTIL_EXPIRATION);
        tx.operations.push_back(op);
        tx.sign(init_account_priv_key, db.get_chain_id());
        tx.validate();
        db.push_transaction(tx, 0);

        const account_object& acct = db.obtain_service<dbs_account>().get_account("alice");
        const account_authority_object& acct_auth = db.get<account_authority_object, by_account>("alice");

        BOOST_REQUIRE(acct.name == "alice");
        BOOST_REQUIRE(acct_auth.owner == authority(1, priv_key.get_public_key(), 1));
        BOOST_REQUIRE(acct_auth.active == authority(2, priv_key.get_public_key(), 2));
        BOOST_REQUIRE(acct.memo_key == priv_key.get_public_key());
        BOOST_REQUIRE(acct.proxy == "");
        BOOST_REQUIRE(acct.created == db.head_block_time());
        BOOST_REQUIRE(acct.balance.amount.value == ASSET_SCR(0).amount.value);
        BOOST_REQUIRE(acct.id._id == acct_auth.id._id);

        /// because init_witness has created vesting shares and blocks have been produced, 100 SCR is worth less than
        /// 100 vesting shares due to rounding
        BOOST_REQUIRE_EQUAL(acct.vesting_shares.amount.value, op.fee.amount.value);
        BOOST_REQUIRE(acct.vesting_withdraw_rate.amount.value == ASSET_SP(0).amount.value);
        BOOST_REQUIRE(acct.proxied_vsf_votes_total().value == 0);
        BOOST_REQUIRE_EQUAL((init_starting_balance - SUFFICIENT_FEE).amount.value, init.balance.amount.value);
        validate_database();

        BOOST_TEST_MESSAGE("--- Test failure of duplicate account creation");
        BOOST_REQUIRE_THROW(db.push_transaction(tx, database::skip_transaction_dupe_check), fc::exception);

        BOOST_REQUIRE(acct.name == "alice");
        BOOST_REQUIRE(acct_auth.owner == authority(1, priv_key.get_public_key(), 1));
        BOOST_REQUIRE(acct_auth.active == authority(2, priv_key.get_public_key(), 2));
        BOOST_REQUIRE(acct.memo_key == priv_key.get_public_key());
        BOOST_REQUIRE(acct.proxy == "");
        BOOST_REQUIRE(acct.created == db.head_block_time());
        BOOST_REQUIRE(acct.balance.amount.value == ASSET_SCR(0).amount.value);
        BOOST_REQUIRE_EQUAL(acct.vesting_shares.amount.value, op.fee.amount.value);
        BOOST_REQUIRE(acct.vesting_withdraw_rate.amount.value == ASSET_SP(0).amount.value);
        BOOST_REQUIRE(acct.proxied_vsf_votes_total().value == 0);
        BOOST_REQUIRE_EQUAL((init_starting_balance - SUFFICIENT_FEE).amount.value, init.balance.amount.value);
        validate_database();

        BOOST_TEST_MESSAGE("--- Test failure when creator cannot cover fee");
        tx.signatures.clear();
        tx.operations.clear();
        op.fee = asset(db.obtain_service<dbs_account>().get_account(TEST_INIT_DELEGATE_NAME).balance.amount + 1,
                       SCORUM_SYMBOL);
        op.new_account_name = "bob";
        tx.operations.push_back(op);
        tx.sign(init_account_priv_key, db.get_chain_id());
        SCORUM_REQUIRE_THROW(db.push_transaction(tx, 0), fc::exception);
        validate_database();

        BOOST_TEST_MESSAGE("--- Test failure covering witness fee");
        generate_block();
        db_plugin->debug_update([=](database& db) {
            db.modify(db.get_dynamic_global_properties(), [&](dynamic_global_property_object& dgpo) {
                dgpo.median_chain_props.account_creation_fee = SUFFICIENT_FEE * 10;
            });
        });
        generate_block();

        tx.clear();
        op.fee = SUFFICIENT_FEE;
        tx.operations.push_back(op);
        tx.sign(init_account_priv_key, db.get_chain_id());
        SCORUM_REQUIRE_THROW(db.push_transaction(tx, 0), fc::exception);
        validate_database();
    }
    FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE(account_update_validate)
{
    try
    {
        BOOST_TEST_MESSAGE("Testing: account_update_validate");

        ACTORS((alice))

        account_update_operation op;
        op.account = "alice";
        op.posting = authority();
        op.posting->weight_threshold = 1;
        op.posting->add_authorities("abcdefghijklmnopq", 1);

        try
        {
            op.validate();

            signed_transaction tx;
            tx.set_expiration(db.head_block_time() + SCORUM_MAX_TIME_UNTIL_EXPIRATION);
            tx.operations.push_back(op);
            tx.sign(alice_private_key, db.get_chain_id());
            db.push_transaction(tx, 0);

            BOOST_FAIL("An exception was not thrown for an invalid account name");
        }
        catch (fc::exception&)
        {
        }

        validate_database();
    }
    FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE(account_update_authorities)
{
    try
    {
        BOOST_TEST_MESSAGE("Testing: account_update_authorities");

        ACTORS((alice)(bob))
        private_key_type active_key = generate_private_key("new_key");

        db.modify(db.get<account_authority_object, by_account>("alice"),
                  [&](account_authority_object& a) { a.active = authority(1, active_key.get_public_key(), 1); });

        account_update_operation op;
        op.account = "alice";
        op.json_metadata = "{\"success\":true}";

        signed_transaction tx;
        tx.operations.push_back(op);
        tx.set_expiration(db.head_block_time() + SCORUM_MAX_TIME_UNTIL_EXPIRATION);

        BOOST_TEST_MESSAGE("  Tests when owner authority is not updated ---");
        BOOST_TEST_MESSAGE("--- Test failure when no signature");
        SCORUM_REQUIRE_THROW(db.push_transaction(tx, 0), tx_missing_active_auth);

        BOOST_TEST_MESSAGE("--- Test failure when wrong signature");
        tx.sign(bob_private_key, db.get_chain_id());
        SCORUM_REQUIRE_THROW(db.push_transaction(tx, 0), tx_missing_active_auth);

        BOOST_TEST_MESSAGE("--- Test failure when containing additional incorrect signature");
        tx.sign(alice_private_key, db.get_chain_id());
        SCORUM_REQUIRE_THROW(db.push_transaction(tx, 0), tx_irrelevant_sig);

        BOOST_TEST_MESSAGE("--- Test failure when containing duplicate signatures");
        tx.signatures.clear();
        tx.sign(active_key, db.get_chain_id());
        tx.sign(active_key, db.get_chain_id());
        SCORUM_REQUIRE_THROW(db.push_transaction(tx, 0), tx_duplicate_sig);

        BOOST_TEST_MESSAGE("--- Test success on active key");
        tx.signatures.clear();
        tx.sign(active_key, db.get_chain_id());
        db.push_transaction(tx, 0);

        BOOST_TEST_MESSAGE("--- Test success on owner key alone");
        tx.signatures.clear();
        tx.sign(alice_private_key, db.get_chain_id());
        db.push_transaction(tx, database::skip_transaction_dupe_check);

        BOOST_TEST_MESSAGE("  Tests when owner authority is updated ---");
        BOOST_TEST_MESSAGE("--- Test failure when updating the owner authority with an active key");
        tx.signatures.clear();
        tx.operations.clear();
        op.owner = authority(1, active_key.get_public_key(), 1);
        tx.operations.push_back(op);
        tx.sign(active_key, db.get_chain_id());
        SCORUM_REQUIRE_THROW(db.push_transaction(tx, 0), tx_missing_owner_auth);

        BOOST_TEST_MESSAGE("--- Test failure when owner key and active key are present");
        tx.sign(alice_private_key, db.get_chain_id());
        SCORUM_REQUIRE_THROW(db.push_transaction(tx, 0), tx_irrelevant_sig);

        BOOST_TEST_MESSAGE("--- Test failure when incorrect signature");
        tx.signatures.clear();
        tx.sign(alice_post_key, db.get_chain_id());
        SCORUM_REQUIRE_THROW(db.push_transaction(tx, 0), tx_missing_owner_auth);

        BOOST_TEST_MESSAGE("--- Test failure when duplicate owner keys are present");
        tx.signatures.clear();
        tx.sign(alice_private_key, db.get_chain_id());
        tx.sign(alice_private_key, db.get_chain_id());
        SCORUM_REQUIRE_THROW(db.push_transaction(tx, 0), tx_duplicate_sig);

        BOOST_TEST_MESSAGE("--- Test success when updating the owner authority with an owner key");
        tx.signatures.clear();
        tx.sign(alice_private_key, db.get_chain_id());
        db.push_transaction(tx, 0);

        validate_database();
    }
    FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE(account_update_apply)
{
    try
    {
        BOOST_TEST_MESSAGE("Testing: account_update_apply");

        ACTORS((alice))
        private_key_type new_private_key = generate_private_key("new_key");

        BOOST_TEST_MESSAGE("--- Test normal update");

        account_update_operation op;
        op.account = "alice";
        op.owner = authority(1, new_private_key.get_public_key(), 1);
        op.active = authority(2, new_private_key.get_public_key(), 2);
        op.memo_key = new_private_key.get_public_key();
        op.json_metadata = "{\"bar\":\"foo\"}";

        signed_transaction tx;
        tx.operations.push_back(op);
        tx.set_expiration(db.head_block_time() + SCORUM_MAX_TIME_UNTIL_EXPIRATION);
        tx.sign(alice_private_key, db.get_chain_id());
        db.push_transaction(tx, 0);

        const account_object& acct = db.obtain_service<dbs_account>().get_account("alice");
        const account_authority_object& acct_auth = db.get<account_authority_object, by_account>("alice");

        BOOST_REQUIRE(acct.name == "alice");
        BOOST_REQUIRE(acct_auth.owner == authority(1, new_private_key.get_public_key(), 1));
        BOOST_REQUIRE(acct_auth.active == authority(2, new_private_key.get_public_key(), 2));
        BOOST_REQUIRE(acct.memo_key == new_private_key.get_public_key());

        /* This is being moved out of consensus
        #ifndef IS_LOW_MEM
           BOOST_REQUIRE( acct.json_metadata == "{\"bar\":\"foo\"}" );
        #else
           BOOST_REQUIRE( acct.json_metadata == "" );
        #endif
        */

        validate_database();

        BOOST_TEST_MESSAGE("--- Test failure when updating a non-existent account");
        tx.operations.clear();
        tx.signatures.clear();
        op.account = "bob";
        tx.operations.push_back(op);
        tx.sign(new_private_key, db.get_chain_id());
        SCORUM_REQUIRE_THROW(db.push_transaction(tx, 0), fc::exception)
        validate_database();

        BOOST_TEST_MESSAGE("--- Test failure when account authority does not exist");
        tx.clear();
        op = account_update_operation();
        op.account = "alice";
        op.posting = authority();
        op.posting->weight_threshold = 1;
        op.posting->add_authorities("dave", 1);
        tx.operations.push_back(op);
        tx.sign(new_private_key, db.get_chain_id());
        SCORUM_REQUIRE_THROW(db.push_transaction(tx, 0), fc::exception);
        validate_database();
    }
    FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE(comment_validate)
{
    try
    {
        BOOST_TEST_MESSAGE("Testing: comment_validate");

        validate_database();
    }
    FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE(comment_authorities)
{
    try
    {
        BOOST_TEST_MESSAGE("Testing: comment_authorities");

        ACTORS((alice)(bob));
        generate_blocks(60 / SCORUM_BLOCK_INTERVAL);

        comment_operation op;
        op.author = "alice";
        op.permlink = "lorem";
        op.parent_author = "";
        op.parent_permlink = "ipsum";
        op.title = "Lorem Ipsum";
        op.body = "Lorem ipsum dolor sit amet, consectetur adipiscing elit, sed do eiusmod tempor incididunt ut labore "
                  "et dolore magna aliqua.";
        op.json_metadata = "{\"foo\":\"bar\"}";

        signed_transaction tx;
        tx.operations.push_back(op);
        tx.set_expiration(db.head_block_time() + SCORUM_MAX_TIME_UNTIL_EXPIRATION);

        BOOST_TEST_MESSAGE("--- Test failure when no signatures");
        SCORUM_REQUIRE_THROW(db.push_transaction(tx, 0), tx_missing_posting_auth);

        BOOST_TEST_MESSAGE("--- Test failure when duplicate signatures");
        tx.sign(alice_post_key, db.get_chain_id());
        tx.sign(alice_post_key, db.get_chain_id());
        SCORUM_REQUIRE_THROW(db.push_transaction(tx, 0), tx_duplicate_sig);

        BOOST_TEST_MESSAGE("--- Test success with post signature");
        tx.signatures.clear();
        tx.sign(alice_post_key, db.get_chain_id());
        db.push_transaction(tx, 0);

        BOOST_TEST_MESSAGE("--- Test failure when signed by an additional signature not in the creator's authority");
        tx.sign(bob_private_key, db.get_chain_id());
        SCORUM_REQUIRE_THROW(db.push_transaction(tx, database::skip_transaction_dupe_check), tx_irrelevant_sig);

        BOOST_TEST_MESSAGE("--- Test failure when signed by a signature not in the creator's authority");
        tx.signatures.clear();
        tx.sign(bob_private_key, db.get_chain_id());
        SCORUM_REQUIRE_THROW(db.push_transaction(tx, database::skip_transaction_dupe_check), tx_missing_posting_auth);

        validate_database();
    }
    FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE(comment_apply)
{
    try
    {
        BOOST_TEST_MESSAGE("Testing: comment_apply");

        ACTORS((alice)(bob)(sam))
        generate_blocks(60 / SCORUM_BLOCK_INTERVAL);

        comment_operation op;
        op.author = "alice";
        op.permlink = "lorem";
        op.parent_author = "";
        op.parent_permlink = "ipsum";
        op.title = "Lorem Ipsum";
        op.body = "Lorem ipsum dolor sit amet, consectetur adipiscing elit, sed do eiusmod tempor incididunt ut labore "
                  "et dolore magna aliqua.";
        op.json_metadata = "{\"foo\":\"bar\"}";

        signed_transaction tx;
        tx.set_expiration(db.head_block_time() + SCORUM_MAX_TIME_UNTIL_EXPIRATION);

        BOOST_TEST_MESSAGE("--- Test Alice posting a root comment");
        tx.operations.push_back(op);
        tx.sign(alice_private_key, db.get_chain_id());
        db.push_transaction(tx, 0);

        const comment_object& alice_comment = db.get_comment("alice", string("lorem"));

        BOOST_REQUIRE(alice_comment.author == op.author);
        BOOST_REQUIRE(fc::to_string(alice_comment.permlink) == op.permlink);
        BOOST_REQUIRE(fc::to_string(alice_comment.parent_permlink) == op.parent_permlink);
        BOOST_REQUIRE(alice_comment.last_update == db.head_block_time());
        BOOST_REQUIRE(alice_comment.created == db.head_block_time());
        BOOST_REQUIRE(alice_comment.net_rshares.value == 0);
        BOOST_REQUIRE(alice_comment.abs_rshares.value == 0);
        BOOST_REQUIRE(alice_comment.cashout_time
                      == fc::time_point_sec(db.head_block_time() + fc::seconds(SCORUM_CASHOUT_WINDOW_SECONDS)));

#ifndef IS_LOW_MEM
        BOOST_REQUIRE(fc::to_string(alice_comment.title) == op.title);
        BOOST_REQUIRE(fc::to_string(alice_comment.body) == op.body);
// BOOST_REQUIRE( alice_comment.json_metadata == op.json_metadata );
#else
        BOOST_REQUIRE(fc::to_string(alice_comment.title) == "");
        BOOST_REQUIRE(fc::to_string(alice_comment.body) == "");
// BOOST_REQUIRE( alice_comment.json_metadata == "" );
#endif

        validate_database();

        BOOST_TEST_MESSAGE("--- Test Bob posting a comment on a non-existent comment");
        op.author = "bob";
        op.permlink = "ipsum";
        op.parent_author = "alice";
        op.parent_permlink = "foobar";

        tx.signatures.clear();
        tx.operations.clear();
        tx.operations.push_back(op);
        tx.sign(bob_private_key, db.get_chain_id());
        SCORUM_REQUIRE_THROW(db.push_transaction(tx, 0), fc::exception);

        BOOST_TEST_MESSAGE("--- Test Bob posting a comment on Alice's comment");
        op.parent_permlink = "lorem";

        tx.signatures.clear();
        tx.operations.clear();
        tx.operations.push_back(op);
        tx.sign(bob_private_key, db.get_chain_id());
        db.push_transaction(tx, 0);

        const comment_object& bob_comment = db.get_comment("bob", string("ipsum"));

        BOOST_REQUIRE(bob_comment.author == op.author);
        BOOST_REQUIRE(fc::to_string(bob_comment.permlink) == op.permlink);
        BOOST_REQUIRE(bob_comment.parent_author == op.parent_author);
        BOOST_REQUIRE(fc::to_string(bob_comment.parent_permlink) == op.parent_permlink);
        BOOST_REQUIRE(bob_comment.last_update == db.head_block_time());
        BOOST_REQUIRE(bob_comment.created == db.head_block_time());
        BOOST_REQUIRE(bob_comment.net_rshares.value == 0);
        BOOST_REQUIRE(bob_comment.abs_rshares.value == 0);
        BOOST_REQUIRE(bob_comment.cashout_time == bob_comment.created + SCORUM_CASHOUT_WINDOW_SECONDS);
        BOOST_REQUIRE(bob_comment.root_comment == alice_comment.id);
        validate_database();

        BOOST_TEST_MESSAGE("--- Test Sam posting a comment on Bob's comment");

        op.author = "sam";
        op.permlink = "dolor";
        op.parent_author = "bob";
        op.parent_permlink = "ipsum";

        tx.signatures.clear();
        tx.operations.clear();
        tx.operations.push_back(op);
        tx.sign(sam_private_key, db.get_chain_id());
        db.push_transaction(tx, 0);

        const comment_object& sam_comment = db.get_comment("sam", string("dolor"));

        BOOST_REQUIRE(sam_comment.author == op.author);
        BOOST_REQUIRE(fc::to_string(sam_comment.permlink) == op.permlink);
        BOOST_REQUIRE(sam_comment.parent_author == op.parent_author);
        BOOST_REQUIRE(fc::to_string(sam_comment.parent_permlink) == op.parent_permlink);
        BOOST_REQUIRE(sam_comment.last_update == db.head_block_time());
        BOOST_REQUIRE(sam_comment.created == db.head_block_time());
        BOOST_REQUIRE(sam_comment.net_rshares.value == 0);
        BOOST_REQUIRE(sam_comment.abs_rshares.value == 0);
        BOOST_REQUIRE(sam_comment.cashout_time == sam_comment.created + SCORUM_CASHOUT_WINDOW_SECONDS);
        BOOST_REQUIRE(sam_comment.root_comment == alice_comment.id);
        validate_database();

        generate_blocks(60 * 5 / SCORUM_BLOCK_INTERVAL + 1);

        BOOST_TEST_MESSAGE("--- Test modifying a comment");
        const auto& mod_sam_comment = db.get_comment("sam", string("dolor"));
        //        const auto& mod_bob_comment = db.get_comment("bob", string("ipsum"));
        //        const auto& mod_alice_comment = db.get_comment("alice", string("lorem"));
        fc::time_point_sec created = mod_sam_comment.created;

        db.modify(mod_sam_comment, [&](comment_object& com) {
            com.net_rshares = 10;
            com.abs_rshares = 10;
        });

        tx.signatures.clear();
        tx.operations.clear();
        op.title = "foo";
        op.body = "bar";
        op.json_metadata = "{\"bar\":\"foo\"}";
        tx.operations.push_back(op);
        tx.set_expiration(db.head_block_time() + SCORUM_MAX_TIME_UNTIL_EXPIRATION);
        tx.sign(sam_private_key, db.get_chain_id());
        db.push_transaction(tx, 0);

        BOOST_REQUIRE(mod_sam_comment.author == op.author);
        BOOST_REQUIRE(fc::to_string(mod_sam_comment.permlink) == op.permlink);
        BOOST_REQUIRE(mod_sam_comment.parent_author == op.parent_author);
        BOOST_REQUIRE(fc::to_string(mod_sam_comment.parent_permlink) == op.parent_permlink);
        BOOST_REQUIRE(mod_sam_comment.last_update == db.head_block_time());
        BOOST_REQUIRE(mod_sam_comment.created == created);
        BOOST_REQUIRE(mod_sam_comment.cashout_time == mod_sam_comment.created + SCORUM_CASHOUT_WINDOW_SECONDS);
        validate_database();

        BOOST_TEST_MESSAGE("--- Test failure posting withing 1 minute");

        op.permlink = "sit";
        op.parent_author = "";
        op.parent_permlink = "test";
        tx.operations.clear();
        tx.signatures.clear();
        tx.set_expiration(db.head_block_time() + SCORUM_MAX_TIME_UNTIL_EXPIRATION);
        tx.operations.push_back(op);
        tx.sign(sam_private_key, db.get_chain_id());
        db.push_transaction(tx, 0);

        generate_blocks(60 * 5 / SCORUM_BLOCK_INTERVAL);

        op.permlink = "amet";
        tx.operations.clear();
        tx.signatures.clear();
        tx.set_expiration(db.head_block_time() + SCORUM_MAX_TIME_UNTIL_EXPIRATION);
        tx.operations.push_back(op);
        tx.sign(sam_private_key, db.get_chain_id());
        SCORUM_REQUIRE_THROW(db.push_transaction(tx, 0), fc::exception);

        validate_database();

        generate_block();
        db.push_transaction(tx, 0);
        validate_database();
    }
    FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE(comment_delete_apply)
{
    try
    {
        BOOST_TEST_MESSAGE("Testing: comment_delete_apply");
        ACTORS((alice))
        generate_block();

        vest("alice", ASSET_SCR(1e+6));

        generate_block();

        signed_transaction tx;
        comment_operation comment;
        vote_operation vote;

        comment.author = "alice";
        comment.permlink = "test1";
        comment.title = "test";
        comment.body = "foo bar";
        comment.parent_permlink = "test";
        vote.voter = "alice";
        vote.author = "alice";
        vote.permlink = "test1";
        vote.weight = (int16_t)100;
        tx.operations.push_back(comment);
        tx.operations.push_back(vote);
        tx.set_expiration(db.head_block_time() + SCORUM_MIN_TRANSACTION_EXPIRATION_LIMIT);
        tx.sign(alice_private_key, db.get_chain_id());
        db.push_transaction(tx, 0);

        BOOST_TEST_MESSAGE("--- Test failue deleting a comment with positive rshares");

        delete_comment_operation op;
        op.author = "alice";
        op.permlink = "test1";
        tx.clear();
        tx.operations.push_back(op);
        tx.sign(alice_private_key, db.get_chain_id());
        SCORUM_REQUIRE_THROW(db.push_transaction(tx, 0), fc::assert_exception);

        BOOST_TEST_MESSAGE("--- Test success deleting a comment with negative rshares");

        generate_block();
        vote.weight = -1 * (int16_t)100;
        tx.clear();
        tx.operations.push_back(vote);
        tx.operations.push_back(op);
        tx.sign(alice_private_key, db.get_chain_id());
        db.push_transaction(tx, 0);

        auto test_comment = db.find<comment_object, by_permlink>(boost::make_tuple("alice", string("test1")));
        BOOST_REQUIRE(test_comment == nullptr);

        BOOST_TEST_MESSAGE("--- Test failure deleting a comment past cashout");
        generate_blocks(SCORUM_MIN_ROOT_COMMENT_INTERVAL.to_seconds() / SCORUM_BLOCK_INTERVAL);

        tx.clear();
        tx.operations.push_back(comment);
        tx.set_expiration(db.head_block_time() + SCORUM_MIN_TRANSACTION_EXPIRATION_LIMIT);
        tx.sign(alice_private_key, db.get_chain_id());
        db.push_transaction(tx, 0);

        generate_blocks(SCORUM_CASHOUT_WINDOW_SECONDS / SCORUM_BLOCK_INTERVAL);
        BOOST_REQUIRE(db.get_comment("alice", string("test1")).cashout_time == fc::time_point_sec::maximum());

        tx.clear();
        tx.operations.push_back(op);
        tx.set_expiration(db.head_block_time() + SCORUM_MIN_TRANSACTION_EXPIRATION_LIMIT);
        tx.sign(alice_private_key, db.get_chain_id());
        SCORUM_REQUIRE_THROW(db.push_transaction(tx, 0), fc::assert_exception);

        BOOST_TEST_MESSAGE("--- Test failure deleting a comment with a reply");

        comment.permlink = "test2";
        comment.parent_author = "alice";
        comment.parent_permlink = "test1";
        tx.clear();
        tx.operations.push_back(comment);
        tx.set_expiration(db.head_block_time() + SCORUM_MIN_TRANSACTION_EXPIRATION_LIMIT);
        tx.sign(alice_private_key, db.get_chain_id());
        db.push_transaction(tx, 0);

        generate_blocks(SCORUM_MIN_ROOT_COMMENT_INTERVAL.to_seconds() / SCORUM_BLOCK_INTERVAL);
        comment.permlink = "test3";
        comment.parent_permlink = "test2";
        tx.clear();
        tx.operations.push_back(comment);
        tx.set_expiration(db.head_block_time() + SCORUM_MIN_TRANSACTION_EXPIRATION_LIMIT);
        tx.sign(alice_private_key, db.get_chain_id());
        db.push_transaction(tx, 0);

        op.permlink = "test2";
        tx.clear();
        tx.operations.push_back(op);
        tx.sign(alice_private_key, db.get_chain_id());
        SCORUM_REQUIRE_THROW(db.push_transaction(tx, 0), fc::assert_exception);
    }
    FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE(vote_validate)
{
    try
    {
        BOOST_TEST_MESSAGE("Testing: vote_validate");

        validate_database();
    }
    FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE(vote_authorities)
{
    try
    {
        BOOST_TEST_MESSAGE("Testing: vote_authorities");

        validate_database();
    }
    FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_FIXTURE_TEST_SUITE(operation_tests, database_default_integration_fixture)

BOOST_AUTO_TEST_CASE(transfer_validate)
{
    try
    {
        BOOST_TEST_MESSAGE("Testing: transfer_validate");

        validate_database();
    }
    FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE(transfer_authorities)
{
    try
    {
        ACTORS((alice)(bob))
        fund("alice", 10e+3);

        BOOST_TEST_MESSAGE("Testing: transfer_authorities");

        transfer_operation op;
        op.from = "alice";
        op.to = "bob";
        op.amount = ASSET_SCR(25e+2);

        signed_transaction tx;
        tx.set_expiration(db.head_block_time() + SCORUM_MAX_TIME_UNTIL_EXPIRATION);
        tx.operations.push_back(op);

        BOOST_TEST_MESSAGE("--- Test failure when no signatures");
        SCORUM_REQUIRE_THROW(db.push_transaction(tx, 0), tx_missing_active_auth);

        BOOST_TEST_MESSAGE("--- Test failure when signed by a signature not in the account's authority");
        tx.sign(alice_post_key, db.get_chain_id());
        SCORUM_REQUIRE_THROW(db.push_transaction(tx, 0), tx_missing_active_auth);

        BOOST_TEST_MESSAGE("--- Test failure when duplicate signatures");
        tx.signatures.clear();
        tx.sign(alice_private_key, db.get_chain_id());
        tx.sign(alice_private_key, db.get_chain_id());
        SCORUM_REQUIRE_THROW(db.push_transaction(tx, 0), tx_duplicate_sig);

        BOOST_TEST_MESSAGE("--- Test failure when signed by an additional signature not in the creator's authority");
        tx.signatures.clear();
        tx.sign(alice_private_key, db.get_chain_id());
        tx.sign(bob_private_key, db.get_chain_id());
        SCORUM_REQUIRE_THROW(db.push_transaction(tx, 0), tx_irrelevant_sig);

        BOOST_TEST_MESSAGE("--- Test success with witness signature");
        tx.signatures.clear();
        tx.sign(alice_private_key, db.get_chain_id());
        db.push_transaction(tx, 0);

        validate_database();
    }
    FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE(signature_stripping)
{
    try
    {
        // Alice, Bob and Sam all have 2-of-3 multisig on corp.
        // Legitimate tx signed by (Alice, Bob) goes through.
        // Sam shouldn't be able to add or remove signatures to get the transaction to process multiple times.

        ACTORS((alice)(bob)(sam)(corp))
        fund("corp", 10e+3);

        account_update_operation update_op;
        update_op.account = "corp";
        update_op.active = authority(2, "alice", 1, "bob", 1, "sam", 1);

        signed_transaction tx;
        tx.set_expiration(db.head_block_time() + SCORUM_MAX_TIME_UNTIL_EXPIRATION);
        tx.operations.push_back(update_op);

        tx.sign(corp_private_key, db.get_chain_id());
        db.push_transaction(tx, 0);

        tx.operations.clear();
        tx.signatures.clear();

        transfer_operation transfer_op;
        transfer_op.from = "corp";
        transfer_op.to = "sam";
        transfer_op.amount = ASSET_SCR(1e+3);

        tx.operations.push_back(transfer_op);

        tx.sign(alice_private_key, db.get_chain_id());
        signature_type alice_sig = tx.signatures.back();
        SCORUM_REQUIRE_THROW(db.push_transaction(tx, 0), tx_missing_active_auth);
        tx.sign(bob_private_key, db.get_chain_id());
        signature_type bob_sig = tx.signatures.back();
        tx.sign(sam_private_key, db.get_chain_id());
        signature_type sam_sig = tx.signatures.back();
        SCORUM_REQUIRE_THROW(db.push_transaction(tx, 0), tx_irrelevant_sig);

        tx.signatures.clear();
        tx.signatures.push_back(alice_sig);
        tx.signatures.push_back(bob_sig);
        db.push_transaction(tx, 0);

        tx.signatures.clear();
        tx.signatures.push_back(alice_sig);
        tx.signatures.push_back(sam_sig);
        SCORUM_REQUIRE_THROW(db.push_transaction(tx, 0), fc::exception);
    }
    FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE(transfer_apply)
{
    try
    {
        BOOST_TEST_MESSAGE("Testing: transfer_apply");

        ACTORS((alice)(bob))
        fund("alice", 10e+3);

        BOOST_REQUIRE(alice.balance.amount.value == ASSET_SCR(10e+3).amount.value);
        BOOST_REQUIRE(bob.balance.amount.value == ASSET_SCR(0).amount.value);

        signed_transaction tx;
        transfer_operation op;

        op.from = "alice";
        op.to = "bob";
        op.amount = ASSET_SCR(5e+3);

        BOOST_TEST_MESSAGE("--- Test normal transaction");
        tx.operations.push_back(op);
        tx.set_expiration(db.head_block_time() + SCORUM_MAX_TIME_UNTIL_EXPIRATION);
        tx.sign(alice_private_key, db.get_chain_id());
        db.push_transaction(tx, 0);

        BOOST_REQUIRE(alice.balance.amount.value == ASSET_SCR(5e+3).amount.value);
        BOOST_REQUIRE(bob.balance.amount.value == ASSET_SCR(5e+3).amount.value);
        validate_database();

        BOOST_TEST_MESSAGE("--- Generating a block");
        generate_block();

        const auto& new_alice = db.obtain_service<dbs_account>().get_account("alice");
        const auto& new_bob = db.obtain_service<dbs_account>().get_account("bob");

        BOOST_REQUIRE(new_alice.balance.amount.value == ASSET_SCR(5e+3).amount.value);
        BOOST_REQUIRE(new_bob.balance.amount.value == ASSET_SCR(5e+3).amount.value);
        validate_database();

        BOOST_TEST_MESSAGE("--- Test emptying an account");
        tx.signatures.clear();
        tx.operations.clear();
        tx.operations.push_back(op);
        tx.set_expiration(db.head_block_time() + SCORUM_MAX_TIME_UNTIL_EXPIRATION);
        tx.sign(alice_private_key, db.get_chain_id());
        db.push_transaction(tx, database::skip_transaction_dupe_check);

        BOOST_REQUIRE(new_alice.balance.amount.value == ASSET_SCR(0).amount.value);
        BOOST_REQUIRE(new_bob.balance.amount.value == ASSET_SCR(10e+3).amount.value);
        validate_database();

        BOOST_TEST_MESSAGE("--- Test transferring non-existent funds");
        tx.signatures.clear();
        tx.operations.clear();
        tx.operations.push_back(op);
        tx.set_expiration(db.head_block_time() + SCORUM_MAX_TIME_UNTIL_EXPIRATION);
        tx.sign(alice_private_key, db.get_chain_id());
        SCORUM_REQUIRE_THROW(db.push_transaction(tx, database::skip_transaction_dupe_check), fc::exception);

        BOOST_REQUIRE(new_alice.balance.amount.value == ASSET_SCR(0).amount.value);
        BOOST_REQUIRE(new_bob.balance.amount.value == ASSET_SCR(10e+3).amount.value);
        validate_database();
    }
    FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE(transfer_to_vesting_validate)
{
    try
    {
        BOOST_TEST_MESSAGE("Testing: transfer_to_vesting_validate");

        validate_database();
    }
    FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE(transfer_to_vesting_authorities)
{
    try
    {
        ACTORS((alice)(bob))
        fund("alice", 10000);

        BOOST_TEST_MESSAGE("Testing: transfer_to_vesting_authorities");

        transfer_to_vesting_operation op;
        op.from = "alice";
        op.to = "bob";
        op.amount = ASSET_SCR(25e+2);

        signed_transaction tx;
        tx.set_expiration(db.head_block_time() + SCORUM_MAX_TIME_UNTIL_EXPIRATION);
        tx.operations.push_back(op);

        BOOST_TEST_MESSAGE("--- Test failure when no signatures");
        SCORUM_REQUIRE_THROW(db.push_transaction(tx, 0), tx_missing_active_auth);

        BOOST_TEST_MESSAGE("--- Test failure when signed by a signature not in the account's authority");
        tx.sign(alice_post_key, db.get_chain_id());
        SCORUM_REQUIRE_THROW(db.push_transaction(tx, 0), tx_missing_active_auth);

        BOOST_TEST_MESSAGE("--- Test failure when duplicate signatures");
        tx.signatures.clear();
        tx.sign(alice_private_key, db.get_chain_id());
        tx.sign(alice_private_key, db.get_chain_id());
        SCORUM_REQUIRE_THROW(db.push_transaction(tx, 0), tx_duplicate_sig);

        BOOST_TEST_MESSAGE("--- Test failure when signed by an additional signature not in the creator's authority");
        tx.signatures.clear();
        tx.sign(alice_private_key, db.get_chain_id());
        tx.sign(bob_private_key, db.get_chain_id());
        SCORUM_REQUIRE_THROW(db.push_transaction(tx, 0), tx_irrelevant_sig);

        BOOST_TEST_MESSAGE("--- Test success with from signature");
        tx.signatures.clear();
        tx.sign(alice_private_key, db.get_chain_id());
        db.push_transaction(tx, 0);

        validate_database();
    }
    FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE(transfer_to_vesting_apply)
{
    try
    {
        BOOST_TEST_MESSAGE("Testing: transfer_to_vesting_apply");

        ACTORS((alice)(bob))
        fund("alice", 10000);

        const auto& gpo = db.get_dynamic_global_properties();

        BOOST_REQUIRE(alice.balance == ASSET_SCR(10e+3));

        auto shares = gpo.total_vesting_shares;
        auto alice_shares = alice.vesting_shares;
        auto bob_shares = bob.vesting_shares;

        transfer_to_vesting_operation op;
        op.from = "alice";
        op.to = "";
        op.amount = ASSET_SCR(75e+2);

        signed_transaction tx;
        tx.operations.push_back(op);
        tx.set_expiration(db.head_block_time() + SCORUM_MAX_TIME_UNTIL_EXPIRATION);
        tx.sign(alice_private_key, db.get_chain_id());
        db.push_transaction(tx, 0);

        auto new_vest = ASSET_SP(op.amount.amount.value);
        shares += new_vest;
        alice_shares += new_vest;

        BOOST_REQUIRE(alice.balance.amount.value == ASSET_SCR(25e+2).amount.value);
        BOOST_REQUIRE_EQUAL(alice.vesting_shares, alice_shares);
        BOOST_REQUIRE_EQUAL(gpo.total_vesting_shares, shares);

        validate_database();

        op.to = "bob";
        op.amount = asset(2e+3, SCORUM_SYMBOL);
        tx.operations.clear();
        tx.signatures.clear();
        tx.operations.push_back(op);
        tx.set_expiration(db.head_block_time() + SCORUM_MAX_TIME_UNTIL_EXPIRATION);
        tx.sign(alice_private_key, db.get_chain_id());
        db.push_transaction(tx, 0);

        new_vest = ASSET_SP(op.amount.amount.value);
        shares += new_vest;
        bob_shares += new_vest;

        BOOST_REQUIRE(alice.balance.amount.value == ASSET_SCR(500).amount.value);
        BOOST_REQUIRE(alice.vesting_shares.amount.value == alice_shares.amount.value);
        BOOST_REQUIRE(bob.balance.amount.value == ASSET_SCR(0).amount.value);
        BOOST_REQUIRE_EQUAL(bob.vesting_shares, bob_shares);
        BOOST_REQUIRE_EQUAL(gpo.total_vesting_shares, shares);
        validate_database();

        SCORUM_REQUIRE_THROW(db.push_transaction(tx, database::skip_transaction_dupe_check), fc::exception);

        BOOST_REQUIRE(alice.balance.amount.value == ASSET_SCR(500).amount.value);
        BOOST_REQUIRE_EQUAL(alice.vesting_shares, alice_shares);
        BOOST_REQUIRE(bob.balance.amount.value == ASSET_SCR(0).amount.value);
        BOOST_REQUIRE_EQUAL(bob.vesting_shares, bob_shares);
        BOOST_REQUIRE_EQUAL(gpo.total_vesting_shares, shares);
        validate_database();
    }
    FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE(withdraw_vesting_validate)
{
    try
    {
        BOOST_TEST_MESSAGE("Testing: withdraw_vesting_validate");

        validate_database();
    }
    FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE(withdraw_vesting_authorities)
{
    try
    {
        BOOST_TEST_MESSAGE("Testing: withdraw_vesting_authorities");

        ACTORS((alice)(bob))
        fund("alice", 10000);
        vest("alice", 10000);

        withdraw_vesting_operation op;
        op.account = "alice";
        op.vesting_shares = ASSET_SP(1e+3);

        signed_transaction tx;
        tx.operations.push_back(op);
        tx.set_expiration(db.head_block_time() + SCORUM_MAX_TIME_UNTIL_EXPIRATION);

        BOOST_TEST_MESSAGE("--- Test failure when no signature.");
        SCORUM_REQUIRE_THROW(db.push_transaction(tx, database::skip_transaction_dupe_check), tx_missing_active_auth);

        BOOST_TEST_MESSAGE("--- Test success with account signature");
        tx.sign(alice_private_key, db.get_chain_id());
        db.push_transaction(tx, database::skip_transaction_dupe_check);

        BOOST_TEST_MESSAGE("--- Test failure with duplicate signature");
        tx.sign(alice_private_key, db.get_chain_id());
        SCORUM_REQUIRE_THROW(db.push_transaction(tx, database::skip_transaction_dupe_check), tx_duplicate_sig);

        BOOST_TEST_MESSAGE("--- Test failure with additional incorrect signature");
        tx.signatures.clear();
        tx.sign(alice_private_key, db.get_chain_id());
        tx.sign(bob_private_key, db.get_chain_id());
        SCORUM_REQUIRE_THROW(db.push_transaction(tx, database::skip_transaction_dupe_check), tx_irrelevant_sig);

        BOOST_TEST_MESSAGE("--- Test failure with incorrect signature");
        tx.signatures.clear();
        tx.sign(alice_post_key, db.get_chain_id());
        SCORUM_REQUIRE_THROW(db.push_transaction(tx, database::skip_transaction_dupe_check), tx_missing_active_auth);

        validate_database();
    }
    FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE(withdraw_vesting_apply)
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

BOOST_AUTO_TEST_CASE(witness_update_validate)
{
    try
    {
        BOOST_TEST_MESSAGE("Testing: withness_update_validate");

        validate_database();
    }
    FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE(witness_update_authorities)
{
    try
    {
        BOOST_TEST_MESSAGE("Testing: witness_update_authorities");

        ACTORS((alice)(bob));
        fund("alice", 10000);

        private_key_type signing_key = generate_private_key("new_key");

        witness_update_operation op;
        op.owner = "alice";
        op.url = "foo.bar";
        op.block_signing_key = signing_key.get_public_key();

        signed_transaction tx;
        tx.set_expiration(db.head_block_time() + SCORUM_MAX_TIME_UNTIL_EXPIRATION);
        tx.operations.push_back(op);

        BOOST_TEST_MESSAGE("--- Test failure when no signatures");
        SCORUM_REQUIRE_THROW(db.push_transaction(tx, 0), tx_missing_active_auth);

        BOOST_TEST_MESSAGE("--- Test failure when signed by a signature not in the account's authority");
        tx.sign(alice_post_key, db.get_chain_id());
        SCORUM_REQUIRE_THROW(db.push_transaction(tx, 0), tx_missing_active_auth);

        BOOST_TEST_MESSAGE("--- Test failure when duplicate signatures");
        tx.signatures.clear();
        tx.sign(alice_private_key, db.get_chain_id());
        tx.sign(alice_private_key, db.get_chain_id());
        SCORUM_REQUIRE_THROW(db.push_transaction(tx, 0), tx_duplicate_sig);

        BOOST_TEST_MESSAGE("--- Test failure when signed by an additional signature not in the creator's authority");
        tx.signatures.clear();
        tx.sign(alice_private_key, db.get_chain_id());
        tx.sign(bob_private_key, db.get_chain_id());
        SCORUM_REQUIRE_THROW(db.push_transaction(tx, 0), tx_irrelevant_sig);

        BOOST_TEST_MESSAGE("--- Test success with witness signature");
        tx.signatures.clear();
        tx.sign(alice_private_key, db.get_chain_id());
        db.push_transaction(tx, 0);

        tx.signatures.clear();
        tx.sign(signing_key, db.get_chain_id());
        SCORUM_REQUIRE_THROW(db.push_transaction(tx, database::skip_transaction_dupe_check), tx_missing_active_auth);
        validate_database();
    }
    FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE(witness_update_apply)
{
    try
    {
        BOOST_TEST_MESSAGE("Testing: witness_update_apply");

        ACTORS((alice))
        fund("alice", 10000);

        private_key_type signing_key = generate_private_key("new_key");

        BOOST_TEST_MESSAGE("--- Test upgrading an account to a witness");

        witness_update_operation op;
        op.owner = "alice";
        op.url = "foo.bar";
        op.block_signing_key = signing_key.get_public_key();
        op.proposed_chain_props.account_creation_fee = SCORUM_MIN_ACCOUNT_CREATION_FEE + 10;
        op.proposed_chain_props.maximum_block_size = SCORUM_MIN_BLOCK_SIZE_LIMIT + 100;

        signed_transaction tx;
        tx.set_expiration(db.head_block_time() + SCORUM_MAX_TIME_UNTIL_EXPIRATION);
        tx.operations.push_back(op);
        tx.sign(alice_private_key, db.get_chain_id());

        db.push_transaction(tx, 0);

        const witness_object& alice_witness = db.obtain_service<dbs_witness>().get("alice");

        BOOST_REQUIRE(alice_witness.owner == "alice");
        BOOST_REQUIRE(alice_witness.created == db.head_block_time());
        BOOST_REQUIRE(fc::to_string(alice_witness.url) == op.url);
        BOOST_REQUIRE(alice_witness.signing_key == op.block_signing_key);
        BOOST_REQUIRE(alice_witness.proposed_chain_props.account_creation_fee
                      == op.proposed_chain_props.account_creation_fee);
        BOOST_REQUIRE(alice_witness.proposed_chain_props.maximum_block_size
                      == op.proposed_chain_props.maximum_block_size);
        BOOST_REQUIRE(alice_witness.total_missed == 0);
        BOOST_REQUIRE(alice_witness.last_confirmed_block_num == 0);
        BOOST_REQUIRE(alice_witness.votes.value == 0);
        BOOST_REQUIRE(alice_witness.virtual_last_update == 0);
        BOOST_REQUIRE(alice_witness.virtual_position == 0);
        BOOST_REQUIRE(alice_witness.virtual_scheduled_time == fc::uint128_t::max_value());
        BOOST_REQUIRE(alice.balance.amount.value == ASSET_SCR(10e+3).amount.value); // No fee
        validate_database();

        BOOST_TEST_MESSAGE("--- Test updating a witness");

        tx.signatures.clear();
        tx.operations.clear();
        op.url = "bar.foo";
        tx.operations.push_back(op);
        tx.sign(alice_private_key, db.get_chain_id());

        db.push_transaction(tx, 0);

        BOOST_REQUIRE(alice_witness.owner == "alice");
        BOOST_REQUIRE(alice_witness.created == db.head_block_time());
        BOOST_REQUIRE(fc::to_string(alice_witness.url) == "bar.foo");
        BOOST_REQUIRE(alice_witness.signing_key == op.block_signing_key);
        BOOST_REQUIRE(alice_witness.proposed_chain_props.account_creation_fee
                      == op.proposed_chain_props.account_creation_fee);
        BOOST_REQUIRE(alice_witness.proposed_chain_props.maximum_block_size
                      == op.proposed_chain_props.maximum_block_size);
        BOOST_REQUIRE(alice_witness.total_missed == 0);
        BOOST_REQUIRE(alice_witness.last_confirmed_block_num == 0);
        BOOST_REQUIRE(alice_witness.votes.value == 0);
        BOOST_REQUIRE(alice_witness.virtual_last_update == 0);
        BOOST_REQUIRE(alice_witness.virtual_position == 0);
        BOOST_REQUIRE(alice_witness.virtual_scheduled_time == fc::uint128_t::max_value());
        BOOST_REQUIRE(alice.balance.amount.value == ASSET_SCR(10e+3).amount.value);
        validate_database();

        BOOST_TEST_MESSAGE("--- Test failure when upgrading a non-existent account");

        tx.signatures.clear();
        tx.operations.clear();
        op.owner = "bob";
        tx.operations.push_back(op);
        tx.sign(alice_private_key, db.get_chain_id());
        SCORUM_REQUIRE_THROW(db.push_transaction(tx, 0), fc::exception);
        validate_database();
    }
    FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE(account_witness_vote_validate)
{
    try
    {
        BOOST_TEST_MESSAGE("Testing: account_witness_vote_validate");

        validate_database();
    }
    FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE(account_witness_vote_authorities)
{
    try
    {
        BOOST_TEST_MESSAGE("Testing: account_witness_vote_authorities");

        ACTORS((alice)(bob)(sam))

        fund("alice", 1000);
        private_key_type alice_witness_key = generate_private_key("alice_witness");
        witness_create("alice", alice_private_key, "foo.bar", alice_witness_key.get_public_key(), 1000);

        account_witness_vote_operation op;
        op.account = "bob";
        op.witness = "alice";

        signed_transaction tx;
        tx.set_expiration(db.head_block_time() + SCORUM_MAX_TIME_UNTIL_EXPIRATION);
        tx.operations.push_back(op);

        BOOST_TEST_MESSAGE("--- Test failure when no signatures");
        SCORUM_REQUIRE_THROW(db.push_transaction(tx, 0), tx_missing_active_auth);

        BOOST_TEST_MESSAGE("--- Test failure when signed by a signature not in the account's authority");
        tx.sign(bob_post_key, db.get_chain_id());
        SCORUM_REQUIRE_THROW(db.push_transaction(tx, 0), tx_missing_active_auth);

        BOOST_TEST_MESSAGE("--- Test failure when duplicate signatures");
        tx.signatures.clear();
        tx.sign(bob_private_key, db.get_chain_id());
        tx.sign(bob_private_key, db.get_chain_id());
        SCORUM_REQUIRE_THROW(db.push_transaction(tx, 0), tx_duplicate_sig);

        BOOST_TEST_MESSAGE("--- Test failure when signed by an additional signature not in the creator's authority");
        tx.signatures.clear();
        tx.sign(bob_private_key, db.get_chain_id());
        tx.sign(alice_private_key, db.get_chain_id());
        SCORUM_REQUIRE_THROW(db.push_transaction(tx, 0), tx_irrelevant_sig);

        BOOST_TEST_MESSAGE("--- Test success with witness signature");
        tx.signatures.clear();
        tx.sign(bob_private_key, db.get_chain_id());
        db.push_transaction(tx, 0);

        BOOST_TEST_MESSAGE("--- Test failure with proxy signature");
        proxy("bob", "sam");
        tx.signatures.clear();
        tx.sign(sam_private_key, db.get_chain_id());
        SCORUM_REQUIRE_THROW(db.push_transaction(tx, database::skip_transaction_dupe_check), tx_missing_active_auth);

        validate_database();
    }
    FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE(account_witness_vote_apply)
{
    try
    {
        BOOST_TEST_MESSAGE("Testing: account_witness_vote_apply");

        ACTORS((alice)(bob)(sam))
        fund("alice", 5000);
        vest("alice", 5000);
        fund("sam", 1000);

        private_key_type sam_witness_key = generate_private_key("sam_key");
        witness_create("sam", sam_private_key, "foo.bar", sam_witness_key.get_public_key(), 1000);
        const witness_object& sam_witness = db.obtain_service<dbs_witness>().get("sam");

        const auto& witness_vote_idx = db.get_index<witness_vote_index>().indices().get<by_witness_account>();

        BOOST_TEST_MESSAGE("--- Test normal vote");
        account_witness_vote_operation op;
        op.account = "alice";
        op.witness = "sam";
        op.approve = true;

        signed_transaction tx;
        tx.set_expiration(db.head_block_time() + SCORUM_MAX_TIME_UNTIL_EXPIRATION);
        tx.operations.push_back(op);
        tx.sign(alice_private_key, db.get_chain_id());

        db.push_transaction(tx, 0);

        BOOST_REQUIRE(sam_witness.votes == alice.vesting_shares.amount);
        BOOST_REQUIRE(witness_vote_idx.find(std::make_tuple(sam_witness.id, alice.id)) != witness_vote_idx.end());
        validate_database();

        BOOST_TEST_MESSAGE("--- Test revoke vote");
        op.approve = false;
        tx.operations.clear();
        tx.signatures.clear();
        tx.operations.push_back(op);
        tx.sign(alice_private_key, db.get_chain_id());

        db.push_transaction(tx, 0);
        BOOST_REQUIRE(sam_witness.votes.value == 0);
        BOOST_REQUIRE(witness_vote_idx.find(std::make_tuple(sam_witness.id, alice.id)) == witness_vote_idx.end());

        BOOST_TEST_MESSAGE("--- Test failure when attempting to revoke a non-existent vote");

        SCORUM_REQUIRE_THROW(db.push_transaction(tx, database::skip_transaction_dupe_check), fc::exception);
        BOOST_REQUIRE(sam_witness.votes.value == 0);
        BOOST_REQUIRE(witness_vote_idx.find(std::make_tuple(sam_witness.id, alice.id)) == witness_vote_idx.end());

        BOOST_TEST_MESSAGE("--- Test proxied vote");
        proxy("alice", "bob");
        tx.operations.clear();
        tx.signatures.clear();
        op.approve = true;
        op.account = "bob";
        tx.operations.push_back(op);
        tx.sign(bob_private_key, db.get_chain_id());

        db.push_transaction(tx, 0);

        BOOST_REQUIRE(sam_witness.votes == (bob.proxied_vsf_votes_total() + bob.vesting_shares.amount));
        BOOST_REQUIRE(witness_vote_idx.find(std::make_tuple(sam_witness.id, bob.id)) != witness_vote_idx.end());
        BOOST_REQUIRE(witness_vote_idx.find(std::make_tuple(sam_witness.id, alice.id)) == witness_vote_idx.end());

        BOOST_TEST_MESSAGE("--- Test vote from a proxied account");
        tx.operations.clear();
        tx.signatures.clear();
        op.account = "alice";
        tx.operations.push_back(op);
        tx.sign(alice_private_key, db.get_chain_id());
        SCORUM_REQUIRE_THROW(db.push_transaction(tx, database::skip_transaction_dupe_check), fc::exception);

        BOOST_REQUIRE(sam_witness.votes == (bob.proxied_vsf_votes_total() + bob.vesting_shares.amount));
        BOOST_REQUIRE(witness_vote_idx.find(std::make_tuple(sam_witness.id, bob.id)) != witness_vote_idx.end());
        BOOST_REQUIRE(witness_vote_idx.find(std::make_tuple(sam_witness.id, alice.id)) == witness_vote_idx.end());

        BOOST_TEST_MESSAGE("--- Test revoke proxied vote");
        tx.operations.clear();
        tx.signatures.clear();
        op.account = "bob";
        op.approve = false;
        tx.operations.push_back(op);
        tx.sign(bob_private_key, db.get_chain_id());

        db.push_transaction(tx, 0);

        BOOST_REQUIRE(sam_witness.votes.value == 0);
        BOOST_REQUIRE(witness_vote_idx.find(std::make_tuple(sam_witness.id, bob.id)) == witness_vote_idx.end());
        BOOST_REQUIRE(witness_vote_idx.find(std::make_tuple(sam_witness.id, alice.id)) == witness_vote_idx.end());

        BOOST_TEST_MESSAGE("--- Test failure when voting for a non-existent account");
        tx.operations.clear();
        tx.signatures.clear();
        op.witness = "dave";
        op.approve = true;
        tx.operations.push_back(op);
        tx.sign(bob_private_key, db.get_chain_id());

        SCORUM_REQUIRE_THROW(db.push_transaction(tx, 0), fc::exception);
        validate_database();

        BOOST_TEST_MESSAGE("--- Test failure when voting for an account that is not a witness");
        tx.operations.clear();
        tx.signatures.clear();
        op.witness = "alice";
        tx.operations.push_back(op);
        tx.sign(bob_private_key, db.get_chain_id());

        SCORUM_REQUIRE_THROW(db.push_transaction(tx, 0), fc::exception);
        validate_database();
    }
    FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE(account_witness_proxy_validate)
{
    try
    {
        BOOST_TEST_MESSAGE("Testing: account_witness_proxy_validate");

        validate_database();
    }
    FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE(account_witness_proxy_authorities)
{
    try
    {
        BOOST_TEST_MESSAGE("Testing: account_witness_proxy_authorities");

        ACTORS((alice)(bob))

        account_witness_proxy_operation op;
        op.account = "bob";
        op.proxy = "alice";

        signed_transaction tx;
        tx.set_expiration(db.head_block_time() + SCORUM_MAX_TIME_UNTIL_EXPIRATION);
        tx.operations.push_back(op);

        BOOST_TEST_MESSAGE("--- Test failure when no signatures");
        SCORUM_REQUIRE_THROW(db.push_transaction(tx, 0), tx_missing_active_auth);

        BOOST_TEST_MESSAGE("--- Test failure when signed by a signature not in the account's authority");
        tx.sign(bob_post_key, db.get_chain_id());
        SCORUM_REQUIRE_THROW(db.push_transaction(tx, 0), tx_missing_active_auth);

        BOOST_TEST_MESSAGE("--- Test failure when duplicate signatures");
        tx.signatures.clear();
        tx.sign(bob_private_key, db.get_chain_id());
        tx.sign(bob_private_key, db.get_chain_id());
        SCORUM_REQUIRE_THROW(db.push_transaction(tx, 0), tx_duplicate_sig);

        BOOST_TEST_MESSAGE("--- Test failure when signed by an additional signature not in the creator's authority");
        tx.signatures.clear();
        tx.sign(bob_private_key, db.get_chain_id());
        tx.sign(alice_private_key, db.get_chain_id());
        SCORUM_REQUIRE_THROW(db.push_transaction(tx, 0), tx_irrelevant_sig);

        BOOST_TEST_MESSAGE("--- Test success with witness signature");
        tx.signatures.clear();
        tx.sign(bob_private_key, db.get_chain_id());
        db.push_transaction(tx, 0);

        BOOST_TEST_MESSAGE("--- Test failure with proxy signature");
        tx.signatures.clear();
        tx.sign(alice_private_key, db.get_chain_id());
        SCORUM_REQUIRE_THROW(db.push_transaction(tx, database::skip_transaction_dupe_check), tx_missing_active_auth);

        validate_database();
    }
    FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE(account_witness_proxy_apply)
{
    try
    {
        BOOST_TEST_MESSAGE("Testing: account_witness_proxy_apply");

        ACTORS((alice)(bob)(sam)(dave))
        fund("alice", 1000);
        vest("alice", 1000);
        fund("bob", 3000);
        vest("bob", 3000);
        fund("sam", 5000);
        vest("sam", 5000);
        fund("dave", 7000);
        vest("dave", 7000);

        BOOST_TEST_MESSAGE("--- Test setting proxy to another account from self.");
        // bob -> alice

        account_witness_proxy_operation op;
        op.account = "bob";
        op.proxy = "alice";

        signed_transaction tx;
        tx.operations.push_back(op);
        tx.set_expiration(db.head_block_time() + SCORUM_MAX_TIME_UNTIL_EXPIRATION);
        tx.sign(bob_private_key, db.get_chain_id());

        db.push_transaction(tx, 0);

        BOOST_REQUIRE(bob.proxy == "alice");
        BOOST_REQUIRE(bob.proxied_vsf_votes_total().value == 0);
        BOOST_REQUIRE(alice.proxy == SCORUM_PROXY_TO_SELF_ACCOUNT);
        BOOST_REQUIRE(alice.proxied_vsf_votes_total() == bob.vesting_shares.amount);
        validate_database();

        BOOST_TEST_MESSAGE("--- Test changing proxy");
        // bob->sam

        tx.operations.clear();
        tx.signatures.clear();
        op.proxy = "sam";
        tx.operations.push_back(op);
        tx.sign(bob_private_key, db.get_chain_id());

        db.push_transaction(tx, 0);

        BOOST_REQUIRE(bob.proxy == "sam");
        BOOST_REQUIRE(bob.proxied_vsf_votes_total().value == 0);
        BOOST_REQUIRE(alice.proxied_vsf_votes_total().value == 0);
        BOOST_REQUIRE(sam.proxy == SCORUM_PROXY_TO_SELF_ACCOUNT);
        BOOST_REQUIRE(sam.proxied_vsf_votes_total().value == bob.vesting_shares.amount);
        validate_database();

        BOOST_TEST_MESSAGE("--- Test failure when changing proxy to existing proxy");

        SCORUM_REQUIRE_THROW(db.push_transaction(tx, database::skip_transaction_dupe_check), fc::exception);

        BOOST_REQUIRE(bob.proxy == "sam");
        BOOST_REQUIRE(bob.proxied_vsf_votes_total().value == 0);
        BOOST_REQUIRE(sam.proxy == SCORUM_PROXY_TO_SELF_ACCOUNT);
        BOOST_REQUIRE(sam.proxied_vsf_votes_total() == bob.vesting_shares.amount);
        validate_database();

        BOOST_TEST_MESSAGE("--- Test adding a grandparent proxy");
        // bob->sam->dave

        tx.operations.clear();
        tx.signatures.clear();
        op.proxy = "dave";
        op.account = "sam";
        tx.operations.push_back(op);
        tx.sign(sam_private_key, db.get_chain_id());

        db.push_transaction(tx, 0);

        BOOST_REQUIRE(bob.proxy == "sam");
        BOOST_REQUIRE(bob.proxied_vsf_votes_total().value == 0);
        BOOST_REQUIRE(sam.proxy == "dave");
        BOOST_REQUIRE(sam.proxied_vsf_votes_total() == bob.vesting_shares.amount);
        BOOST_REQUIRE(dave.proxy == SCORUM_PROXY_TO_SELF_ACCOUNT);
        BOOST_REQUIRE(dave.proxied_vsf_votes_total() == (sam.vesting_shares + bob.vesting_shares).amount);
        validate_database();

        BOOST_TEST_MESSAGE("--- Test adding a grandchild proxy");
        // alice
        // bob->  sam->dave

        tx.operations.clear();
        tx.signatures.clear();
        op.proxy = "sam";
        op.account = "alice";
        tx.operations.push_back(op);
        tx.sign(alice_private_key, db.get_chain_id());

        db.push_transaction(tx, 0);

        BOOST_REQUIRE(alice.proxy == "sam");
        BOOST_REQUIRE(alice.proxied_vsf_votes_total().value == 0);
        BOOST_REQUIRE(bob.proxy == "sam");
        BOOST_REQUIRE(bob.proxied_vsf_votes_total().value == 0);
        BOOST_REQUIRE(sam.proxy == "dave");
        BOOST_REQUIRE(sam.proxied_vsf_votes_total() == (bob.vesting_shares + alice.vesting_shares).amount);
        BOOST_REQUIRE(dave.proxy == SCORUM_PROXY_TO_SELF_ACCOUNT);
        BOOST_REQUIRE(dave.proxied_vsf_votes_total()
                      == (sam.vesting_shares + bob.vesting_shares + alice.vesting_shares).amount);
        validate_database();

        BOOST_TEST_MESSAGE("--- Test removing a grandchild proxy");
        // alice->sam->dave

        tx.operations.clear();
        tx.signatures.clear();
        op.proxy = SCORUM_PROXY_TO_SELF_ACCOUNT;
        op.account = "bob";
        tx.operations.push_back(op);
        tx.sign(bob_private_key, db.get_chain_id());

        db.push_transaction(tx, 0);

        BOOST_REQUIRE(alice.proxy == "sam");
        BOOST_REQUIRE(alice.proxied_vsf_votes_total().value == 0);
        BOOST_REQUIRE(bob.proxy == SCORUM_PROXY_TO_SELF_ACCOUNT);
        BOOST_REQUIRE(bob.proxied_vsf_votes_total().value == 0);
        BOOST_REQUIRE(sam.proxy == "dave");
        BOOST_REQUIRE(sam.proxied_vsf_votes_total() == alice.vesting_shares.amount);
        BOOST_REQUIRE(dave.proxy == SCORUM_PROXY_TO_SELF_ACCOUNT);
        BOOST_REQUIRE(dave.proxied_vsf_votes_total() == (sam.vesting_shares + alice.vesting_shares).amount);
        validate_database();

        BOOST_TEST_MESSAGE("--- Test votes are transferred when a proxy is added");
        account_witness_vote_operation vote;
        vote.account = "bob";
        vote.witness = TEST_INIT_DELEGATE_NAME;
        tx.operations.clear();
        tx.signatures.clear();
        tx.operations.push_back(vote);
        tx.sign(bob_private_key, db.get_chain_id());

        db.push_transaction(tx, 0);

        tx.operations.clear();
        tx.signatures.clear();
        op.account = "alice";
        op.proxy = "bob";
        tx.operations.push_back(op);
        tx.sign(alice_private_key, db.get_chain_id());

        db.push_transaction(tx, 0);

        BOOST_REQUIRE(db.obtain_service<dbs_witness>().get(TEST_INIT_DELEGATE_NAME).votes
                      == (alice.vesting_shares + bob.vesting_shares).amount);
        validate_database();

        BOOST_TEST_MESSAGE("--- Test votes are removed when a proxy is removed");
        op.proxy = SCORUM_PROXY_TO_SELF_ACCOUNT;
        tx.signatures.clear();
        tx.operations.clear();
        tx.operations.push_back(op);
        tx.sign(alice_private_key, db.get_chain_id());

        db.push_transaction(tx, 0);

        BOOST_REQUIRE(db.obtain_service<dbs_witness>().get(TEST_INIT_DELEGATE_NAME).votes == bob.vesting_shares.amount);
        validate_database();
    }
    FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE(account_recovery)
{
    try
    {
        BOOST_TEST_MESSAGE("Testing: account recovery");

        ACTORS((alice));
        fund("alice", 1000000);

        BOOST_TEST_MESSAGE("Creating account bob with alice");

        account_create_with_delegation_operation acc_create;
        acc_create.fee = SUFFICIENT_FEE;
        acc_create.delegation = ASSET_SP(0);
        acc_create.creator = "alice";
        acc_create.new_account_name = "bob";
        acc_create.owner = authority(1, generate_private_key("bob_owner").get_public_key(), 1);
        acc_create.active = authority(1, generate_private_key("bob_active").get_public_key(), 1);
        acc_create.posting = authority(1, generate_private_key("bob_posting").get_public_key(), 1);
        acc_create.memo_key = generate_private_key("bob_memo").get_public_key();
        acc_create.json_metadata = "";

        signed_transaction tx;
        tx.operations.push_back(acc_create);
        tx.set_expiration(db.head_block_time() + SCORUM_MAX_TIME_UNTIL_EXPIRATION);
        tx.sign(alice_private_key, db.get_chain_id());
        db.push_transaction(tx, 0);

        const auto& bob_auth = db.get<account_authority_object, by_account>("bob");
        BOOST_REQUIRE(bob_auth.owner == acc_create.owner);

        BOOST_TEST_MESSAGE("Changing bob's owner authority");

        account_update_operation acc_update;
        acc_update.account = "bob";
        acc_update.owner = authority(1, generate_private_key("bad_key").get_public_key(), 1);
        acc_update.memo_key = acc_create.memo_key;
        acc_update.json_metadata = "";

        tx.operations.clear();
        tx.signatures.clear();

        tx.operations.push_back(acc_update);
        tx.sign(generate_private_key("bob_owner"), db.get_chain_id());
        db.push_transaction(tx, 0);

        BOOST_REQUIRE(bob_auth.owner == *acc_update.owner);

        BOOST_TEST_MESSAGE("Creating recover request for bob with alice");

        request_account_recovery_operation request;
        request.recovery_account = "alice";
        request.account_to_recover = "bob";
        request.new_owner_authority = authority(1, generate_private_key("new_key").get_public_key(), 1);

        tx.operations.clear();
        tx.signatures.clear();

        tx.operations.push_back(request);
        tx.sign(alice_private_key, db.get_chain_id());
        db.push_transaction(tx, 0);

        BOOST_REQUIRE(bob_auth.owner == *acc_update.owner);

        BOOST_TEST_MESSAGE("Recovering bob's account with original owner auth and new secret");

        generate_blocks(db.head_block_time() + SCORUM_OWNER_UPDATE_LIMIT);

        recover_account_operation recover;
        recover.account_to_recover = "bob";
        recover.new_owner_authority = request.new_owner_authority;
        recover.recent_owner_authority = acc_create.owner;

        tx.operations.clear();
        tx.signatures.clear();

        tx.operations.push_back(recover);
        tx.sign(generate_private_key("bob_owner"), db.get_chain_id());
        tx.sign(generate_private_key("new_key"), db.get_chain_id());
        db.push_transaction(tx, 0);
        const auto& owner1 = db.get<account_authority_object, by_account>("bob").owner;

        BOOST_REQUIRE(owner1 == recover.new_owner_authority);

        BOOST_TEST_MESSAGE("Creating new recover request for a bogus key");

        request.new_owner_authority = authority(1, generate_private_key("foo bar").get_public_key(), 1);

        tx.operations.clear();
        tx.signatures.clear();

        tx.operations.push_back(request);
        tx.sign(alice_private_key, db.get_chain_id());
        db.push_transaction(tx, 0);

        BOOST_TEST_MESSAGE("Testing failure when bob does not have new authority");

        generate_blocks(db.head_block_time() + SCORUM_OWNER_UPDATE_LIMIT + fc::seconds(SCORUM_BLOCK_INTERVAL));

        recover.new_owner_authority = authority(1, generate_private_key("idontknow").get_public_key(), 1);

        tx.operations.clear();
        tx.signatures.clear();

        tx.operations.push_back(recover);
        tx.sign(generate_private_key("bob_owner"), db.get_chain_id());
        tx.sign(generate_private_key("idontknow"), db.get_chain_id());
        SCORUM_REQUIRE_THROW(db.push_transaction(tx, 0), fc::exception);
        const auto& owner2 = db.get<account_authority_object, by_account>("bob").owner;
        BOOST_REQUIRE(owner2 == authority(1, generate_private_key("new_key").get_public_key(), 1));

        BOOST_TEST_MESSAGE("Testing failure when bob does not have old authority");

        recover.recent_owner_authority = authority(1, generate_private_key("idontknow").get_public_key(), 1);
        recover.new_owner_authority = authority(1, generate_private_key("foo bar").get_public_key(), 1);

        tx.operations.clear();
        tx.signatures.clear();

        tx.operations.push_back(recover);
        tx.sign(generate_private_key("foo bar"), db.get_chain_id());
        tx.sign(generate_private_key("idontknow"), db.get_chain_id());
        SCORUM_REQUIRE_THROW(db.push_transaction(tx, 0), fc::exception);
        const auto& owner3 = db.get<account_authority_object, by_account>("bob").owner;
        BOOST_REQUIRE(owner3 == authority(1, generate_private_key("new_key").get_public_key(), 1));

        BOOST_TEST_MESSAGE("Testing using the same old owner auth again for recovery");

        recover.recent_owner_authority = authority(1, generate_private_key("bob_owner").get_public_key(), 1);
        recover.new_owner_authority = authority(1, generate_private_key("foo bar").get_public_key(), 1);

        tx.operations.clear();
        tx.signatures.clear();

        tx.operations.push_back(recover);
        tx.sign(generate_private_key("bob_owner"), db.get_chain_id());
        tx.sign(generate_private_key("foo bar"), db.get_chain_id());
        db.push_transaction(tx, 0);

        const auto& owner4 = db.get<account_authority_object, by_account>("bob").owner;
        BOOST_REQUIRE(owner4 == recover.new_owner_authority);

        BOOST_TEST_MESSAGE("Creating a recovery request that will expire");

        request.new_owner_authority = authority(1, generate_private_key("expire").get_public_key(), 1);

        tx.operations.clear();
        tx.signatures.clear();

        tx.operations.push_back(request);
        tx.sign(alice_private_key, db.get_chain_id());
        db.push_transaction(tx, 0);

        const auto& request_idx = db.get_index<account_recovery_request_index>().indices();
        auto req_itr = request_idx.begin();

        BOOST_REQUIRE(req_itr->account_to_recover == "bob");
        BOOST_REQUIRE(req_itr->new_owner_authority == authority(1, generate_private_key("expire").get_public_key(), 1));
        BOOST_REQUIRE(req_itr->expires == db.head_block_time() + SCORUM_ACCOUNT_RECOVERY_REQUEST_EXPIRATION_PERIOD);
        auto expires = req_itr->expires;
        ++req_itr;
        BOOST_REQUIRE(req_itr == request_idx.end());

        generate_blocks(time_point_sec(expires - SCORUM_BLOCK_INTERVAL), true);

        const auto& new_request_idx = db.get_index<account_recovery_request_index>().indices();
        BOOST_REQUIRE(new_request_idx.begin() != new_request_idx.end());

        generate_block();

        BOOST_REQUIRE(new_request_idx.begin() == new_request_idx.end());

        recover.new_owner_authority = authority(1, generate_private_key("expire").get_public_key(), 1);
        recover.recent_owner_authority = authority(1, generate_private_key("bob_owner").get_public_key(), 1);

        tx.operations.clear();
        tx.signatures.clear();

        tx.operations.push_back(recover);
        tx.set_expiration(db.head_block_time());
        tx.sign(generate_private_key("expire"), db.get_chain_id());
        tx.sign(generate_private_key("bob_owner"), db.get_chain_id());
        SCORUM_REQUIRE_THROW(db.push_transaction(tx, 0), fc::exception);
        const auto& owner5 = db.get<account_authority_object, by_account>("bob").owner;
        BOOST_REQUIRE(owner5 == authority(1, generate_private_key("foo bar").get_public_key(), 1));

        BOOST_TEST_MESSAGE("Expiring owner authority history");

        acc_update.owner = authority(1, generate_private_key("new_key").get_public_key(), 1);

        tx.operations.clear();
        tx.signatures.clear();

        tx.operations.push_back(acc_update);
        tx.set_expiration(db.head_block_time() + SCORUM_MAX_TIME_UNTIL_EXPIRATION);
        tx.sign(generate_private_key("foo bar"), db.get_chain_id());
        db.push_transaction(tx, 0);

        generate_blocks(db.head_block_time()
                        + (SCORUM_OWNER_AUTH_RECOVERY_PERIOD - SCORUM_ACCOUNT_RECOVERY_REQUEST_EXPIRATION_PERIOD));
        generate_block();

        request.new_owner_authority = authority(1, generate_private_key("last key").get_public_key(), 1);

        tx.operations.clear();
        tx.signatures.clear();

        tx.operations.push_back(request);
        tx.set_expiration(db.head_block_time() + SCORUM_MAX_TIME_UNTIL_EXPIRATION);
        tx.sign(alice_private_key, db.get_chain_id());
        db.push_transaction(tx, 0);

        recover.new_owner_authority = request.new_owner_authority;
        recover.recent_owner_authority = authority(1, generate_private_key("bob_owner").get_public_key(), 1);

        tx.operations.clear();
        tx.signatures.clear();

        tx.operations.push_back(recover);
        tx.set_expiration(db.head_block_time() + SCORUM_MAX_TIME_UNTIL_EXPIRATION);
        tx.sign(generate_private_key("bob_owner"), db.get_chain_id());
        tx.sign(generate_private_key("last key"), db.get_chain_id());
        SCORUM_REQUIRE_THROW(db.push_transaction(tx, 0), fc::exception);
        const auto& owner6 = db.get<account_authority_object, by_account>("bob").owner;
        BOOST_REQUIRE(owner6 == authority(1, generate_private_key("new_key").get_public_key(), 1));

        recover.recent_owner_authority = authority(1, generate_private_key("foo bar").get_public_key(), 1);

        tx.operations.clear();
        tx.signatures.clear();

        tx.operations.push_back(recover);
        tx.set_expiration(db.head_block_time() + SCORUM_MAX_TIME_UNTIL_EXPIRATION);
        tx.sign(generate_private_key("foo bar"), db.get_chain_id());
        tx.sign(generate_private_key("last key"), db.get_chain_id());
        db.push_transaction(tx, 0);
        const auto& owner7 = db.get<account_authority_object, by_account>("bob").owner;
        BOOST_REQUIRE(owner7 == authority(1, generate_private_key("last key").get_public_key(), 1));
    }
    FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE(change_recovery_account)
{
    try
    {
        BOOST_TEST_MESSAGE("Testing change_recovery_account_operation");

        ACTORS((alice)(bob)(sam)(tyler))

        auto change_recovery_account
            = [&](const std::string& account_to_recover, const std::string& new_recovery_account) {
                  change_recovery_account_operation op;
                  op.account_to_recover = account_to_recover;
                  op.new_recovery_account = new_recovery_account;

                  signed_transaction tx;
                  tx.operations.push_back(op);
                  tx.set_expiration(db.head_block_time() + SCORUM_MAX_TIME_UNTIL_EXPIRATION);
                  tx.sign(alice_private_key, db.get_chain_id());
                  db.push_transaction(tx, 0);
              };

        auto recover_account = [&](const std::string& account_to_recover, const fc::ecc::private_key& new_owner_key,
                                   const fc::ecc::private_key& recent_owner_key) {
            recover_account_operation op;
            op.account_to_recover = account_to_recover;
            op.new_owner_authority = authority(1, public_key_type(new_owner_key.get_public_key()), 1);
            op.recent_owner_authority = authority(1, public_key_type(recent_owner_key.get_public_key()), 1);

            signed_transaction tx;
            tx.operations.push_back(op);
            tx.set_expiration(db.head_block_time() + SCORUM_MAX_TIME_UNTIL_EXPIRATION);
            tx.sign(recent_owner_key, db.get_chain_id());
            // only Alice -> throw
            SCORUM_REQUIRE_THROW(db.push_transaction(tx, 0), fc::exception);
            tx.signatures.clear();
            tx.sign(new_owner_key, db.get_chain_id());
            // only Sam -> throw
            SCORUM_REQUIRE_THROW(db.push_transaction(tx, 0), fc::exception);
            tx.sign(recent_owner_key, db.get_chain_id());
            // Alice+Sam -> OK
            db.push_transaction(tx, 0);
        };

        auto request_account_recovery
            = [&](const std::string& recovery_account, const fc::ecc::private_key& recovery_account_key,
                  const std::string& account_to_recover, const public_key_type& new_owner_key) {
                  request_account_recovery_operation op;
                  op.recovery_account = recovery_account;
                  op.account_to_recover = account_to_recover;
                  op.new_owner_authority = authority(1, new_owner_key, 1);

                  signed_transaction tx;
                  tx.operations.push_back(op);
                  tx.set_expiration(db.head_block_time() + SCORUM_MAX_TIME_UNTIL_EXPIRATION);
                  tx.sign(recovery_account_key, db.get_chain_id());
                  db.push_transaction(tx, 0);
              };

        auto change_owner = [&](const std::string& account, const fc::ecc::private_key& old_private_key,
                                const public_key_type& new_public_key) {
            account_update_operation op;
            op.account = account;
            op.owner = authority(1, new_public_key, 1);

            signed_transaction tx;
            tx.operations.push_back(op);
            tx.set_expiration(db.head_block_time() + SCORUM_MAX_TIME_UNTIL_EXPIRATION);
            tx.sign(old_private_key, db.get_chain_id());
            db.push_transaction(tx, 0);
        };

        // if either/both users do not exist, we shouldn't allow it
        SCORUM_REQUIRE_THROW(change_recovery_account("alice", "nobody"), fc::exception);
        SCORUM_REQUIRE_THROW(change_recovery_account("haxer", "sam"), fc::exception);
        SCORUM_REQUIRE_THROW(change_recovery_account("haxer", "nobody"), fc::exception);
        change_recovery_account("alice", "sam");

        fc::ecc::private_key alice_priv1 = fc::ecc::private_key::regenerate(fc::sha256::hash("alice_k1"));
        fc::ecc::private_key alice_priv2 = fc::ecc::private_key::regenerate(fc::sha256::hash("alice_k2"));
        public_key_type alice_pub1 = public_key_type(alice_priv1.get_public_key());

        generate_blocks(db.head_block_time() + SCORUM_OWNER_AUTH_RECOVERY_PERIOD - fc::seconds(SCORUM_BLOCK_INTERVAL),
                        true);
        // cannot request account recovery until recovery account is approved
        SCORUM_REQUIRE_THROW(request_account_recovery("sam", sam_private_key, "alice", alice_pub1), fc::exception);
        generate_blocks(1);
        // cannot finish account recovery until requested
        SCORUM_REQUIRE_THROW(recover_account("alice", alice_priv1, alice_private_key), fc::exception);
        // do the request
        request_account_recovery("sam", sam_private_key, "alice", alice_pub1);
        // can't recover with the current owner key
        SCORUM_REQUIRE_THROW(recover_account("alice", alice_priv1, alice_private_key), fc::exception);
        // unless we change it!
        change_owner("alice", alice_private_key, public_key_type(alice_priv2.get_public_key()));
        recover_account("alice", alice_priv1, alice_private_key);
    }
    FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE(escrow_transfer_validate)
{
    try
    {
        BOOST_TEST_MESSAGE("Testing: escrow_transfer_validate");

        escrow_transfer_operation op;
        op.from = "alice";
        op.to = "bob";
        op.scorum_amount = ASSET_SCR(1e+3);
        op.escrow_id = 0;
        op.agent = "sam";
        op.fee = SUFFICIENT_FEE;
        op.json_meta = "";
        op.ratification_deadline = db.head_block_time() + 100;
        op.escrow_expiration = db.head_block_time() + 200;

        BOOST_TEST_MESSAGE("--- failure when symbol != SCR");
        op.scorum_amount = ASSET_SP(1e+3);
        SCORUM_REQUIRE_THROW(op.validate(), fc::exception);

        BOOST_TEST_MESSAGE("--- failure when scorum == 0");
        op.scorum_amount = ASSET_SCR(1e+3);
        op.scorum_amount.amount = 0;
        SCORUM_REQUIRE_THROW(op.validate(), fc::exception);

        BOOST_TEST_MESSAGE("--- failure when scorum < 0");
        op.scorum_amount.amount = -100;
        SCORUM_REQUIRE_THROW(op.validate(), fc::exception);

        BOOST_TEST_MESSAGE("--- failure when fee < 0");
        op.scorum_amount.amount = 1000;
        op.fee.amount = -100;
        SCORUM_REQUIRE_THROW(op.validate(), fc::exception);

        BOOST_TEST_MESSAGE("--- failure when ratification deadline == escrow expiration");
        op.fee.amount = 100;
        op.ratification_deadline = op.escrow_expiration;
        SCORUM_REQUIRE_THROW(op.validate(), fc::exception);

        BOOST_TEST_MESSAGE("--- failure when ratification deadline > escrow expiration");
        op.ratification_deadline = op.escrow_expiration + 100;
        SCORUM_REQUIRE_THROW(op.validate(), fc::exception);

        BOOST_TEST_MESSAGE("--- success");
        op.scorum_amount = ASSET_SCR(1e+3);
        op.ratification_deadline = op.escrow_expiration - 100;
        op.validate();
    }
    FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE(escrow_transfer_authorities)
{
    try
    {
        BOOST_TEST_MESSAGE("Testing: escrow_transfer_authorities");

        escrow_transfer_operation op;
        op.from = "alice";
        op.to = "bob";
        op.scorum_amount = ASSET_SCR(1e+6);
        op.escrow_id = 0;
        op.agent = "sam";
        op.fee = SUFFICIENT_FEE;
        op.json_meta = "";
        op.ratification_deadline = db.head_block_time() + 100;
        op.escrow_expiration = db.head_block_time() + 200;

        flat_set<account_name_type> auths;
        flat_set<account_name_type> expected;

        op.get_required_owner_authorities(auths);
        BOOST_REQUIRE(auths == expected);

        op.get_required_posting_authorities(auths);
        BOOST_REQUIRE(auths == expected);

        op.get_required_active_authorities(auths);
        expected.insert("alice");
        BOOST_REQUIRE(auths == expected);
    }
    FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE(escrow_approve_validate)
{
    try
    {
        BOOST_TEST_MESSAGE("Testing: escrow_approve_validate");

        escrow_approve_operation op;

        op.from = "alice";
        op.to = "bob";
        op.agent = "sam";
        op.who = "bob";
        op.escrow_id = 0;
        op.approve = true;

        BOOST_TEST_MESSAGE("--- failure when who is not to or agent");
        op.who = "dave";
        SCORUM_REQUIRE_THROW(op.validate(), fc::exception);

        BOOST_TEST_MESSAGE("--- success when who is to");
        op.who = op.to;
        op.validate();

        BOOST_TEST_MESSAGE("--- success when who is agent");
        op.who = op.agent;
        op.validate();
    }
    FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE(escrow_approve_authorities)
{
    try
    {
        BOOST_TEST_MESSAGE("Testing: escrow_approve_authorities");

        escrow_approve_operation op;

        op.from = "alice";
        op.to = "bob";
        op.agent = "sam";
        op.who = "bob";
        op.escrow_id = 0;
        op.approve = true;

        flat_set<account_name_type> auths;
        flat_set<account_name_type> expected;

        op.get_required_owner_authorities(auths);
        BOOST_REQUIRE(auths == expected);

        op.get_required_posting_authorities(auths);
        BOOST_REQUIRE(auths == expected);

        op.get_required_active_authorities(auths);
        expected.insert("bob");
        BOOST_REQUIRE(auths == expected);

        expected.clear();
        auths.clear();

        op.who = "sam";
        op.get_required_active_authorities(auths);
        expected.insert("sam");
        BOOST_REQUIRE(auths == expected);
    }
    FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE(escrow_approve_apply)
{
    try
    {
        BOOST_TEST_MESSAGE("Testing: escrow_approve_apply");
        ACTORS((alice)(bob)(sam)(dave))
        fund("alice", 10000);

        escrow_transfer_operation et_op;
        et_op.from = "alice";
        et_op.to = "bob";
        et_op.agent = "sam";
        et_op.scorum_amount = ASSET_SCR(1e+3);
        et_op.fee = SUFFICIENT_FEE;
        et_op.json_meta = "";
        et_op.ratification_deadline = db.head_block_time() + 100;
        et_op.escrow_expiration = db.head_block_time() + 200;

        signed_transaction tx;
        tx.operations.push_back(et_op);
        tx.set_expiration(db.head_block_time() + SCORUM_MAX_TIME_UNTIL_EXPIRATION);
        tx.sign(alice_private_key, db.get_chain_id());
        db.push_transaction(tx, 0);
        tx.operations.clear();
        tx.signatures.clear();

        BOOST_TEST_MESSAGE("---failure when to does not match escrow");
        escrow_approve_operation op;
        op.from = "alice";
        op.to = "dave";
        op.agent = "sam";
        op.who = "dave";
        op.approve = true;

        tx.operations.push_back(op);
        tx.sign(dave_private_key, db.get_chain_id());
        SCORUM_REQUIRE_THROW(db.push_transaction(tx, 0), fc::exception);

        BOOST_TEST_MESSAGE("--- failure when agent does not match escrow");
        op.to = "bob";
        op.agent = "dave";

        tx.operations.clear();
        tx.signatures.clear();

        tx.operations.push_back(op);
        tx.sign(dave_private_key, db.get_chain_id());
        SCORUM_REQUIRE_THROW(db.push_transaction(tx, 0), fc::exception);

        BOOST_TEST_MESSAGE("--- success approving to");
        op.agent = "sam";
        op.who = "bob";

        tx.operations.clear();
        tx.signatures.clear();

        tx.operations.push_back(op);
        tx.sign(bob_private_key, db.get_chain_id());
        db.push_transaction(tx, 0);

        auto& escrow = db.get_escrow(op.from, op.escrow_id);
        BOOST_REQUIRE(escrow.to == "bob");
        BOOST_REQUIRE(escrow.agent == "sam");
        BOOST_REQUIRE(escrow.ratification_deadline == et_op.ratification_deadline);
        BOOST_REQUIRE(escrow.escrow_expiration == et_op.escrow_expiration);
        BOOST_REQUIRE(escrow.scorum_balance == ASSET_SCR(1e+3));
        BOOST_REQUIRE_EQUAL(escrow.pending_fee, SUFFICIENT_FEE);
        BOOST_REQUIRE(escrow.to_approved);
        BOOST_REQUIRE(!escrow.agent_approved);
        BOOST_REQUIRE(!escrow.disputed);

        BOOST_TEST_MESSAGE("--- failure on repeat approval");
        tx.signatures.clear();

        tx.set_expiration(db.head_block_time() + SCORUM_BLOCK_INTERVAL);
        tx.sign(bob_private_key, db.get_chain_id());
        SCORUM_REQUIRE_THROW(db.push_transaction(tx, 0), fc::exception);

        BOOST_REQUIRE(escrow.to == "bob");
        BOOST_REQUIRE(escrow.agent == "sam");
        BOOST_REQUIRE(escrow.ratification_deadline == et_op.ratification_deadline);
        BOOST_REQUIRE(escrow.escrow_expiration == et_op.escrow_expiration);
        BOOST_REQUIRE(escrow.scorum_balance == ASSET_SCR(1e+3));
        BOOST_REQUIRE_EQUAL(escrow.pending_fee, SUFFICIENT_FEE);
        BOOST_REQUIRE(escrow.to_approved);
        BOOST_REQUIRE(!escrow.agent_approved);
        BOOST_REQUIRE(!escrow.disputed);

        BOOST_TEST_MESSAGE("--- failure trying to repeal after approval");
        tx.signatures.clear();
        tx.operations.clear();

        op.approve = false;

        tx.operations.push_back(op);
        tx.sign(bob_private_key, db.get_chain_id());
        SCORUM_REQUIRE_THROW(db.push_transaction(tx, 0), fc::exception);

        BOOST_REQUIRE(escrow.to == "bob");
        BOOST_REQUIRE(escrow.agent == "sam");
        BOOST_REQUIRE(escrow.ratification_deadline == et_op.ratification_deadline);
        BOOST_REQUIRE(escrow.escrow_expiration == et_op.escrow_expiration);
        BOOST_REQUIRE(escrow.scorum_balance == ASSET_SCR(1e+3));
        BOOST_REQUIRE_EQUAL(escrow.pending_fee, SUFFICIENT_FEE);
        BOOST_REQUIRE(escrow.to_approved);
        BOOST_REQUIRE(!escrow.agent_approved);
        BOOST_REQUIRE(!escrow.disputed);

        BOOST_TEST_MESSAGE("--- success refunding from because of repeal");
        tx.signatures.clear();
        tx.operations.clear();

        op.who = op.agent;

        tx.operations.push_back(op);
        tx.sign(sam_private_key, db.get_chain_id());
        db.push_transaction(tx, 0);

        SCORUM_REQUIRE_THROW(db.get_escrow(op.from, op.escrow_id), fc::exception);
        BOOST_REQUIRE(alice.balance == ASSET_SCR(10e+3));
        validate_database();

        BOOST_TEST_MESSAGE("--- test automatic refund when escrow is not ratified before deadline");
        tx.operations.clear();
        tx.signatures.clear();
        tx.operations.push_back(et_op);
        tx.sign(alice_private_key, db.get_chain_id());
        db.push_transaction(tx, 0);

        generate_blocks(et_op.ratification_deadline + SCORUM_BLOCK_INTERVAL, true);

        SCORUM_REQUIRE_THROW(db.get_escrow(op.from, op.escrow_id), fc::exception);
        BOOST_REQUIRE(db.obtain_service<dbs_account>().get_account("alice").balance == ASSET_SCR(10e+3));
        validate_database();

        BOOST_TEST_MESSAGE("--- test ratification expiration when escrow is only approved by to");
        tx.operations.clear();
        tx.signatures.clear();
        et_op.ratification_deadline = db.head_block_time() + 100;
        et_op.escrow_expiration = db.head_block_time() + 200;
        tx.operations.push_back(et_op);
        tx.set_expiration(db.head_block_time() + SCORUM_MAX_TIME_UNTIL_EXPIRATION);
        tx.sign(alice_private_key, db.get_chain_id());
        db.push_transaction(tx, 0);

        tx.operations.clear();
        tx.signatures.clear();
        op.who = op.to;
        op.approve = true;
        tx.operations.push_back(op);
        tx.sign(bob_private_key, db.get_chain_id());
        db.push_transaction(tx, 0);

        generate_blocks(et_op.ratification_deadline + SCORUM_BLOCK_INTERVAL, true);

        SCORUM_REQUIRE_THROW(db.get_escrow(op.from, op.escrow_id), fc::exception);
        BOOST_REQUIRE(db.obtain_service<dbs_account>().get_account("alice").balance == ASSET_SCR(10e+3));
        validate_database();

        BOOST_TEST_MESSAGE("--- test ratification expiration when escrow is only approved by agent");
        tx.operations.clear();
        tx.signatures.clear();
        et_op.ratification_deadline = db.head_block_time() + 100;
        et_op.escrow_expiration = db.head_block_time() + 200;
        tx.operations.push_back(et_op);
        tx.set_expiration(db.head_block_time() + SCORUM_MAX_TIME_UNTIL_EXPIRATION);
        tx.sign(alice_private_key, db.get_chain_id());
        db.push_transaction(tx, 0);

        tx.operations.clear();
        tx.signatures.clear();
        op.who = op.agent;
        tx.operations.push_back(op);
        tx.sign(sam_private_key, db.get_chain_id());
        db.push_transaction(tx, 0);

        generate_blocks(et_op.ratification_deadline + SCORUM_BLOCK_INTERVAL, true);

        SCORUM_REQUIRE_THROW(db.get_escrow(op.from, op.escrow_id), fc::exception);
        BOOST_REQUIRE(db.obtain_service<dbs_account>().get_account("alice").balance == ASSET_SCR(10e+3));
        validate_database();

        BOOST_TEST_MESSAGE("--- success approving escrow");
        tx.operations.clear();
        tx.signatures.clear();
        et_op.ratification_deadline = db.head_block_time() + 100;
        et_op.escrow_expiration = db.head_block_time() + 200;
        tx.operations.push_back(et_op);
        tx.set_expiration(db.head_block_time() + SCORUM_MAX_TIME_UNTIL_EXPIRATION);
        tx.sign(alice_private_key, db.get_chain_id());
        db.push_transaction(tx, 0);

        tx.operations.clear();
        tx.signatures.clear();
        op.who = op.to;
        tx.operations.push_back(op);
        tx.sign(bob_private_key, db.get_chain_id());
        db.push_transaction(tx, 0);

        tx.operations.clear();
        tx.signatures.clear();
        op.who = op.agent;
        tx.operations.push_back(op);
        tx.sign(sam_private_key, db.get_chain_id());
        db.push_transaction(tx, 0);

        {
            const auto& escrow = db.get_escrow(op.from, op.escrow_id);
            BOOST_REQUIRE(escrow.to == "bob");
            BOOST_REQUIRE(escrow.agent == "sam");
            BOOST_REQUIRE(escrow.ratification_deadline == et_op.ratification_deadline);
            BOOST_REQUIRE(escrow.escrow_expiration == et_op.escrow_expiration);
            BOOST_REQUIRE(escrow.scorum_balance == ASSET_SCR(1e+3));
            BOOST_REQUIRE(escrow.pending_fee == ASSET_SCR(0));
            BOOST_REQUIRE(escrow.to_approved);
            BOOST_REQUIRE(escrow.agent_approved);
            BOOST_REQUIRE(!escrow.disputed);
        }

        BOOST_REQUIRE_EQUAL(db.obtain_service<dbs_account>().get_account("sam").balance, et_op.fee);
        validate_database();

        BOOST_TEST_MESSAGE("--- ratification expiration does not remove an approved escrow");

        generate_blocks(et_op.ratification_deadline + SCORUM_BLOCK_INTERVAL, true);
        {
            const auto& escrow = db.get_escrow(op.from, op.escrow_id);
            BOOST_REQUIRE(escrow.to == "bob");
            BOOST_REQUIRE(escrow.agent == "sam");
            BOOST_REQUIRE(escrow.ratification_deadline == et_op.ratification_deadline);
            BOOST_REQUIRE(escrow.escrow_expiration == et_op.escrow_expiration);
            BOOST_REQUIRE(escrow.scorum_balance == ASSET_SCR(1e+3));
            BOOST_REQUIRE(escrow.pending_fee == ASSET_SCR(0));
            BOOST_REQUIRE(escrow.to_approved);
            BOOST_REQUIRE(escrow.agent_approved);
            BOOST_REQUIRE(!escrow.disputed);
        }

        BOOST_REQUIRE_EQUAL(db.obtain_service<dbs_account>().get_account("sam").balance, et_op.fee);
        validate_database();
    }
    FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE(escrow_dispute_validate)
{
    try
    {
        BOOST_TEST_MESSAGE("Testing: escrow_dispute_validate");
        escrow_dispute_operation op;
        op.from = "alice";
        op.to = "bob";
        op.agent = "alice";
        op.who = "alice";

        BOOST_TEST_MESSAGE("failure when who is not from or to");
        op.who = "sam";
        SCORUM_REQUIRE_THROW(op.validate(), fc::exception);

        BOOST_TEST_MESSAGE("success");
        op.who = "alice";
        op.validate();

        op.who = "bob";
        op.validate();
    }
    FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE(escrow_dispute_authorities)
{
    try
    {
        BOOST_TEST_MESSAGE("Testing: escrow_dispute_authorities");
        escrow_dispute_operation op;
        op.from = "alice";
        op.to = "bob";
        op.who = "alice";

        flat_set<account_name_type> auths;
        flat_set<account_name_type> expected;

        op.get_required_owner_authorities(auths);
        BOOST_REQUIRE(auths == expected);

        op.get_required_posting_authorities(auths);
        BOOST_REQUIRE(auths == expected);

        op.get_required_active_authorities(auths);
        expected.insert("alice");
        BOOST_REQUIRE(auths == expected);

        auths.clear();
        expected.clear();
        op.who = "bob";
        op.get_required_active_authorities(auths);
        expected.insert("bob");
        BOOST_REQUIRE(auths == expected);
    }
    FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE(escrow_dispute_apply)
{
    try
    {
        BOOST_TEST_MESSAGE("Testing: escrow_dispute_apply");

        ACTORS((alice)(bob)(sam)(dave))
        fund("alice", 10000);

        escrow_transfer_operation et_op;
        et_op.from = "alice";
        et_op.to = "bob";
        et_op.agent = "sam";
        et_op.scorum_amount = ASSET_SCR(1e+3);
        et_op.fee = SUFFICIENT_FEE;
        et_op.ratification_deadline = db.head_block_time() + SCORUM_BLOCK_INTERVAL;
        et_op.escrow_expiration = db.head_block_time() + 2 * SCORUM_BLOCK_INTERVAL;

        escrow_approve_operation ea_b_op;
        ea_b_op.from = "alice";
        ea_b_op.to = "bob";
        ea_b_op.agent = "sam";
        ea_b_op.who = "bob";
        ea_b_op.approve = true;

        signed_transaction tx;
        tx.operations.push_back(et_op);
        tx.operations.push_back(ea_b_op);
        tx.set_expiration(db.head_block_time() + SCORUM_MAX_TIME_UNTIL_EXPIRATION);
        tx.sign(alice_private_key, db.get_chain_id());
        tx.sign(bob_private_key, db.get_chain_id());
        db.push_transaction(tx, 0);

        BOOST_TEST_MESSAGE("--- failure when escrow has not been approved");
        escrow_dispute_operation op;
        op.from = "alice";
        op.to = "bob";
        op.agent = "sam";
        op.who = "bob";

        tx.operations.clear();
        tx.signatures.clear();
        tx.operations.push_back(op);
        tx.sign(bob_private_key, db.get_chain_id());
        SCORUM_REQUIRE_THROW(db.push_transaction(tx, 0), fc::exception);

        const auto& escrow = db.get_escrow(et_op.from, et_op.escrow_id);
        BOOST_REQUIRE(escrow.to == "bob");
        BOOST_REQUIRE(escrow.agent == "sam");
        BOOST_REQUIRE(escrow.ratification_deadline == et_op.ratification_deadline);
        BOOST_REQUIRE(escrow.escrow_expiration == et_op.escrow_expiration);
        BOOST_REQUIRE(escrow.scorum_balance == et_op.scorum_amount);
        BOOST_REQUIRE_EQUAL(escrow.pending_fee, et_op.fee);
        BOOST_REQUIRE(escrow.to_approved);
        BOOST_REQUIRE(!escrow.agent_approved);
        BOOST_REQUIRE(!escrow.disputed);

        BOOST_TEST_MESSAGE("--- failure when to does not match escrow");
        escrow_approve_operation ea_s_op;
        ea_s_op.from = "alice";
        ea_s_op.to = "bob";
        ea_s_op.agent = "sam";
        ea_s_op.who = "sam";
        ea_s_op.approve = true;

        tx.operations.clear();
        tx.signatures.clear();
        tx.operations.push_back(ea_s_op);
        tx.sign(sam_private_key, db.get_chain_id());
        db.push_transaction(tx, 0);

        op.to = "dave";
        op.who = "alice";
        tx.operations.clear();
        tx.signatures.clear();
        tx.operations.push_back(op);
        tx.sign(alice_private_key, db.get_chain_id());
        SCORUM_REQUIRE_THROW(db.push_transaction(tx, 0), fc::exception);

        BOOST_REQUIRE(escrow.to == "bob");
        BOOST_REQUIRE(escrow.agent == "sam");
        BOOST_REQUIRE(escrow.ratification_deadline == et_op.ratification_deadline);
        BOOST_REQUIRE(escrow.escrow_expiration == et_op.escrow_expiration);
        BOOST_REQUIRE(escrow.scorum_balance == et_op.scorum_amount);
        BOOST_REQUIRE(escrow.pending_fee == ASSET_SCR(0));
        BOOST_REQUIRE(escrow.to_approved);
        BOOST_REQUIRE(escrow.agent_approved);
        BOOST_REQUIRE(!escrow.disputed);

        BOOST_TEST_MESSAGE("--- failure when agent does not match escrow");
        op.to = "bob";
        op.who = "alice";
        op.agent = "dave";
        tx.operations.clear();
        tx.signatures.clear();
        tx.operations.push_back(op);
        tx.sign(alice_private_key, db.get_chain_id());
        SCORUM_REQUIRE_THROW(db.push_transaction(tx, 0), fc::exception);

        BOOST_REQUIRE(escrow.to == "bob");
        BOOST_REQUIRE(escrow.agent == "sam");
        BOOST_REQUIRE(escrow.ratification_deadline == et_op.ratification_deadline);
        BOOST_REQUIRE(escrow.escrow_expiration == et_op.escrow_expiration);
        BOOST_REQUIRE(escrow.scorum_balance == et_op.scorum_amount);
        BOOST_REQUIRE(escrow.pending_fee == ASSET_SCR(0));
        BOOST_REQUIRE(escrow.to_approved);
        BOOST_REQUIRE(escrow.agent_approved);
        BOOST_REQUIRE(!escrow.disputed);

        BOOST_TEST_MESSAGE("--- failure when escrow is expired");
        generate_blocks(2);

        tx.operations.clear();
        tx.signatures.clear();
        op.agent = "sam";
        tx.operations.push_back(op);
        tx.set_expiration(db.head_block_time() + SCORUM_MAX_TIME_UNTIL_EXPIRATION);
        tx.sign(alice_private_key, db.get_chain_id());
        SCORUM_REQUIRE_THROW(db.push_transaction(tx, 0), fc::exception);

        {
            const auto& escrow = db.get_escrow(et_op.from, et_op.escrow_id);
            BOOST_REQUIRE(escrow.to == "bob");
            BOOST_REQUIRE(escrow.agent == "sam");
            BOOST_REQUIRE(escrow.ratification_deadline == et_op.ratification_deadline);
            BOOST_REQUIRE(escrow.escrow_expiration == et_op.escrow_expiration);
            BOOST_REQUIRE(escrow.scorum_balance == et_op.scorum_amount);
            BOOST_REQUIRE(escrow.pending_fee == ASSET_SCR(0));
            BOOST_REQUIRE(escrow.to_approved);
            BOOST_REQUIRE(escrow.agent_approved);
            BOOST_REQUIRE(!escrow.disputed);
        }

        BOOST_TEST_MESSAGE("--- success disputing escrow");
        et_op.escrow_id = 1;
        et_op.ratification_deadline = db.head_block_time() + SCORUM_BLOCK_INTERVAL;
        et_op.escrow_expiration = db.head_block_time() + 2 * SCORUM_BLOCK_INTERVAL;
        ea_b_op.escrow_id = et_op.escrow_id;
        ea_s_op.escrow_id = et_op.escrow_id;

        tx.operations.clear();
        tx.signatures.clear();
        tx.operations.push_back(et_op);
        tx.operations.push_back(ea_b_op);
        tx.operations.push_back(ea_s_op);
        tx.sign(alice_private_key, db.get_chain_id());
        tx.sign(bob_private_key, db.get_chain_id());
        tx.sign(sam_private_key, db.get_chain_id());
        db.push_transaction(tx, 0);

        tx.operations.clear();
        tx.signatures.clear();
        op.escrow_id = et_op.escrow_id;
        tx.operations.push_back(op);
        tx.sign(alice_private_key, db.get_chain_id());
        db.push_transaction(tx, 0);

        {
            const auto& escrow = db.get_escrow(et_op.from, et_op.escrow_id);
            BOOST_REQUIRE(escrow.to == "bob");
            BOOST_REQUIRE(escrow.agent == "sam");
            BOOST_REQUIRE(escrow.ratification_deadline == et_op.ratification_deadline);
            BOOST_REQUIRE(escrow.escrow_expiration == et_op.escrow_expiration);
            BOOST_REQUIRE(escrow.scorum_balance == et_op.scorum_amount);
            BOOST_REQUIRE(escrow.pending_fee == ASSET_SCR(0));
            BOOST_REQUIRE(escrow.to_approved);
            BOOST_REQUIRE(escrow.agent_approved);
            BOOST_REQUIRE(escrow.disputed);
        }

        BOOST_TEST_MESSAGE("--- failure when escrow is already under dispute");
        tx.operations.clear();
        tx.signatures.clear();
        op.who = "bob";
        tx.operations.push_back(op);
        tx.sign(bob_private_key, db.get_chain_id());
        SCORUM_REQUIRE_THROW(db.push_transaction(tx, 0), fc::exception);

        {
            const auto& escrow = db.get_escrow(et_op.from, et_op.escrow_id);
            BOOST_REQUIRE(escrow.to == "bob");
            BOOST_REQUIRE(escrow.agent == "sam");
            BOOST_REQUIRE(escrow.ratification_deadline == et_op.ratification_deadline);
            BOOST_REQUIRE(escrow.escrow_expiration == et_op.escrow_expiration);
            BOOST_REQUIRE(escrow.scorum_balance == et_op.scorum_amount);
            BOOST_REQUIRE(escrow.pending_fee == ASSET_SCR(0));
            BOOST_REQUIRE(escrow.to_approved);
            BOOST_REQUIRE(escrow.agent_approved);
            BOOST_REQUIRE(escrow.disputed);
        }
    }
    FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE(escrow_release_validate)
{
    try
    {
        BOOST_TEST_MESSAGE("Testing: escrow release validate");
        escrow_release_operation op;
        op.from = "alice";
        op.to = "bob";
        op.who = "alice";
        op.agent = "sam";
        op.receiver = "bob";

        BOOST_TEST_MESSAGE("--- failure when scorum < 0");
        op.scorum_amount.amount = -1;
        SCORUM_REQUIRE_THROW(op.validate(), fc::exception);

        BOOST_TEST_MESSAGE("--- failure when scorum is invalid symbol");
        op.scorum_amount = ASSET_SP(1e+3);
        SCORUM_REQUIRE_THROW(op.validate(), fc::exception);

        BOOST_TEST_MESSAGE("--- success");
        op.scorum_amount = ASSET_SCR(1e+3);
        op.validate();
    }
    FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE(escrow_release_authorities)
{
    try
    {
        BOOST_TEST_MESSAGE("Testing: escrow_release_authorities");
        escrow_release_operation op;
        op.from = "alice";
        op.to = "bob";
        op.who = "alice";

        flat_set<account_name_type> auths;
        flat_set<account_name_type> expected;

        op.get_required_owner_authorities(auths);
        BOOST_REQUIRE(auths == expected);

        op.get_required_posting_authorities(auths);
        BOOST_REQUIRE(auths == expected);

        expected.insert("alice");
        op.get_required_active_authorities(auths);
        BOOST_REQUIRE(auths == expected);

        op.who = "bob";
        auths.clear();
        expected.clear();
        expected.insert("bob");
        op.get_required_active_authorities(auths);
        BOOST_REQUIRE(auths == expected);

        op.who = "sam";
        auths.clear();
        expected.clear();
        expected.insert("sam");
        op.get_required_active_authorities(auths);
        BOOST_REQUIRE(auths == expected);
    }
    FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE(escrow_release_apply)
{
    try
    {
        BOOST_TEST_MESSAGE("Testing: escrow_release_apply");

        ACTORS((alice)(bob)(sam)(dave))

        generate_block();

        fund("alice", 10000);

        generate_block();

        const account_object& alice_vested = db.obtain_service<dbs_account>().get_account("alice");
        const account_object& bob_vested = db.obtain_service<dbs_account>().get_account("bob");
        const account_object& sam_vested = db.obtain_service<dbs_account>().get_account("sam");

        auto initial_alice_balance = alice_vested.balance;
        const auto escrow_transfer_amount = ASSET_SCR(1e+3);
        const auto escrow_release_amount = ASSET_SCR(100);

        escrow_transfer_operation et_op;
        et_op.from = alice_vested.name;
        et_op.to = bob_vested.name;
        et_op.agent = sam_vested.name;
        et_op.scorum_amount = escrow_transfer_amount;
        et_op.fee = SUFFICIENT_FEE;

        escrow_release_operation op;
        op.from = et_op.from;
        op.to = et_op.to;
        op.agent = et_op.agent;
        op.who = et_op.from;
        op.receiver = et_op.to;
        op.scorum_amount = escrow_release_amount;

        escrow_approve_operation ea_b_op;
        ea_b_op.from = alice_vested.name;
        ea_b_op.to = bob_vested.name;
        ea_b_op.agent = sam_vested.name;
        ea_b_op.who = bob_vested.name;

        escrow_approve_operation ea_s_op;
        ea_s_op.from = alice_vested.name;
        ea_s_op.to = bob_vested.name;
        ea_s_op.agent = sam_vested.name;
        ea_s_op.who = sam_vested.name;

        signed_transaction tx;
        tx.set_expiration(db.head_block_time() + SCORUM_MAX_TIME_UNTIL_EXPIRATION);

        {
            et_op.escrow_expiration = fc::time_point_sec::maximum(); // never expire
            et_op.ratification_deadline = fc::time_point_sec::maximum() - SCORUM_BLOCK_INTERVAL;

            tx.operations.push_back(et_op);
            tx.sign(alice_private_key, db.get_chain_id());
            BOOST_REQUIRE_NO_THROW(db.push_transaction(tx, 0));

            generate_block();

            const escrow_object& escrow_vested = db.get_escrow(et_op.from, et_op.escrow_id);

            BOOST_TEST_MESSAGE("--- failure releasing funds prior to approval");

            tx.clear();
            tx.operations.push_back(op);
            tx.sign(alice_private_key, db.get_chain_id());
            SCORUM_REQUIRE_THROW(db.push_transaction(tx, 0), fc::exception);

            tx.clear();
            tx.operations.push_back(ea_b_op);
            tx.operations.push_back(ea_s_op);
            tx.sign(bob_private_key, db.get_chain_id());
            tx.sign(sam_private_key, db.get_chain_id());
            BOOST_REQUIRE_NO_THROW(db.push_transaction(tx, 0));

            BOOST_TEST_MESSAGE("--- failure when 'agent' attempts to release non-disputed escrow to 'to'");
            op.who = et_op.agent;
            tx.clear();
            tx.operations.push_back(op);
            tx.sign(sam_private_key, db.get_chain_id());
            SCORUM_REQUIRE_THROW(db.push_transaction(tx, 0), fc::exception);

            BOOST_TEST_MESSAGE("--- failure when 'agent' attempts to release non-disputed escrow to 'from' ");
            op.receiver = et_op.from;

            tx.clear();
            tx.operations.push_back(op);
            tx.sign(sam_private_key, db.get_chain_id());
            SCORUM_REQUIRE_THROW(db.push_transaction(tx, 0), fc::exception);

            BOOST_TEST_MESSAGE("--- failure when 'agent' attempt to release non-disputed escrow to not 'to' or 'from'");
            op.receiver = "dave";

            tx.clear();
            tx.operations.push_back(op);
            tx.sign(sam_private_key, db.get_chain_id());
            SCORUM_REQUIRE_THROW(db.push_transaction(tx, 0), fc::exception);

            BOOST_TEST_MESSAGE("--- failure when other attempts to release non-disputed escrow to 'to'");
            op.receiver = et_op.to;
            op.who = "dave";

            tx.clear();
            tx.operations.push_back(op);
            tx.sign(dave_private_key, db.get_chain_id());
            SCORUM_REQUIRE_THROW(db.push_transaction(tx, 0), fc::exception);

            BOOST_TEST_MESSAGE("--- failure when other attempts to release non-disputed escrow to 'from' ");
            op.receiver = et_op.from;

            tx.clear();
            tx.operations.push_back(op);
            tx.sign(dave_private_key, db.get_chain_id());
            SCORUM_REQUIRE_THROW(db.push_transaction(tx, 0), fc::exception);

            BOOST_TEST_MESSAGE("--- failure when other attempt to release non-disputed escrow to not 'to' or 'from'");
            op.receiver = "dave";

            tx.clear();
            tx.operations.push_back(op);
            tx.sign(dave_private_key, db.get_chain_id());
            SCORUM_REQUIRE_THROW(db.push_transaction(tx, 0), fc::exception);

            BOOST_TEST_MESSAGE("--- failure when 'to' attemtps to release non-disputed escrow to 'to'");
            op.receiver = et_op.to;
            op.who = et_op.to;

            tx.clear();
            tx.operations.push_back(op);
            tx.sign(bob_private_key, db.get_chain_id());
            SCORUM_REQUIRE_THROW(db.push_transaction(tx, 0), fc::exception);

            BOOST_TEST_MESSAGE("--- failure when 'to' attempts to release non-dispured escrow to 'agent' ");
            op.receiver = et_op.agent;

            tx.clear();
            tx.operations.push_back(op);
            tx.sign(bob_private_key, db.get_chain_id());
            SCORUM_REQUIRE_THROW(db.push_transaction(tx, 0), fc::exception);

            BOOST_TEST_MESSAGE("--- failure when 'to' attempts to release non-disputed escrow to not 'from'");
            op.receiver = "dave";

            tx.clear();
            tx.operations.push_back(op);
            tx.sign(bob_private_key, db.get_chain_id());
            SCORUM_REQUIRE_THROW(db.push_transaction(tx, 0), fc::exception);

            BOOST_TEST_MESSAGE("--- success release non-disputed escrow to 'to' from 'from'");
            op.receiver = et_op.from;

            tx.clear();
            tx.operations.push_back(op);
            tx.sign(bob_private_key, db.get_chain_id());
            BOOST_REQUIRE_NO_THROW(db.push_transaction(tx, 0));

            BOOST_REQUIRE_EQUAL(db.get_escrow(op.from, op.escrow_id).scorum_balance,
                                escrow_transfer_amount - escrow_release_amount);
            BOOST_REQUIRE_EQUAL(alice_vested.balance,
                                initial_alice_balance - SUFFICIENT_FEE - escrow_transfer_amount
                                    + escrow_release_amount);

            BOOST_TEST_MESSAGE("--- failure when 'from' attempts to release non-disputed escrow to 'from'");
            op.receiver = et_op.from;
            op.who = et_op.from;

            tx.clear();
            tx.operations.push_back(op);
            tx.sign(alice_private_key, db.get_chain_id());
            SCORUM_REQUIRE_THROW(db.push_transaction(tx, 0), fc::exception);

            BOOST_TEST_MESSAGE("--- failure when 'from' attempts to release non-disputed escrow to 'agent'");
            op.receiver = et_op.agent;

            tx.clear();
            tx.operations.push_back(op);
            tx.sign(alice_private_key, db.get_chain_id());
            SCORUM_REQUIRE_THROW(db.push_transaction(tx, 0), fc::exception);

            BOOST_TEST_MESSAGE("--- failure when 'from' attempts to release non-disputed escrow to not 'from'");
            op.receiver = "dave";

            tx.clear();
            tx.operations.push_back(op);
            tx.sign(alice_private_key, db.get_chain_id());
            SCORUM_REQUIRE_THROW(db.push_transaction(tx, 0), fc::exception);

            BOOST_TEST_MESSAGE("--- success release non-disputed escrow to 'from' from 'to'");
            op.receiver = et_op.to;

            tx.clear();
            tx.operations.push_back(op);
            tx.sign(alice_private_key, db.get_chain_id());
            BOOST_REQUIRE_NO_THROW(db.push_transaction(tx, 0));

            BOOST_REQUIRE_EQUAL(db.get_escrow(op.from, op.escrow_id).scorum_balance,
                                escrow_transfer_amount - escrow_release_amount * 2);
            BOOST_REQUIRE_EQUAL(bob_vested.balance, escrow_release_amount);

            BOOST_TEST_MESSAGE("--- failure when 'to' attempts to release disputed escrow");
            escrow_dispute_operation ed_op;
            ed_op.from = "alice";
            ed_op.to = "bob";
            ed_op.agent = "sam";
            ed_op.who = "alice";

            tx.clear();
            tx.operations.push_back(ed_op);
            tx.sign(alice_private_key, db.get_chain_id());
            BOOST_REQUIRE_NO_THROW(db.push_transaction(tx, 0));

            tx.clear();
            op.from = et_op.from;
            op.receiver = et_op.from;
            op.who = et_op.to;
            op.scorum_amount = escrow_release_amount;
            tx.operations.push_back(op);
            tx.sign(bob_private_key, db.get_chain_id());
            SCORUM_REQUIRE_THROW(db.push_transaction(tx, 0), fc::exception);

            BOOST_TEST_MESSAGE("--- failure when 'from' attempts to release disputed escrow");
            tx.clear();
            op.receiver = et_op.to;
            op.who = et_op.from;
            tx.operations.push_back(op);
            tx.sign(alice_private_key, db.get_chain_id());
            SCORUM_REQUIRE_THROW(db.push_transaction(tx, 0), fc::exception);

            BOOST_TEST_MESSAGE("--- failure when releasing disputed escrow to an account not 'to' or 'from'");
            tx.clear();
            op.who = et_op.agent;
            op.receiver = "dave";
            tx.operations.push_back(op);
            tx.sign(sam_private_key, db.get_chain_id());
            SCORUM_REQUIRE_THROW(db.push_transaction(tx, 0), fc::exception);

            BOOST_TEST_MESSAGE("--- failure when agent does not match escrow");
            tx.clear();
            op.who = "dave";
            op.receiver = et_op.from;
            tx.operations.push_back(op);
            tx.sign(dave_private_key, db.get_chain_id());
            SCORUM_REQUIRE_THROW(db.push_transaction(tx, 0), fc::exception);

            BOOST_TEST_MESSAGE("--- success releasing disputed escrow with agent to 'to'");
            tx.clear();
            op.receiver = et_op.to;
            op.who = et_op.agent;
            tx.operations.push_back(op);
            tx.sign(sam_private_key, db.get_chain_id());
            BOOST_REQUIRE_NO_THROW(db.push_transaction(tx, 0));

            BOOST_REQUIRE_EQUAL(bob_vested.balance, escrow_release_amount * 2);
            BOOST_REQUIRE_EQUAL(escrow_vested.scorum_balance, escrow_transfer_amount - escrow_release_amount * 3);

            BOOST_TEST_MESSAGE("--- success releasing disputed escrow with agent to 'from'");
            tx.clear();
            op.receiver = et_op.from;
            op.who = et_op.agent;
            tx.operations.push_back(op);
            tx.sign(sam_private_key, db.get_chain_id());
            BOOST_REQUIRE_NO_THROW(db.push_transaction(tx, 0));

            BOOST_REQUIRE_EQUAL(alice_vested.balance,
                                initial_alice_balance - SUFFICIENT_FEE - escrow_transfer_amount
                                    + escrow_release_amount * 2);
            BOOST_REQUIRE_EQUAL(escrow_vested.scorum_balance, escrow_transfer_amount - escrow_release_amount * 4);

            BOOST_TEST_MESSAGE("--- failure when 'to' attempts to release disputed not expired escrow");

            tx.clear();
            op.receiver = et_op.from;
            op.who = et_op.to;
            tx.operations.push_back(op);
            tx.set_expiration(db.head_block_time() + SCORUM_MAX_TIME_UNTIL_EXPIRATION);
            tx.sign(bob_private_key, db.get_chain_id());
            SCORUM_REQUIRE_THROW(db.push_transaction(tx, 0), fc::exception);

            BOOST_TEST_MESSAGE("--- failure when 'from' attempts to release disputed non-expired escrow");
            tx.clear();
            op.receiver = et_op.to;
            op.who = et_op.from;
            tx.operations.push_back(op);
            tx.sign(alice_private_key, db.get_chain_id());
            SCORUM_REQUIRE_THROW(db.push_transaction(tx, 0), fc::exception);

            BOOST_TEST_MESSAGE("--- success releasing disputed non-expired escrow with agent");
            tx.clear();
            op.receiver = et_op.from;
            op.who = et_op.agent;
            tx.operations.push_back(op);
            tx.sign(sam_private_key, db.get_chain_id());
            BOOST_REQUIRE_NO_THROW(db.push_transaction(tx, 0));

            BOOST_REQUIRE_EQUAL(alice_vested.balance,
                                initial_alice_balance - SUFFICIENT_FEE - escrow_transfer_amount
                                    + escrow_release_amount * 3);
            BOOST_REQUIRE_EQUAL(escrow_vested.scorum_balance, escrow_transfer_amount - escrow_release_amount * 5);

            auto rest = escrow_transfer_amount - escrow_release_amount * 5; // rest

            BOOST_TEST_MESSAGE("--- success deleting escrow when balances are both zero");
            tx.clear();
            op.scorum_amount = rest;
            tx.operations.push_back(op);
            tx.sign(sam_private_key, db.get_chain_id());
            BOOST_REQUIRE_NO_THROW(db.push_transaction(tx, 0));

            BOOST_REQUIRE_EQUAL(alice_vested.balance,
                                initial_alice_balance - SUFFICIENT_FEE - escrow_transfer_amount
                                    + escrow_release_amount * 3 + rest);
            SCORUM_REQUIRE_THROW(db.get_escrow(et_op.from, et_op.escrow_id), fc::exception);
        }

        {
            tx.clear();

            et_op.ratification_deadline = db.head_block_time() + SCORUM_BLOCK_INTERVAL;
            et_op.escrow_expiration = db.head_block_time() + 2 * SCORUM_BLOCK_INTERVAL;

            tx.operations.push_back(et_op);
            tx.operations.push_back(ea_b_op);
            tx.operations.push_back(ea_s_op);
            tx.set_expiration(db.head_block_time() + SCORUM_MAX_TIME_UNTIL_EXPIRATION);
            tx.sign(alice_private_key, db.get_chain_id());
            tx.sign(bob_private_key, db.get_chain_id());
            tx.sign(sam_private_key, db.get_chain_id());
            BOOST_REQUIRE_NO_THROW(db.push_transaction(tx, 0));

            generate_block();

            const escrow_object& escrow_vested = db.get_escrow(et_op.from, et_op.escrow_id);

            BOOST_TEST_MESSAGE("--- failure when 'agent' attempts to release non-disputed expired escrow to 'to'");
            tx.clear();
            op.receiver = et_op.to;
            op.who = et_op.agent;
            op.scorum_amount = escrow_release_amount;
            tx.operations.push_back(op);
            tx.sign(sam_private_key, db.get_chain_id());
            SCORUM_REQUIRE_THROW(db.push_transaction(tx, 0), fc::exception);

            BOOST_TEST_MESSAGE("--- failure when 'agent' attempts to release non-disputed expired escrow to 'from'");
            tx.clear();
            op.receiver = et_op.from;
            tx.operations.push_back(op);
            tx.sign(sam_private_key, db.get_chain_id());
            SCORUM_REQUIRE_THROW(db.push_transaction(tx, 0), fc::exception);

            BOOST_TEST_MESSAGE(
                "--- failure when 'agent' attempt to release non-disputed expired escrow to not 'to' or 'from'");
            tx.clear();
            op.receiver = "dave";
            tx.operations.push_back(op);
            tx.sign(sam_private_key, db.get_chain_id());
            SCORUM_REQUIRE_THROW(db.push_transaction(tx, 0), fc::exception);

            BOOST_TEST_MESSAGE("--- failure when 'to' attempts to release non-dispured expired escrow to 'agent'");
            tx.clear();
            op.who = et_op.to;
            op.receiver = et_op.agent;
            tx.operations.push_back(op);
            tx.sign(bob_private_key, db.get_chain_id());
            SCORUM_REQUIRE_THROW(db.push_transaction(tx, 0), fc::exception);

            BOOST_TEST_MESSAGE(
                "--- failure when 'to' attempts to release non-disputed expired escrow to not 'from' or 'to'");
            tx.clear();
            op.receiver = "dave";
            tx.operations.push_back(op);
            tx.sign(bob_private_key, db.get_chain_id());
            SCORUM_REQUIRE_THROW(db.push_transaction(tx, 0), fc::exception);

            generate_blocks(2); // move up time

            BOOST_TEST_MESSAGE("--- success release non-disputed expired escrow to 'to' from 'to'");
            tx.clear();
            op.receiver = et_op.to;
            tx.operations.push_back(op);
            tx.sign(bob_private_key, db.get_chain_id());
            BOOST_REQUIRE_NO_THROW(db.push_transaction(tx, 0));

            BOOST_REQUIRE_EQUAL(bob_vested.balance, escrow_release_amount * 3);
            BOOST_REQUIRE_EQUAL(escrow_vested.scorum_balance, escrow_transfer_amount - escrow_release_amount);

            BOOST_TEST_MESSAGE("--- success release non-disputed expired escrow to 'from' from 'to'");
            tx.clear();
            op.receiver = et_op.from;
            tx.operations.push_back(op);
            tx.sign(bob_private_key, db.get_chain_id());
            BOOST_REQUIRE_NO_THROW(db.push_transaction(tx, 0));

            BOOST_REQUIRE_EQUAL(escrow_vested.scorum_balance, escrow_transfer_amount - escrow_release_amount * 2);

            BOOST_TEST_MESSAGE("--- failure when 'from' attempts to release non-disputed expired escrow to 'agent'");
            tx.clear();
            op.who = et_op.from;
            op.receiver = et_op.agent;
            tx.operations.push_back(op);
            tx.sign(alice_private_key, db.get_chain_id());
            SCORUM_REQUIRE_THROW(db.push_transaction(tx, 0), fc::exception);

            BOOST_TEST_MESSAGE(
                "--- failure when 'from' attempts to release non-disputed expired escrow to not 'from' or 'to'");
            tx.clear();
            op.receiver = "dave";
            tx.operations.push_back(op);
            tx.sign(alice_private_key, db.get_chain_id());
            SCORUM_REQUIRE_THROW(db.push_transaction(tx, 0), fc::exception);

            BOOST_TEST_MESSAGE("--- success release non-disputed expired escrow to 'to' from 'from'");
            tx.clear();
            op.receiver = et_op.to;
            tx.operations.push_back(op);
            tx.sign(alice_private_key, db.get_chain_id());
            BOOST_REQUIRE_NO_THROW(db.push_transaction(tx, 0));

            BOOST_REQUIRE_EQUAL(bob_vested.balance, escrow_release_amount * 4);
            BOOST_REQUIRE_EQUAL(escrow_vested.scorum_balance, escrow_transfer_amount - escrow_release_amount * 3);

            BOOST_TEST_MESSAGE("--- success release non-disputed expired escrow to 'from' from 'from'");
            tx.clear();
            op.receiver = et_op.from;
            tx.operations.push_back(op);
            tx.sign(alice_private_key, db.get_chain_id());
            BOOST_REQUIRE_NO_THROW(db.push_transaction(tx, 0));

            BOOST_REQUIRE_EQUAL(escrow_vested.scorum_balance, escrow_transfer_amount - escrow_release_amount * 4);

            auto rest = escrow_transfer_amount - escrow_release_amount * 4; // rest

            BOOST_TEST_MESSAGE("--- success deleting escrow when balances are zero on non-disputed escrow");
            tx.clear();
            op.scorum_amount = rest;
            tx.operations.push_back(op);
            tx.sign(alice_private_key, db.get_chain_id());
            BOOST_REQUIRE_NO_THROW(db.push_transaction(tx, 0));

            BOOST_REQUIRE_EQUAL(alice_vested.balance,
                                initial_alice_balance - (SUFFICIENT_FEE)*2 - escrow_transfer_amount + rest);
            SCORUM_REQUIRE_THROW(db.get_escrow(et_op.from, et_op.escrow_id), fc::exception);
        }
    }
    FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE(decline_voting_rights_authorities)
{
    try
    {
        BOOST_TEST_MESSAGE("Testing: decline_voting_rights_authorities");

        decline_voting_rights_operation op;
        op.account = "alice";

        flat_set<account_name_type> auths;
        flat_set<account_name_type> expected;

        op.get_required_active_authorities(auths);
        BOOST_REQUIRE(auths == expected);

        op.get_required_posting_authorities(auths);
        BOOST_REQUIRE(auths == expected);

        expected.insert("alice");
        op.get_required_owner_authorities(auths);
        BOOST_REQUIRE(auths == expected);
    }
    FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE(decline_voting_rights_apply)
{
    try
    {
        BOOST_TEST_MESSAGE("Testing: decline_voting_rights_apply");

        ACTORS((alice)(bob));
        generate_block();
        vest("alice", ASSET_SCR(100e+3));
        vest("bob", ASSET_SCR(100e+3));
        generate_block();

        account_witness_proxy_operation proxy;
        proxy.account = "bob";
        proxy.proxy = "alice";

        signed_transaction tx;
        tx.set_expiration(db.head_block_time() + SCORUM_MAX_TIME_UNTIL_EXPIRATION);
        tx.operations.push_back(proxy);
        tx.sign(bob_private_key, db.get_chain_id());
        db.push_transaction(tx, 0);

        decline_voting_rights_operation op;
        op.account = "alice";

        BOOST_TEST_MESSAGE("--- success");
        tx.clear();
        tx.operations.push_back(op);
        tx.sign(alice_private_key, db.get_chain_id());
        db.push_transaction(tx, 0);

        const auto& request_idx = db.get_index<decline_voting_rights_request_index>().indices().get<by_account>();
        auto itr = request_idx.find(db.obtain_service<dbs_account>().get_account("alice").id);
        BOOST_REQUIRE(itr != request_idx.end());
        BOOST_REQUIRE(itr->effective_date == db.head_block_time() + SCORUM_OWNER_AUTH_RECOVERY_PERIOD);

        BOOST_TEST_MESSAGE("--- failure revoking voting rights with existing request");
        generate_block();
        tx.clear();
        tx.operations.push_back(op);
        tx.set_expiration(db.head_block_time() + SCORUM_MAX_TIME_UNTIL_EXPIRATION);
        tx.sign(alice_private_key, db.get_chain_id());
        SCORUM_REQUIRE_THROW(db.push_transaction(tx, 0), fc::assert_exception);

        BOOST_TEST_MESSAGE("--- successs cancelling a request");
        op.decline = false;
        tx.clear();
        tx.operations.push_back(op);
        tx.sign(alice_private_key, db.get_chain_id());
        db.push_transaction(tx, 0);

        itr = request_idx.find(db.obtain_service<dbs_account>().get_account("alice").id);
        BOOST_REQUIRE(itr == request_idx.end());

        BOOST_TEST_MESSAGE("--- failure cancelling a request that doesn't exist");
        generate_block();
        tx.clear();
        tx.operations.push_back(op);
        tx.set_expiration(db.head_block_time() + SCORUM_MAX_TIME_UNTIL_EXPIRATION);
        tx.sign(alice_private_key, db.get_chain_id());
        SCORUM_REQUIRE_THROW(db.push_transaction(tx, 0), fc::exception);

        BOOST_TEST_MESSAGE("--- check account can vote during waiting period");
        op.decline = true;
        tx.clear();
        tx.operations.push_back(op);
        tx.sign(alice_private_key, db.get_chain_id());
        db.push_transaction(tx, 0);

        generate_blocks(db.head_block_time() + SCORUM_OWNER_AUTH_RECOVERY_PERIOD - fc::seconds(SCORUM_BLOCK_INTERVAL),
                        true);
        BOOST_REQUIRE(db.obtain_service<dbs_account>().get_account("alice").can_vote);
        witness_create("alice", alice_private_key, "foo.bar", alice_private_key.get_public_key(), 0);

        account_witness_vote_operation witness_vote;
        witness_vote.account = "alice";
        witness_vote.witness = "alice";
        tx.clear();
        tx.operations.push_back(witness_vote);
        tx.set_expiration(db.head_block_time() + SCORUM_MAX_TIME_UNTIL_EXPIRATION);
        tx.sign(alice_private_key, db.get_chain_id());
        db.push_transaction(tx, 0);

        comment_operation comment;
        comment.author = "alice";
        comment.permlink = "test";
        comment.parent_permlink = "test";
        comment.title = "test";
        comment.body = "test";
        vote_operation vote;
        vote.voter = "alice";
        vote.author = "alice";
        vote.permlink = "test";
        vote.weight = (int16_t)100;
        tx.clear();
        tx.operations.push_back(comment);
        tx.operations.push_back(vote);
        tx.sign(alice_private_key, db.get_chain_id());
        db.push_transaction(tx, 0);
        validate_database();

        BOOST_TEST_MESSAGE("--- check account cannot vote after request is processed");
        generate_block();
        BOOST_REQUIRE(!db.obtain_service<dbs_account>().get_account("alice").can_vote);
        validate_database();

        itr = request_idx.find(db.obtain_service<dbs_account>().get_account("alice").id);
        BOOST_REQUIRE(itr == request_idx.end());

        const auto& witness_idx = db.get_index<witness_vote_index>().indices().get<by_account_witness>();
        auto witness_itr = witness_idx.find(boost::make_tuple(db.obtain_service<dbs_account>().get_account("alice").id,
                                                              db.obtain_service<dbs_witness>().get("alice").id));
        BOOST_REQUIRE(witness_itr == witness_idx.end());

        tx.clear();
        tx.set_expiration(db.head_block_time() + SCORUM_MAX_TIME_UNTIL_EXPIRATION);
        tx.operations.push_back(witness_vote);
        tx.sign(alice_private_key, db.get_chain_id());
        SCORUM_REQUIRE_THROW(db.push_transaction(tx, 0), fc::exception);

        db.get<comment_vote_object, by_comment_voter>(boost::make_tuple(
            db.get_comment("alice", string("test")).id, db.obtain_service<dbs_account>().get_account("alice").id));

        vote.weight = (int16_t)0;
        tx.clear();
        tx.operations.push_back(vote);
        tx.sign(alice_private_key, db.get_chain_id());
        SCORUM_REQUIRE_THROW(db.push_transaction(tx, 0), fc::exception);

        vote.weight = (int16_t)50;
        tx.clear();
        tx.operations.push_back(vote);
        tx.sign(alice_private_key, db.get_chain_id());
        SCORUM_REQUIRE_THROW(db.push_transaction(tx, 0), fc::exception);

        proxy.account = "alice";
        proxy.proxy = "bob";
        tx.clear();
        tx.operations.push_back(proxy);
        tx.sign(alice_private_key, db.get_chain_id());
        SCORUM_REQUIRE_THROW(db.push_transaction(tx, 0), fc::exception);
    }
    FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE(account_bandwidth)
{
    try
    {
        BOOST_TEST_MESSAGE("Testing: account_bandwidth");
        ACTORS((alice)(bob))
        generate_block();
        vest("alice", ASSET_SCR(10e+3));
        fund("alice", ASSET_SCR(10e+3));
        vest("bob", ASSET_SCR(10e+3));

        generate_block();

        BOOST_TEST_MESSAGE("--- Test first tx in block");

        signed_transaction tx;
        transfer_operation op;

        op.from = "alice";
        op.to = "bob";
        op.amount = ASSET_SCR(1e+3);

        tx.operations.push_back(op);
        tx.set_expiration(db.head_block_time() + SCORUM_MAX_TIME_UNTIL_EXPIRATION);
        tx.sign(alice_private_key, db.get_chain_id());

        db.push_transaction(tx, 0);

        auto last_bandwidth_update = db.get<witness::account_bandwidth_object, witness::by_account_bandwidth_type>(
                                           boost::make_tuple("alice", witness::bandwidth_type::market))
                                         .last_bandwidth_update;
        auto average_bandwidth = db.get<witness::account_bandwidth_object, witness::by_account_bandwidth_type>(
                                       boost::make_tuple("alice", witness::bandwidth_type::market))
                                     .average_bandwidth;
        BOOST_REQUIRE(last_bandwidth_update == db.head_block_time());
        BOOST_REQUIRE(average_bandwidth == fc::raw::pack_size(tx) * 10 * SCORUM_BANDWIDTH_PRECISION);
        auto total_bandwidth = average_bandwidth;

        BOOST_TEST_MESSAGE("--- Test second tx in block");

        op.amount = ASSET_SCR(100);
        tx.clear();
        tx.operations.push_back(op);
        tx.set_expiration(db.head_block_time() + SCORUM_MAX_TIME_UNTIL_EXPIRATION);
        tx.sign(alice_private_key, db.get_chain_id());

        db.push_transaction(tx, 0);

        last_bandwidth_update = db.get<witness::account_bandwidth_object, witness::by_account_bandwidth_type>(
                                      boost::make_tuple("alice", witness::bandwidth_type::market))
                                    .last_bandwidth_update;
        average_bandwidth = db.get<witness::account_bandwidth_object, witness::by_account_bandwidth_type>(
                                  boost::make_tuple("alice", witness::bandwidth_type::market))
                                .average_bandwidth;
        BOOST_REQUIRE(last_bandwidth_update == db.head_block_time());
        BOOST_REQUIRE(average_bandwidth == total_bandwidth + fc::raw::pack_size(tx) * 10 * SCORUM_BANDWIDTH_PRECISION);
    }
    FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE(account_create_with_delegation_authorities)
{
    try
    {
        BOOST_TEST_MESSAGE("Testing: account_create_with_delegation_authorities");

        signed_transaction tx;
        ACTORS((alice));

        vest("alice", ASSET_SCR(100e+3));
        generate_blocks(1);
        fund("alice", ASSET_SCR(1e+6));

        private_key_type priv_key = generate_private_key("temp_key");

        account_create_with_delegation_operation op;
        op.fee = SUFFICIENT_FEE;
        op.delegation = asset(100, VESTS_SYMBOL);
        op.creator = "alice";
        op.new_account_name = "bob";
        op.owner = authority(1, priv_key.get_public_key(), 1);
        op.active = authority(2, priv_key.get_public_key(), 2);
        op.memo_key = priv_key.get_public_key();
        op.json_metadata = "{\"foo\":\"bar\"}";

        tx.set_expiration(db.head_block_time() + SCORUM_MAX_TIME_UNTIL_EXPIRATION);
        tx.operations.push_back(op);

        BOOST_TEST_MESSAGE("--- Test failure when no signatures");
        SCORUM_REQUIRE_THROW(db.push_transaction(tx, 0), tx_missing_active_auth);

        BOOST_TEST_MESSAGE("--- Test success with witness signature");
        tx.sign(alice_private_key, db.get_chain_id());
        db.push_transaction(tx, 0);

        BOOST_TEST_MESSAGE("--- Test failure when duplicate signatures");
        tx.operations.clear();
        tx.signatures.clear();
        op.new_account_name = "sam";
        tx.operations.push_back(op);
        tx.sign(alice_private_key, db.get_chain_id());
        tx.sign(alice_private_key, db.get_chain_id());
        SCORUM_REQUIRE_THROW(db.push_transaction(tx, 0), tx_duplicate_sig);

        BOOST_TEST_MESSAGE("--- Test failure when signed by an additional signature not in the creator's authority");
        tx.signatures.clear();
        tx.sign(init_account_priv_key, db.get_chain_id());
        tx.sign(alice_private_key, db.get_chain_id());
        SCORUM_REQUIRE_THROW(db.push_transaction(tx, 0), tx_irrelevant_sig);

        BOOST_TEST_MESSAGE("--- Test failure when signed by a signature not in the creator's authority");
        tx.signatures.clear();
        tx.sign(init_account_priv_key, db.get_chain_id());
        SCORUM_REQUIRE_THROW(db.push_transaction(tx, 0), tx_missing_active_auth);

        validate_database();
    }
    FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE(account_create_with_delegation_apply)
{
    const asset to_delegate = ASSET_SP(100e+3);

    try
    {
        BOOST_TEST_MESSAGE("Testing: account_create_with_delegation_apply");
        signed_transaction tx;
        ACTORS((alice));
        // 150 * fee = ( 5 * SCR ) + SP
        generate_blocks(1);
        fund("alice", ASSET_SCR(151e+4));
        vest("alice", ASSET_SCR(1e+6));

        const auto& account_service = db.account_service();

        const account_object& alice_vested = account_service.get_account("alice");
        BOOST_REQUIRE_GE(alice_vested.vesting_shares, to_delegate);

        private_key_type priv_key = generate_private_key("temp_key");

        generate_block();

        const auto new_account_creation_fee = ASSET_SCR(1e+3);

        db_plugin->debug_update(
            [=](database& db) {
                db.modify(db.get_dynamic_global_properties(), [&](dynamic_global_property_object& dgpo) {
                    dgpo.median_chain_props.account_creation_fee = new_account_creation_fee;
                });
            },
            default_skip);

        generate_block();

        BOOST_TEST_MESSAGE("--- Test failure when SP are powering down.");
        withdraw_vesting_operation withdraw;
        withdraw.account = "alice";
        withdraw.vesting_shares = alice_vested.vesting_shares;
        account_create_with_delegation_operation op;
        op.fee = new_account_creation_fee * SCORUM_CREATE_ACCOUNT_WITH_SCORUM_MODIFIER;
        op.delegation = to_delegate;
        op.creator = "alice";
        op.new_account_name = "bob";
        op.owner = authority(1, priv_key.get_public_key(), 1);
        op.active = authority(2, priv_key.get_public_key(), 2);
        op.memo_key = priv_key.get_public_key();
        op.json_metadata = "{\"foo\":\"bar\"}";
        tx.operations.push_back(withdraw);
        tx.operations.push_back(op);
        tx.set_expiration(db.head_block_time() + SCORUM_MAX_TIME_UNTIL_EXPIRATION);
        tx.sign(alice_private_key, db.get_chain_id());
        SCORUM_REQUIRE_THROW(db.push_transaction(tx, 0), fc::assert_exception);

        BOOST_TEST_MESSAGE("--- Test success under normal conditions. ");
        tx.clear();
        tx.operations.push_back(op);
        tx.sign(alice_private_key, db.get_chain_id());
        db.push_transaction(tx, 0);

        const account_object& bob_acc = account_service.get_account("bob");
        const account_object& alice_acc = account_service.get_account("alice");
        BOOST_REQUIRE_EQUAL(alice_acc.delegated_vesting_shares, to_delegate);
        BOOST_REQUIRE_EQUAL(bob_acc.received_vesting_shares, to_delegate);
        BOOST_REQUIRE_EQUAL(bob_acc.effective_vesting_shares(),
                            bob_acc.vesting_shares - bob_acc.delegated_vesting_shares
                                + bob_acc.received_vesting_shares);

        BOOST_TEST_MESSAGE("--- Test delegator object integrety. ");
        auto delegation
            = db.find<vesting_delegation_object, by_delegation>(boost::make_tuple(op.creator, op.new_account_name));

        BOOST_REQUIRE(delegation != nullptr);
        BOOST_REQUIRE_EQUAL(delegation->delegator, op.creator);
        BOOST_REQUIRE_EQUAL(delegation->delegatee, op.new_account_name);
        BOOST_REQUIRE_EQUAL(delegation->vesting_shares, to_delegate);
        BOOST_REQUIRE(delegation->min_delegation_time == db.head_block_time() + SCORUM_CREATE_ACCOUNT_DELEGATION_TIME);
        auto del_amt = delegation->vesting_shares;
        auto exp_time = delegation->min_delegation_time;

        generate_block();

        BOOST_TEST_MESSAGE("--- Test success using only SCR to reach target delegation.");

        tx.clear();
        op.fee = asset(db.get_dynamic_global_properties().median_chain_props.account_creation_fee.amount
                           * SCORUM_CREATE_ACCOUNT_WITH_SCORUM_MODIFIER * SCORUM_CREATE_ACCOUNT_DELEGATION_RATIO,
                       SCORUM_SYMBOL);
        op.delegation = asset(0, VESTS_SYMBOL);
        op.new_account_name = "sam";
        tx.set_expiration(db.head_block_time() + SCORUM_MAX_TIME_UNTIL_EXPIRATION);
        tx.operations.push_back(op);
        tx.sign(alice_private_key, db.get_chain_id());
        db.push_transaction(tx, 0);

        BOOST_TEST_MESSAGE("--- Test failure when insufficient funds to process transaction.");
        tx.clear();
        op.fee = SUFFICIENT_FEE;
        op.delegation = ASSET_NULL_SP;
        op.new_account_name = "pam";
        tx.set_expiration(db.head_block_time() + SCORUM_MAX_TIME_UNTIL_EXPIRATION);
        tx.operations.push_back(op);
        tx.sign(alice_private_key, db.get_chain_id());

        SCORUM_REQUIRE_THROW(db.push_transaction(tx, 0), fc::exception);

        BOOST_TEST_MESSAGE("--- Test failure when insufficient fee fo reach target delegation.");
        fund("alice", asset(db.get_dynamic_global_properties().median_chain_props.account_creation_fee.amount
                                * SCORUM_CREATE_ACCOUNT_WITH_SCORUM_MODIFIER * SCORUM_CREATE_ACCOUNT_DELEGATION_RATIO,
                            SCORUM_SYMBOL));
        SCORUM_REQUIRE_THROW(db.push_transaction(tx, 0), fc::exception);

        validate_database();

        BOOST_TEST_MESSAGE("--- Test removing delegation from new account");
        tx.clear();
        delegate_vesting_shares_operation delegate;
        delegate.delegator = "alice";
        delegate.delegatee = "bob";
        delegate.vesting_shares = ASSET_NULL_SP;
        tx.operations.push_back(delegate);
        tx.sign(alice_private_key, db.get_chain_id());
        db.push_transaction(tx, 0);

        auto itr = db.get_index<vesting_delegation_expiration_index, by_id>().begin();
        auto end = db.get_index<vesting_delegation_expiration_index, by_id>().end();

        BOOST_REQUIRE(itr != end);
        BOOST_REQUIRE(itr->delegator == "alice");
        BOOST_REQUIRE(itr->vesting_shares == del_amt);
        BOOST_REQUIRE(itr->expiration == exp_time);
        validate_database();
    }
    FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE(delegate_vesting_shares_validate)
{
    try
    {
        delegate_vesting_shares_operation op;

        op.delegator = "alice";
        op.delegatee = "bob";
        op.vesting_shares = asset(-1, VESTS_SYMBOL);
        SCORUM_REQUIRE_THROW(op.validate(), fc::assert_exception);
    }
    FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE(delegate_vesting_shares_authorities)
{
    asset to_delegate = ASSET_SP(300e+3);

    try
    {
        BOOST_TEST_MESSAGE("Testing: delegate_vesting_shares_authorities");
        signed_transaction tx;
        ACTORS((alice)(bob))

        vest("alice", ASSET_SCR(1e+6));

        generate_block();

        const auto& account_service = db.account_service();

        const account_object& alice_vested = account_service.get_account("alice");
        BOOST_REQUIRE_GE(alice_vested.vesting_shares, to_delegate);

        delegate_vesting_shares_operation op;
        op.vesting_shares = to_delegate;
        op.delegator = "alice";
        op.delegatee = "bob";

        tx.set_expiration(db.head_block_time() + SCORUM_MAX_TIME_UNTIL_EXPIRATION);
        tx.operations.push_back(op);

        BOOST_TEST_MESSAGE("--- Test failure when no signatures");
        SCORUM_REQUIRE_THROW(db.push_transaction(tx, 0), tx_missing_active_auth);

        BOOST_TEST_MESSAGE("--- Test success with witness signature");
        tx.sign(alice_private_key, db.get_chain_id());
        db.push_transaction(tx, 0);

        BOOST_TEST_MESSAGE("--- Test failure when duplicate signatures");
        tx.operations.clear();
        tx.signatures.clear();
        op.delegatee = "sam";
        tx.operations.push_back(op);
        tx.sign(alice_private_key, db.get_chain_id());
        tx.sign(alice_private_key, db.get_chain_id());
        SCORUM_REQUIRE_THROW(db.push_transaction(tx, 0), tx_duplicate_sig);

        BOOST_TEST_MESSAGE("--- Test failure when signed by an additional signature not in the creator's authority");
        tx.signatures.clear();
        tx.sign(init_account_priv_key, db.get_chain_id());
        tx.sign(alice_private_key, db.get_chain_id());
        SCORUM_REQUIRE_THROW(db.push_transaction(tx, 0), tx_irrelevant_sig);

        BOOST_TEST_MESSAGE("--- Test failure when signed by a signature not in the creator's authority");
        tx.signatures.clear();
        tx.sign(init_account_priv_key, db.get_chain_id());
        SCORUM_REQUIRE_THROW(db.push_transaction(tx, 0), tx_missing_active_auth);
        validate_database();
    }
    FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE(delegate_vesting_shares_apply)
{
    const asset to_delegate = ASSET_SP(10e+6);

    try
    {
        BOOST_TEST_MESSAGE("Testing: delegate_vesting_shares_apply");
        signed_transaction tx;
        ACTORS((alice)(bob))
        generate_block();

        vest("alice", ASSET_SCR(1e+9));
        vest("bob", ASSET_SCR(100e+3));

        generate_block();

        db_plugin->debug_update(
            [=](database& db) {
                db.modify(db.get_dynamic_global_properties(), [&](dynamic_global_property_object& dgpo) {
                    dgpo.median_chain_props.account_creation_fee = ASSET_SCR(1e+3);
                });
            },
            default_skip);

        generate_block();

        delegate_vesting_shares_operation op;
        op.vesting_shares = to_delegate;
        op.delegator = "alice";
        op.delegatee = "bob";

        tx.set_expiration(db.head_block_time() + SCORUM_MAX_TIME_UNTIL_EXPIRATION);
        tx.operations.push_back(op);
        tx.sign(alice_private_key, db.get_chain_id());
        db.push_transaction(tx, 0);
        generate_blocks(1);
        const account_object& alice_acc = db.obtain_service<dbs_account>().get_account("alice");
        const account_object& bob_acc = db.obtain_service<dbs_account>().get_account("bob");

        BOOST_REQUIRE_EQUAL(alice_acc.delegated_vesting_shares, to_delegate);
        BOOST_REQUIRE_EQUAL(bob_acc.received_vesting_shares, to_delegate);

        BOOST_TEST_MESSAGE("--- Test that the delegation object is correct. ");
        auto delegation
            = db.find<vesting_delegation_object, by_delegation>(boost::make_tuple(op.delegator, op.delegatee));

        BOOST_REQUIRE(delegation != nullptr);
        BOOST_REQUIRE_EQUAL(delegation->delegator, op.delegator);
        BOOST_REQUIRE_EQUAL(delegation->vesting_shares, to_delegate);

        validate_database();
        tx.clear();
        op.vesting_shares = to_delegate * 2;
        tx.set_expiration(db.head_block_time() + SCORUM_MAX_TIME_UNTIL_EXPIRATION);
        tx.operations.push_back(op);
        tx.sign(alice_private_key, db.get_chain_id());
        db.push_transaction(tx, 0);
        generate_blocks(1);

        BOOST_REQUIRE(delegation != nullptr);
        BOOST_REQUIRE_EQUAL(delegation->delegator, op.delegator);
        BOOST_REQUIRE_EQUAL(delegation->vesting_shares, to_delegate * 2);
        BOOST_REQUIRE_EQUAL(alice_acc.delegated_vesting_shares, to_delegate * 2);
        BOOST_REQUIRE_EQUAL(bob_acc.received_vesting_shares, to_delegate * 2);

        BOOST_TEST_MESSAGE("--- Test that effective vesting shares is accurate and being applied.");
        tx.operations.clear();
        tx.signatures.clear();

        comment_operation comment_op;
        comment_op.author = "alice";
        comment_op.permlink = "foo";
        comment_op.parent_permlink = "test";
        comment_op.title = "bar";
        comment_op.body = "foo bar";
        tx.operations.push_back(comment_op);
        tx.set_expiration(db.head_block_time() + SCORUM_MAX_TIME_UNTIL_EXPIRATION);
        tx.sign(alice_private_key, db.get_chain_id());
        db.push_transaction(tx, 0);
        tx.signatures.clear();
        tx.operations.clear();
        vote_operation vote_op;
        vote_op.voter = "bob";
        vote_op.author = "alice";
        vote_op.permlink = "foo";
        vote_op.weight = (int16_t)100;
        tx.set_expiration(db.head_block_time() + SCORUM_MAX_TIME_UNTIL_EXPIRATION);
        tx.operations.push_back(vote_op);
        tx.sign(bob_private_key, db.get_chain_id());
        auto old_voting_power = bob_acc.voting_power;

        db.push_transaction(tx, 0);
        generate_blocks(1);

        const auto& vote_idx = db.get_index<comment_vote_index>().indices().get<by_comment_voter>();

        auto& alice_comment = db.get_comment("alice", string("foo"));
        auto itr = vote_idx.find(std::make_tuple(alice_comment.id, bob_acc.id));
        BOOST_REQUIRE_EQUAL(alice_comment.net_rshares.value,
                            bob_acc.effective_vesting_shares().amount.value * (old_voting_power - bob_acc.voting_power)
                                / SCORUM_100_PERCENT);
        BOOST_REQUIRE_EQUAL(itr->rshares,
                            bob_acc.effective_vesting_shares().amount.value * (old_voting_power - bob_acc.voting_power)
                                / SCORUM_100_PERCENT);

        generate_block();
        ACTORS((sam)(dave))
        generate_block();

        vest("sam", ASSET_SCR(1e+9));

        generate_block();

        auto sam_vest = db.obtain_service<dbs_account>().get_account("sam").vesting_shares;

        BOOST_TEST_MESSAGE("--- Test failure when delegating 0 SP");
        tx.clear();
        op.delegator = "sam";
        op.delegatee = "dave";
        tx.set_expiration(db.head_block_time() + SCORUM_MAX_TIME_UNTIL_EXPIRATION);
        tx.sign(sam_private_key, db.get_chain_id());
        SCORUM_REQUIRE_THROW(db.push_transaction(tx), fc::assert_exception);

        BOOST_TEST_MESSAGE("--- Testing failure delegating more vesting shares than account has.");
        tx.clear();
        op.vesting_shares = asset(sam_vest.amount + 1, VESTS_SYMBOL);
        tx.operations.push_back(op);
        tx.sign(sam_private_key, db.get_chain_id());
        SCORUM_REQUIRE_THROW(db.push_transaction(tx), fc::assert_exception);

        BOOST_TEST_MESSAGE("--- Test failure delegating vesting shares that are part of a power down");
        tx.clear();
        sam_vest = asset(sam_vest.amount / 2, VESTS_SYMBOL);
        withdraw_vesting_operation withdraw;
        withdraw.account = "sam";
        withdraw.vesting_shares = sam_vest;
        tx.operations.push_back(withdraw);
        tx.sign(sam_private_key, db.get_chain_id());
        db.push_transaction(tx, 0);

        tx.clear();
        op.vesting_shares = asset(sam_vest.amount + 2, VESTS_SYMBOL);
        tx.operations.push_back(op);
        tx.sign(sam_private_key, db.get_chain_id());
        SCORUM_REQUIRE_THROW(db.push_transaction(tx), fc::assert_exception);

        tx.clear();
        withdraw.vesting_shares = ASSET_NULL_SP;
        tx.operations.push_back(withdraw);
        tx.sign(sam_private_key, db.get_chain_id());
        db.push_transaction(tx, 0);

        BOOST_TEST_MESSAGE("--- Test failure powering down vesting shares that are delegated");
        sam_vest.amount += 1000;
        op.vesting_shares = sam_vest;
        tx.clear();
        tx.operations.push_back(op);
        tx.sign(sam_private_key, db.get_chain_id());
        db.push_transaction(tx, 0);

        tx.clear();
        withdraw.vesting_shares = asset(sam_vest.amount, VESTS_SYMBOL);
        tx.operations.push_back(withdraw);
        tx.sign(sam_private_key, db.get_chain_id());
        SCORUM_REQUIRE_THROW(db.push_transaction(tx), fc::assert_exception);

        BOOST_TEST_MESSAGE("--- Remove a delegation and ensure it is returned after 1 week");
        tx.clear();
        op.vesting_shares = ASSET_NULL_SP;
        tx.operations.push_back(op);
        tx.sign(sam_private_key, db.get_chain_id());
        db.push_transaction(tx, 0);

        auto exp_obj = db.get_index<vesting_delegation_expiration_index, by_id>().begin();
        auto end = db.get_index<vesting_delegation_expiration_index, by_id>().end();

        BOOST_REQUIRE(exp_obj != end);
        BOOST_REQUIRE(exp_obj->delegator == "sam");
        BOOST_REQUIRE(exp_obj->vesting_shares == sam_vest);
        BOOST_REQUIRE(exp_obj->expiration == db.head_block_time() + SCORUM_CASHOUT_WINDOW_SECONDS);
        BOOST_REQUIRE(db.obtain_service<dbs_account>().get_account("sam").delegated_vesting_shares == sam_vest);
        BOOST_REQUIRE(db.obtain_service<dbs_account>().get_account("dave").received_vesting_shares == ASSET_SP(0));
        delegation = db.find<vesting_delegation_object, by_delegation>(boost::make_tuple(op.delegator, op.delegatee));
        BOOST_REQUIRE(delegation == nullptr);

        generate_blocks(exp_obj->expiration + SCORUM_BLOCK_INTERVAL);

        exp_obj = db.get_index<vesting_delegation_expiration_index, by_id>().begin();
        end = db.get_index<vesting_delegation_expiration_index, by_id>().end();

        BOOST_REQUIRE(exp_obj == end);
        BOOST_REQUIRE_EQUAL(db.obtain_service<dbs_account>().get_account("sam").delegated_vesting_shares,
                            ASSET_NULL_SP);
    }
    FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE(issue_971_vesting_removal)
{
    const asset to_delegate = ASSET_SP(10e+6);

    // This is a regression test specifically for issue #971
    try
    {
        BOOST_TEST_MESSAGE("Test Issue 971 Vesting Removal");
        ACTORS((alice)(bob))
        generate_block();

        vest("alice", ASSET_SCR(1e+9));

        generate_block();

        db_plugin->debug_update(
            [=](database& db) {
                db.modify(db.get_dynamic_global_properties(), [&](dynamic_global_property_object& dgpo) {
                    dgpo.median_chain_props.account_creation_fee = ASSET_SCR(1e+3);
                });
            },
            default_skip);

        generate_block();

        signed_transaction tx;
        delegate_vesting_shares_operation op;
        op.vesting_shares = to_delegate;
        op.delegator = "alice";
        op.delegatee = "bob";

        tx.set_expiration(db.head_block_time() + SCORUM_MAX_TIME_UNTIL_EXPIRATION);
        tx.operations.push_back(op);
        tx.sign(alice_private_key, db.get_chain_id());
        db.push_transaction(tx, 0);
        generate_block();
        const account_object& alice_acc = db.obtain_service<dbs_account>().get_account("alice");
        const account_object& bob_acc = db.obtain_service<dbs_account>().get_account("bob");

        BOOST_REQUIRE_EQUAL(alice_acc.delegated_vesting_shares, to_delegate);
        BOOST_REQUIRE_EQUAL(bob_acc.received_vesting_shares, to_delegate);

        generate_block();

        db_plugin->debug_update([=](database& db) {
            db.modify(db.get_dynamic_global_properties(), [&](dynamic_global_property_object& dgpo) {
                dgpo.median_chain_props.account_creation_fee = ASSET_SCR(100e+6);
            });
        });

        generate_block();

        op.vesting_shares = ASSET_NULL_SP;

        tx.clear();
        tx.operations.push_back(op);
        tx.sign(alice_private_key, db.get_chain_id());
        db.push_transaction(tx, 0);
        generate_block();

        BOOST_REQUIRE_EQUAL(alice_acc.delegated_vesting_shares, to_delegate);
        BOOST_REQUIRE_EQUAL(bob_acc.received_vesting_shares, ASSET_NULL_SP);
    }
    FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE(comment_beneficiaries_validate)
{
    try
    {
        BOOST_TEST_MESSAGE("Test Comment Beneficiaries Validate");
        comment_options_operation op;

        op.author = "alice";
        op.permlink = "test";

        BOOST_TEST_MESSAGE("--- Testing more than 100% weight on a single route");
        comment_payout_beneficiaries b;
        b.beneficiaries.push_back(beneficiary_route_type(account_name_type("bob"), SCORUM_100_PERCENT + 1));
        op.extensions.insert(b);
        SCORUM_REQUIRE_THROW(op.validate(), fc::assert_exception);

        BOOST_TEST_MESSAGE("--- Testing more than 100% total weight");
        b.beneficiaries.clear();
        b.beneficiaries.push_back(beneficiary_route_type(account_name_type("bob"), SCORUM_1_PERCENT * 75));
        b.beneficiaries.push_back(beneficiary_route_type(account_name_type("sam"), SCORUM_1_PERCENT * 75));
        op.extensions.clear();
        op.extensions.insert(b);
        SCORUM_REQUIRE_THROW(op.validate(), fc::assert_exception);

        BOOST_TEST_MESSAGE("--- Testing maximum number of routes");
        b.beneficiaries.clear();
        for (size_t i = 0; i < 127; i++)
        {
            b.beneficiaries.push_back(beneficiary_route_type(account_name_type("foo" + fc::to_string(i)), 1));
        }

        op.extensions.clear();
        std::sort(b.beneficiaries.begin(), b.beneficiaries.end());
        op.extensions.insert(b);
        op.validate();

        BOOST_TEST_MESSAGE("--- Testing one too many routes");
        b.beneficiaries.push_back(beneficiary_route_type(account_name_type("bar"), 1));
        std::sort(b.beneficiaries.begin(), b.beneficiaries.end());
        op.extensions.clear();
        op.extensions.insert(b);
        SCORUM_REQUIRE_THROW(op.validate(), fc::assert_exception);

        BOOST_TEST_MESSAGE("--- Testing duplicate accounts");
        b.beneficiaries.clear();
        b.beneficiaries.push_back(beneficiary_route_type("bob", SCORUM_1_PERCENT * 2));
        b.beneficiaries.push_back(beneficiary_route_type("bob", SCORUM_1_PERCENT));
        op.extensions.clear();
        op.extensions.insert(b);
        SCORUM_REQUIRE_THROW(op.validate(), fc::assert_exception);

        BOOST_TEST_MESSAGE("--- Testing incorrect account sort order");
        b.beneficiaries.clear();
        b.beneficiaries.push_back(beneficiary_route_type("bob", SCORUM_1_PERCENT));
        b.beneficiaries.push_back(beneficiary_route_type("alice", SCORUM_1_PERCENT));
        op.extensions.clear();
        op.extensions.insert(b);
        SCORUM_REQUIRE_THROW(op.validate(), fc::assert_exception);

        BOOST_TEST_MESSAGE("--- Testing correct account sort order");
        b.beneficiaries.clear();
        b.beneficiaries.push_back(beneficiary_route_type("alice", SCORUM_1_PERCENT));
        b.beneficiaries.push_back(beneficiary_route_type("bob", SCORUM_1_PERCENT));
        op.extensions.clear();
        op.extensions.insert(b);
        op.validate();
    }
    FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_SUITE_END()

#endif
