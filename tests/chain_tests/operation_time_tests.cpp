#ifdef IS_TEST_NET
#include <boost/test/unit_test.hpp>

#include <scorum/protocol/exceptions.hpp>

#include <scorum/chain/schema/block_summary_object.hpp>
#include <scorum/chain/database.hpp>
#include <scorum/chain/hardfork.hpp>
#include <scorum/chain/schema/history_objects.hpp>
#include <scorum/chain/schema/scorum_objects.hpp>
#include <scorum/chain/services/account.hpp>

#include <scorum/chain/util/reward.hpp>

#include <scorum/plugins/debug_node/debug_node_plugin.hpp>

#include <fc/crypto/digest.hpp>
#include "database_default_integration.hpp"

#include <cmath>

using namespace scorum;
using namespace scorum::chain;
using namespace scorum::protocol;

BOOST_FIXTURE_TEST_SUITE(operation_time_tests, database_default_integration_fixture)

BOOST_AUTO_TEST_CASE(comment_payout_equalize)
{
    try
    {
        ACTORS((alice)(bob)(dave)(ulysses)(vivian)(wendy))

        struct author_actor
        {
            author_actor(const std::string& n,
                         fc::ecc::private_key pk,
                         fc::optional<asset> mpay = fc::optional<asset>())
                : name(n)
                , private_key(pk)
                , max_accepted_payout(mpay)
            {
            }
            std::string name;
            fc::ecc::private_key private_key;
            fc::optional<asset> max_accepted_payout;
        };

        struct voter_actor
        {
            voter_actor(const std::string& n, fc::ecc::private_key pk, std::string fa)
                : name(n)
                , private_key(pk)
                , favorite_author(fa)
            {
            }
            std::string name;
            fc::ecc::private_key private_key;
            std::string favorite_author;
        };

        std::vector<author_actor> authors;
        std::vector<voter_actor> voters;

        authors.emplace_back("alice", alice_private_key);
        authors.emplace_back("bob", bob_private_key, ASSET_SCR(0));
        authors.emplace_back("dave", dave_private_key);
        voters.emplace_back("ulysses", ulysses_private_key, "alice");
        voters.emplace_back("vivian", vivian_private_key, "bob");
        voters.emplace_back("wendy", wendy_private_key, "dave");

        // A,B,D : posters
        // U,V,W : voters

        // SCORUM: we don't have stable coin but might have an threshold
        // set a ridiculously high SCR price ($1 / satoshi) to disable dust threshold

        for (const auto& voter : voters)
        {
            fund(voter.name, ASSET_SCR(10e+3));
            vest(voter.name, ASSET_SCR(100e+3));
        }

        // authors all write in the same block, but Bob declines payout
        for (const auto& author : authors)
        {
            signed_transaction tx;
            comment_operation com;
            com.author = author.name;
            com.permlink = "mypost";
            com.parent_author = SCORUM_ROOT_POST_PARENT_ACCOUNT;
            com.parent_permlink = "test";
            com.title = "Hello from " + author.name;
            com.body = "Hello, my name is " + author.name;
            tx.operations.push_back(com);

            if (author.max_accepted_payout.valid())
            {
                comment_options_operation copt;
                copt.author = com.author;
                copt.permlink = com.permlink;
                copt.max_accepted_payout = *(author.max_accepted_payout);
                tx.operations.push_back(copt);
            }

            tx.set_expiration(db.head_block_time() + SCORUM_MAX_TIME_UNTIL_EXPIRATION);
            tx.sign(author.private_key, db.get_chain_id());
            db.push_transaction(tx, 0);
        }

        generate_blocks(1);

        // voters all vote in the same block with the same stake
        for (const auto& voter : voters)
        {
            signed_transaction tx;
            vote_operation vote;
            vote.voter = voter.name;
            vote.author = voter.favorite_author;
            vote.permlink = "mypost";
            vote.weight = (int16_t)100;
            tx.operations.push_back(vote);
            tx.set_expiration(db.head_block_time() + SCORUM_MAX_TIME_UNTIL_EXPIRATION);
            tx.sign(voter.private_key, db.get_chain_id());
            db.push_transaction(tx, 0);
        }

        //        auto reward_scorum = db.get_dynamic_global_properties().total_reward_fund_scorum;

        // generate a few blocks to seed the reward fund
        generate_blocks(10);
        // const auto& rf = db.get< reward_fund_object, by_name >( SCORUM_POST_REWARD_FUND_NAME );
        // idump( (rf) );

        generate_blocks(db.get_comment("alice", std::string("mypost")).cashout_time, true);
        /*
        for( const auto& author : authors )
        {
           const account_object& a = db.obtain_service<dbs_account>().get_account(author.name);
           ilog( "${n} : ${scorum} ${sbd}", ("n", author.name)("scorum", a.reward_scorum_balance)("sbd",
        a.reward_sbd_balance) );
        }
        for( const auto& voter : voters )
        {
           const account_object& a = db.obtain_service<dbs_account>().get_account(voter.name);
           ilog( "${n} : ${scorum} ${sbd}", ("n", voter.name)("scorum", a.reward_scorum_balance)("sbd",
        a.reward_sbd_balance) );
        }
        */

        // SCORUM: rewrite to check SCR reward
        //        const account_object& alice_account = db.obtain_service<dbs_account>().get_account("alice");
        //        const account_object& bob_account = db.obtain_service<dbs_account>().get_account("bob");
        //        const account_object& dave_account = db.obtain_service<dbs_account>().get_account("dave");

        //        BOOST_CHECK( alice_account.reward_sbd_balance == ASSET( "14288.000 TBD" ) );
        //        BOOST_CHECK( alice_account.reward_sbd_balance == ASSET( "13967.000 TBD" ) );
        //        BOOST_CHECK( bob_account.reward_sbd_balance == ASSET( "0.000 TBD" ) );
        //        BOOST_CHECK( dave_account.reward_sbd_balance == alice_account.reward_sbd_balance );
    }
    FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE(comment_payout_dust)
{
    try
    {
        BOOST_TEST_MESSAGE("Testing: comment_payout_dust");

        ACTORS((alice)(bob))
        generate_block();

        vest("alice", ASSET_SCR(100e+3));
        vest("bob", ASSET_SCR(100e+3));

        generate_block();
        validate_database();

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
        vote.weight = (int16_t)81;

        signed_transaction tx;
        tx.operations.push_back(comment);
        tx.operations.push_back(vote);
        tx.set_expiration(db.head_block_time() + SCORUM_MAX_TIME_UNTIL_EXPIRATION);
        tx.sign(alice_private_key, db.get_chain_id());
        db.push_transaction(tx, 0);
        validate_database();

        comment.author = "bob";
        vote.voter = "bob";
        vote.author = "bob";
        vote.weight = (int16_t)59;

        tx.clear();
        tx.operations.push_back(comment);
        tx.operations.push_back(vote);
        tx.sign(bob_private_key, db.get_chain_id());
        db.push_transaction(tx, 0);
        validate_database();

        generate_blocks(db.get_comment("alice", std::string("test")).cashout_time);

        // If comments are paid out independent of order, then the last satoshi of SCR cannot be divided among them
        const auto& rf = db.get_reward_fund();
        BOOST_REQUIRE_EQUAL(rf.reward_balance, ASSET_SCR(1));

        validate_database();

        BOOST_TEST_MESSAGE("Done");
    }
    FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE(reward_fund)
{
    try
    {
        BOOST_TEST_MESSAGE("Testing: reward_fund");

        ACTORS((alice)(bob))

        vest("alice", ASSET_SCR(100e+3));
        vest("bob", ASSET_SCR(100e+3));

        generate_block();

        asset account_initial_vest_supply = db.obtain_service<dbs_account>().get_account("alice").vesting_shares;

        const auto blocks_between_comments = 5;

        BOOST_REQUIRE_EQUAL(db.obtain_service<dbs_account>().get_account("alice").balance, asset(0, SCORUM_SYMBOL));
        BOOST_REQUIRE_EQUAL(db.obtain_service<dbs_account>().get_account("bob").balance, asset(0, SCORUM_SYMBOL));
        BOOST_REQUIRE_EQUAL(db.obtain_service<dbs_account>().get_account("bob").vesting_shares,
                            account_initial_vest_supply);

        comment_operation comment;
        vote_operation vote;
        signed_transaction tx;

        comment.author = "alice";
        comment.permlink = "test";
        comment.parent_permlink = "test";
        comment.title = "foo";
        comment.body = "bar";
        vote.voter = "alice";
        vote.author = "alice";
        vote.permlink = "test";
        vote.weight = (int16_t)100;
        tx.operations.push_back(comment);
        tx.operations.push_back(vote);
        tx.set_expiration(db.head_block_time() + SCORUM_MAX_TIME_UNTIL_EXPIRATION);
        tx.sign(alice_private_key, db.get_chain_id());
        db.push_transaction(tx, 0);

        generate_blocks(blocks_between_comments);

        comment.author = "bob";
        comment.parent_author = "alice";
        vote.voter = "bob";
        vote.author = "bob";
        tx.clear();
        tx.operations.push_back(comment);
        tx.operations.push_back(vote);
        tx.sign(bob_private_key, db.get_chain_id());
        db.push_transaction(tx, 0);

        const auto& fund = db.get_reward_fund();

        BOOST_REQUIRE_GT(fund.reward_balance, asset(0, SCORUM_SYMBOL));
        BOOST_REQUIRE_EQUAL(fund.recent_claims.to_uint64(), uint64_t(0));

        share_type alice_comment_net_rshares = db.get_comment("alice", std::string("test")).net_rshares;
        share_type bob_comment_net_rshares = db.get_comment("bob", std::string("test")).net_rshares;

        {
            generate_blocks(db.get_comment("alice", std::string("test")).cashout_time);

            BOOST_REQUIRE_EQUAL(fund.reward_balance, ASSET_SCR(0));
            BOOST_REQUIRE_EQUAL(fund.recent_claims.to_uint64(), alice_comment_net_rshares);

            // clang-format off
            BOOST_REQUIRE_GT   (db.obtain_service<dbs_account>().get_account("alice").vesting_shares, account_initial_vest_supply);
            BOOST_REQUIRE_EQUAL(db.obtain_service<dbs_account>().get_account("alice").vesting_shares,
                                account_initial_vest_supply + ASSET_SP(db.obtain_service<dbs_account>().get_account("alice").balance.amount.value));

            BOOST_REQUIRE_EQUAL(db.obtain_service<dbs_account>().get_account("bob").vesting_shares, account_initial_vest_supply);
            BOOST_REQUIRE_EQUAL(db.obtain_service<dbs_account>().get_account("bob").vesting_shares,
                                account_initial_vest_supply + ASSET_SP(db.obtain_service<dbs_account>().get_account("bob").balance.amount.value));
            // clang-format on

            validate_database();
        }

        {
            generate_blocks(blocks_between_comments);

            // clang-format off
            for (auto i=0; i<blocks_between_comments; ++i)
            {
                alice_comment_net_rshares -= alice_comment_net_rshares * SCORUM_BLOCK_INTERVAL / SCORUM_RECENT_RSHARES_DECAY_RATE.to_seconds();
            }
            BOOST_REQUIRE_EQUAL(fund.recent_claims.to_uint64(), alice_comment_net_rshares + bob_comment_net_rshares);
            BOOST_REQUIRE_GT(fund.reward_balance, ASSET_SCR(0));

            BOOST_REQUIRE_GT(db.obtain_service<dbs_account>().get_account("alice").vesting_shares, account_initial_vest_supply);
            BOOST_REQUIRE_GT(db.obtain_service<dbs_account>().get_account("bob").vesting_shares, account_initial_vest_supply);
            // clang-format on

            validate_database();
        }
    }
    FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE(recent_claims_decay)
{
    try
    {
        BOOST_TEST_MESSAGE("Testing: recent_rshares_2decay");
        ACTORS((alice)(bob))

        vest("alice", ASSET_SCR(100e+3));
        vest("bob", ASSET_SCR(100e+3));

        generate_block();

        comment_operation comment;
        vote_operation vote;
        signed_transaction tx;

        comment.author = "alice";
        comment.permlink = "test";
        comment.parent_permlink = "test";
        comment.title = "foo";
        comment.body = "bar";
        vote.voter = "alice";
        vote.author = "alice";
        vote.permlink = "test";
        vote.weight = (int16_t)100;
        tx.operations.push_back(comment);
        tx.operations.push_back(vote);
        tx.set_expiration(db.head_block_time() + SCORUM_MAX_TIME_UNTIL_EXPIRATION);
        tx.sign(alice_private_key, db.get_chain_id());
        db.push_transaction(tx, 0);

        auto alice_vshares = util::evaluate_reward_curve(db.get_comment("alice", std::string("test")).net_rshares.value,
                                                         db.get_reward_fund().author_reward_curve);

        generate_blocks(5);

        comment.author = "bob";
        vote.voter = "bob";
        vote.author = "bob";
        tx.clear();
        tx.operations.push_back(comment);
        tx.operations.push_back(vote);
        tx.sign(bob_private_key, db.get_chain_id());
        db.push_transaction(tx, 0);

        generate_blocks(db.get_comment("alice", std::string("test")).cashout_time);

        {
            const auto& post_rf = db.get_reward_fund();

            BOOST_REQUIRE(post_rf.recent_claims == alice_vshares);
            validate_database();
        }

        auto bob_cashout_time = db.get_comment("bob", std::string("test")).cashout_time;
        auto bob_vshares = util::evaluate_reward_curve(db.get_comment("bob", std::string("test")).net_rshares.value,
                                                       db.get_reward_fund().author_reward_curve);

        generate_block();

        while (db.head_block_time() < bob_cashout_time)
        {
            alice_vshares -= (alice_vshares * SCORUM_BLOCK_INTERVAL) / SCORUM_RECENT_RSHARES_DECAY_RATE.to_seconds();
            const auto& post_rf = db.get_reward_fund();

            BOOST_REQUIRE(post_rf.recent_claims == alice_vshares);

            generate_block();
        }

        {
            alice_vshares -= (alice_vshares * SCORUM_BLOCK_INTERVAL) / SCORUM_RECENT_RSHARES_DECAY_RATE.to_seconds();
            const auto& post_rf = db.get_reward_fund();

            BOOST_REQUIRE(post_rf.recent_claims == alice_vshares + bob_vshares);
            validate_database();
        }
    }
    FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE(vesting_withdrawals)
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

BOOST_AUTO_TEST_CASE(vesting_withdraw_route)
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

BOOST_AUTO_TEST_CASE(post_rate_limit)
{
    try
    {
        ACTORS((alice))

        fund("alice", ASSET_SCR(10e+3));
        vest("alice", ASSET_SCR(100e+3));

        comment_operation op;
        op.author = "alice";
        op.permlink = "test1";
        op.parent_author = "";
        op.parent_permlink = "test";
        op.body = "test";

        signed_transaction tx;

        tx.operations.push_back(op);
        tx.set_expiration(db.head_block_time() + SCORUM_MAX_TIME_UNTIL_EXPIRATION);
        tx.sign(alice_private_key, db.get_chain_id());
        db.push_transaction(tx, 0);

        BOOST_REQUIRE(db.get_comment("alice", std::string("test1")).reward_weight == SCORUM_100_PERCENT);

        tx.operations.clear();
        tx.signatures.clear();

        generate_blocks(db.head_block_time() + SCORUM_MIN_ROOT_COMMENT_INTERVAL + fc::seconds(SCORUM_BLOCK_INTERVAL),
                        true);

        op.permlink = "test2";

        tx.operations.push_back(op);
        tx.sign(alice_private_key, db.get_chain_id());
        db.push_transaction(tx, 0);

        BOOST_REQUIRE(db.get_comment("alice", std::string("test2")).reward_weight == SCORUM_100_PERCENT);

        generate_blocks(db.head_block_time() + SCORUM_MIN_ROOT_COMMENT_INTERVAL + fc::seconds(SCORUM_BLOCK_INTERVAL),
                        true);

        tx.operations.clear();
        tx.signatures.clear();

        op.permlink = "test3";

        tx.operations.push_back(op);
        tx.sign(alice_private_key, db.get_chain_id());
        db.push_transaction(tx, 0);

        BOOST_REQUIRE(db.get_comment("alice", std::string("test3")).reward_weight == SCORUM_100_PERCENT);

        generate_blocks(db.head_block_time() + SCORUM_MIN_ROOT_COMMENT_INTERVAL + fc::seconds(SCORUM_BLOCK_INTERVAL),
                        true);

        tx.operations.clear();
        tx.signatures.clear();

        op.permlink = "test4";

        tx.operations.push_back(op);
        tx.sign(alice_private_key, db.get_chain_id());
        db.push_transaction(tx, 0);

        BOOST_REQUIRE(db.get_comment("alice", std::string("test4")).reward_weight == SCORUM_100_PERCENT);

        generate_blocks(db.head_block_time() + SCORUM_MIN_ROOT_COMMENT_INTERVAL + fc::seconds(SCORUM_BLOCK_INTERVAL),
                        true);

        tx.operations.clear();
        tx.signatures.clear();

        op.permlink = "test5";

        tx.operations.push_back(op);
        tx.sign(alice_private_key, db.get_chain_id());
        db.push_transaction(tx, 0);

        BOOST_REQUIRE(db.get_comment("alice", std::string("test5")).reward_weight == SCORUM_100_PERCENT);
    }
    FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE(comment_freeze)
{
    try
    {
        ACTORS((alice)(bob)(sam)(dave))
        fund("alice", ASSET_SCR(10e+3));
        fund("bob", ASSET_SCR(10e+3));
        fund("sam", ASSET_SCR(10e+3));
        fund("dave", ASSET_SCR(10e+3));

        vest("alice", ASSET_SCR(100e+3));
        vest("bob", ASSET_SCR(100e+3));
        vest("sam", ASSET_SCR(100e+3));
        vest("dave", ASSET_SCR(100e+3));

        signed_transaction tx;

        comment_operation comment;
        comment.author = "alice";
        comment.parent_author = "";
        comment.permlink = "test";
        comment.parent_permlink = "test";
        comment.body = "test";

        tx.operations.push_back(comment);
        tx.set_expiration(db.head_block_time() + SCORUM_MAX_TIME_UNTIL_EXPIRATION);
        tx.sign(alice_private_key, db.get_chain_id());
        db.push_transaction(tx, 0);

        comment.body = "test2";

        tx.operations.clear();
        tx.signatures.clear();

        tx.operations.push_back(comment);
        tx.sign(alice_private_key, db.get_chain_id());
        db.push_transaction(tx, 0);

        vote_operation vote;
        vote.weight = (int16_t)100;
        vote.voter = "bob";
        vote.author = "alice";
        vote.permlink = "test";

        tx.operations.clear();
        tx.signatures.clear();

        tx.operations.push_back(vote);
        tx.sign(bob_private_key, db.get_chain_id());
        db.push_transaction(tx, 0);

        BOOST_REQUIRE(db.get_comment("alice", std::string("test")).last_payout == fc::time_point_sec::min());
        BOOST_REQUIRE(db.get_comment("alice", std::string("test")).cashout_time != fc::time_point_sec::min());
        BOOST_REQUIRE(db.get_comment("alice", std::string("test")).cashout_time != fc::time_point_sec::maximum());

        generate_blocks(db.get_comment("alice", std::string("test")).cashout_time, true);

        BOOST_REQUIRE(db.get_comment("alice", std::string("test")).last_payout == db.head_block_time());
        BOOST_REQUIRE(db.get_comment("alice", std::string("test")).cashout_time == fc::time_point_sec::maximum());

        vote.voter = "sam";

        tx.operations.clear();
        tx.signatures.clear();

        tx.operations.push_back(vote);
        tx.set_expiration(db.head_block_time() + SCORUM_MAX_TIME_UNTIL_EXPIRATION);
        tx.sign(sam_private_key, db.get_chain_id());
        db.push_transaction(tx, 0);

        BOOST_REQUIRE(db.get_comment("alice", std::string("test")).cashout_time == fc::time_point_sec::maximum());
        BOOST_REQUIRE(db.get_comment("alice", std::string("test")).net_rshares.value == 0);
        BOOST_REQUIRE(db.get_comment("alice", std::string("test")).abs_rshares.value == 0);

        vote.voter = "bob";
        vote.weight = (int16_t)100 * -1;

        tx.operations.clear();
        tx.signatures.clear();

        tx.operations.push_back(vote);
        tx.set_expiration(db.head_block_time() + SCORUM_MAX_TIME_UNTIL_EXPIRATION);
        tx.sign(bob_private_key, db.get_chain_id());
        db.push_transaction(tx, 0);

        BOOST_REQUIRE(db.get_comment("alice", std::string("test")).cashout_time == fc::time_point_sec::maximum());
        BOOST_REQUIRE(db.get_comment("alice", std::string("test")).net_rshares.value == 0);
        BOOST_REQUIRE(db.get_comment("alice", std::string("test")).abs_rshares.value == 0);

        vote.voter = "dave";
        vote.weight = (int16_t)0;

        tx.operations.clear();
        tx.signatures.clear();

        tx.operations.push_back(vote);
        tx.set_expiration(db.head_block_time() + SCORUM_MAX_TIME_UNTIL_EXPIRATION);
        tx.sign(dave_private_key, db.get_chain_id());

        db.push_transaction(tx, 0);
        BOOST_REQUIRE(db.get_comment("alice", std::string("test")).cashout_time == fc::time_point_sec::maximum());
        BOOST_REQUIRE(db.get_comment("alice", std::string("test")).net_rshares.value == 0);
        BOOST_REQUIRE(db.get_comment("alice", std::string("test")).abs_rshares.value == 0);

        comment.body = "test4";

        tx.operations.clear();
        tx.signatures.clear();

        tx.operations.push_back(comment);
        tx.sign(alice_private_key, db.get_chain_id());
        SCORUM_REQUIRE_THROW(db.push_transaction(tx, 0), fc::exception);
    }
    FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_SUITE_END()
#endif
