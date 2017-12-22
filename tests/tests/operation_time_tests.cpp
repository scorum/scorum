#ifdef IS_TEST_NET
#include <boost/test/unit_test.hpp>

#include <scorum/protocol/exceptions.hpp>

#include <scorum/chain/block_summary_object.hpp>
#include <scorum/chain/database.hpp>
#include <scorum/chain/hardfork.hpp>
#include <scorum/chain/history_object.hpp>
#include <scorum/chain/scorum_objects.hpp>

#include <scorum/chain/util/reward.hpp>

#include <scorum/plugins/debug_node/debug_node_plugin.hpp>

#include <fc/crypto/digest.hpp>

#include "database_fixture.hpp"

#include <cmath>

using namespace scorum;
using namespace scorum::chain;
using namespace scorum::protocol;

BOOST_FIXTURE_TEST_SUITE(operation_time_tests, clean_database_fixture)

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
        authors.emplace_back("bob", bob_private_key, ASSET("0.000 TESTS"));
        authors.emplace_back("dave", dave_private_key);
        voters.emplace_back("ulysses", ulysses_private_key, "alice");
        voters.emplace_back("vivian", vivian_private_key, "bob");
        voters.emplace_back("wendy", wendy_private_key, "dave");

        // A,B,D : posters
        // U,V,W : voters

        // SCORUM: we don't have stable coin but might have an threshold
        // set a ridiculously high SCORUM price ($1 / satoshi) to disable dust threshold
        // set_price_feed( price( ASSET( "0.001 TESTS" ), ASSET( "1.000 TBD" ) ) );

        for (const auto& voter : voters)
        {
            fund(voter.name, 10000);
            vest(voter.name, 10000);
        }

        // authors all write in the same block, but Bob declines payout
        for (const auto& author : authors)
        {
            signed_transaction tx;
            comment_operation com;
            com.author = author.name;
            com.permlink = "mypost";
            com.parent_author = SCORUM_ROOT_POST_PARENT;
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
            vote.weight = SCORUM_100_PERCENT;
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
           const account_object& a = db.get_account(author.name);
           ilog( "${n} : ${scorum} ${sbd}", ("n", author.name)("scorum", a.reward_scorum_balance)("sbd",
        a.reward_sbd_balance) );
        }
        for( const auto& voter : voters )
        {
           const account_object& a = db.get_account(voter.name);
           ilog( "${n} : ${scorum} ${sbd}", ("n", voter.name)("scorum", a.reward_scorum_balance)("sbd",
        a.reward_sbd_balance) );
        }
        */

        // SCORUM: rewrite to check SCR reward
        //        const account_object& alice_account = db.get_account("alice");
        //        const account_object& bob_account = db.get_account("bob");
        //        const account_object& dave_account = db.get_account("dave");

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

        vest("alice", ASSET("10.000 TESTS"));
        vest("bob", ASSET("10.000 TESTS"));

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
        vote.weight = 81 * SCORUM_1_PERCENT;

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
        vote.weight = 59 * SCORUM_1_PERCENT;

        tx.clear();
        tx.operations.push_back(comment);
        tx.operations.push_back(vote);
        tx.sign(bob_private_key, db.get_chain_id());
        db.push_transaction(tx, 0);
        validate_database();

        generate_blocks(db.get_comment("alice", std::string("test")).cashout_time);

        // If comments are paid out independent of order, then the last satoshi of SCORUM cannot be divided among them
        const auto rf = db.get<reward_fund_object, by_name>(SCORUM_POST_REWARD_FUND_NAME);
        BOOST_REQUIRE(rf.reward_balance == ASSET("0.001 TESTS"));

        validate_database();

        BOOST_TEST_MESSAGE("Done");
    }
    FC_LOG_AND_RETHROW()
}

/*
BOOST_AUTO_TEST_CASE( reward_funds )
{
   try
   {
      BOOST_TEST_MESSAGE( "Testing: reward_funds" );

      ACTORS( (alice)(bob) )
      generate_block();

      set_price_feed( price( ASSET( "1.000 TESTS" ), ASSET( "1.000 TBD" ) ) );
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
      vote.weight = SCORUM_100_PERCENT;
      tx.operations.push_back( comment );
      tx.operations.push_back( vote );
      tx.set_expiration( db.head_block_time() + SCORUM_MAX_TIME_UNTIL_EXPIRATION );
      tx.sign( alice_private_key, db.get_chain_id() );
      db.push_transaction( tx, 0 );

      generate_blocks( 5 );

      comment.author = "bob";
      comment.parent_author = "alice";
      vote.voter = "bob";
      vote.author = "bob";
      tx.clear();
      tx.operations.push_back( comment );
      tx.operations.push_back( vote );
      tx.sign( bob_private_key, db.get_chain_id() );
      db.push_transaction( tx, 0 );

      generate_blocks( db.get_comment( "alice", std::string( "test" ) ).cashout_time );

      {
         const auto& post_rf = db.get< reward_fund_object, by_name >( SCORUM_POST_REWARD_FUND_NAME );
         const auto& comment_rf = db.get< reward_fund_object, by_name >( SCORUM_COMMENT_REWARD_FUND_NAME );

         BOOST_REQUIRE( post_rf.reward_balance.amount == 0 );
         BOOST_REQUIRE( comment_rf.reward_balance.amount > 0 );
         BOOST_REQUIRE( db.get_account( "alice" ).reward_sbd_balance.amount > 0 );
         BOOST_REQUIRE( db.get_account( "bob" ).reward_sbd_balance.amount == 0 );
         validate_database();
      }

      generate_blocks( db.get_comment( "bob", std::string( "test" ) ).cashout_time );

      {
         const auto& post_rf = db.get< reward_fund_object, by_name >( SCORUM_POST_REWARD_FUND_NAME );
         const auto& comment_rf = db.get< reward_fund_object, by_name >( SCORUM_COMMENT_REWARD_FUND_NAME );

         BOOST_REQUIRE( post_rf.reward_balance.amount > 0 );
         BOOST_REQUIRE( comment_rf.reward_balance.amount == 0 );
         BOOST_REQUIRE( db.get_account( "alice" ).reward_sbd_balance.amount > 0 );
         BOOST_REQUIRE( db.get_account( "bob" ).reward_sbd_balance.amount > 0 );
         validate_database();
      }
   }
   FC_LOG_AND_RETHROW()
}
*/

BOOST_AUTO_TEST_CASE(recent_claims_decay)
{
    try
    {
        BOOST_TEST_MESSAGE("Testing: recent_rshares_2decay");
        ACTORS((alice)(bob))
        generate_block();

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
        vote.weight = SCORUM_100_PERCENT;
        tx.operations.push_back(comment);
        tx.operations.push_back(vote);
        tx.set_expiration(db.head_block_time() + SCORUM_MAX_TIME_UNTIL_EXPIRATION);
        tx.sign(alice_private_key, db.get_chain_id());
        db.push_transaction(tx, 0);

        auto alice_vshares = util::evaluate_reward_curve(
            db.get_comment("alice", std::string("test")).net_rshares.value,
            db.get<reward_fund_object, by_name>(SCORUM_POST_REWARD_FUND_NAME).author_reward_curve);

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
            const auto& post_rf = db.get<reward_fund_object, by_name>(SCORUM_POST_REWARD_FUND_NAME);

            BOOST_REQUIRE(post_rf.recent_claims == alice_vshares);
            validate_database();
        }

        auto bob_cashout_time = db.get_comment("bob", std::string("test")).cashout_time;
        auto bob_vshares = util::evaluate_reward_curve(
            db.get_comment("bob", std::string("test")).net_rshares.value,
            db.get<reward_fund_object, by_name>(SCORUM_POST_REWARD_FUND_NAME).author_reward_curve);

        generate_block();

        while (db.head_block_time() < bob_cashout_time)
        {
            alice_vshares -= (alice_vshares * SCORUM_BLOCK_INTERVAL) / SCORUM_RECENT_RSHARES_DECAY_RATE.to_seconds();
            const auto& post_rf = db.get<reward_fund_object, by_name>(SCORUM_POST_REWARD_FUND_NAME);

            BOOST_REQUIRE(post_rf.recent_claims == alice_vshares);

            generate_block();
        }

        {
            alice_vshares -= (alice_vshares * SCORUM_BLOCK_INTERVAL) / SCORUM_RECENT_RSHARES_DECAY_RATE.to_seconds();
            const auto& post_rf = db.get<reward_fund_object, by_name>(SCORUM_POST_REWARD_FUND_NAME);

            BOOST_REQUIRE(post_rf.recent_claims == alice_vshares + bob_vshares);
            validate_database();
        }
    }
    FC_LOG_AND_RETHROW()
}

/*BOOST_AUTO_TEST_CASE( comment_payout )
{
   try
   {
      ACTORS( (alice)(bob)(sam)(dave) )
      fund( "alice", 10000 );
      vest( "alice", 10000 );
      fund( "bob", 7500 );
      vest( "bob", 7500 );
      fund( "sam", 8000 );
      vest( "sam", 8000 );
      fund( "dave", 5000 );
      vest( "dave", 5000 );

      price exchange_rate( ASSET( "1.000 TESTS" ), ASSET( "1.000 TBD" ) );
      set_price_feed( exchange_rate );

      signed_transaction tx;

      BOOST_TEST_MESSAGE( "Creating comments." );

      comment_operation com;
      com.author = "alice";
      com.permlink = "test";
      com.parent_author = "";
      com.parent_permlink = "test";
      com.title = "foo";
      com.body = "bar";
      tx.operations.push_back( com );
      tx.set_expiration( db.head_block_time() + SCORUM_MAX_TIME_UNTIL_EXPIRATION );
      tx.sign( alice_private_key, db.get_chain_id() );
      db.push_transaction( tx, 0 );

      tx.operations.clear();
      tx.signatures.clear();

      com.author = "bob";
      tx.operations.push_back( com );
      tx.sign( bob_private_key, db.get_chain_id() );
      db.push_transaction( tx, 0 );

      tx.operations.clear();
      tx.signatures.clear();

      BOOST_TEST_MESSAGE( "Voting for comments." );

      vote_operation vote;
      vote.voter = "alice";
      vote.author = "alice";
      vote.permlink = "test";
      vote.weight = SCORUM_100_PERCENT;
      tx.operations.push_back( vote );
      tx.sign( alice_private_key, db.get_chain_id() );
      db.push_transaction( tx, 0 );

      tx.operations.clear();
      tx.signatures.clear();

      vote.voter = "sam";
      vote.author = "alice";
      tx.operations.push_back( vote );
      tx.sign( sam_private_key, db.get_chain_id() );
      db.push_transaction( tx, 0 );

      tx.operations.clear();
      tx.signatures.clear();

      vote.voter = "bob";
      vote.author = "bob";
      tx.operations.push_back( vote );
      tx.sign( bob_private_key, db.get_chain_id() );
      db.push_transaction( tx, 0 );

      tx.operations.clear();
      tx.signatures.clear();

      vote.voter = "dave";
      vote.author = "bob";
      tx.operations.push_back( vote );
      tx.sign( dave_private_key, db.get_chain_id() );
      db.push_transaction( tx, 0 );

      BOOST_TEST_MESSAGE( "Generate blocks up until first payout" );

      //generate_blocks( db.get_comment( "bob", std::string( "test" ) ).cashout_time - SCORUM_BLOCK_INTERVAL, true );

      auto reward_scorum = db.get_dynamic_global_properties().total_reward_fund_scorum + ASSET( "1.667 TESTS" );
      auto total_rshares2 = db.get_dynamic_global_properties().total_reward_shares2;
      auto bob_comment_rshares = db.get_comment( "bob", std::string( "test" ) ).net_rshares;
      auto bob_vest_shares = db.get_account( "bob" ).vesting_shares;
      auto bob_sbd_balance = db.get_account( "bob" ).sbd_balance;

      auto bob_comment_payout = asset( ( ( uint128_t( bob_comment_rshares.value ) * bob_comment_rshares.value *
reward_scorum.amount.value ) / total_rshares2 ).to_uint64(), SCORUM_SYMBOL );
      auto bob_comment_discussion_rewards = asset( bob_comment_payout.amount / 4, SCORUM_SYMBOL );
      bob_comment_payout -= bob_comment_discussion_rewards;
      auto bob_comment_sbd_reward = db.to_sbd( asset( bob_comment_payout.amount / 2, SCORUM_SYMBOL ) );
      auto bob_comment_vesting_reward = ( bob_comment_payout - asset( bob_comment_payout.amount / 2, SCORUM_SYMBOL) ) *
db.get_dynamic_global_properties().get_vesting_share_price();

      BOOST_TEST_MESSAGE( "Cause first payout" );

      generate_block();

      BOOST_REQUIRE( db.get_dynamic_global_properties().total_reward_fund_scorum == reward_scorum - bob_comment_payout
);
      BOOST_REQUIRE( db.get_comment( "bob", std::string( "test" ) ).total_payout_value == bob_comment_vesting_reward *
db.get_dynamic_global_properties().get_vesting_share_price() + bob_comment_sbd_reward * exchange_rate );
      BOOST_REQUIRE( db.get_account( "bob" ).vesting_shares == bob_vest_shares + bob_comment_vesting_reward );
      BOOST_REQUIRE( db.get_account( "bob" ).sbd_balance == bob_sbd_balance + bob_comment_sbd_reward );

      BOOST_TEST_MESSAGE( "Testing no payout when less than $0.02" );

      tx.operations.clear();
      tx.signatures.clear();
      vote.voter = "alice";
      vote.author = "alice";
      tx.operations.push_back( vote );
      tx.set_expiration( db.head_block_time() + SCORUM_MAX_TIME_UNTIL_EXPIRATION );
      tx.sign( alice_private_key, db.get_chain_id() );
      db.push_transaction( tx, 0 );

      tx.operations.clear();
      tx.signatures.clear();
      vote.voter = "dave";
      vote.author = "bob";
      vote.weight = SCORUM_1_PERCENT;
      tx.operations.push_back( vote );
      tx.sign( dave_private_key, db.get_chain_id() );
      db.push_transaction( tx, 0 );

      generate_blocks( db.get_comment( "bob", std::string( "test" ) ).cashout_time - SCORUM_BLOCK_INTERVAL, true );

      tx.operations.clear();
      tx.signatures.clear();
      vote.voter = "bob";
      vote.author = "alice";
      vote.weight = SCORUM_100_PERCENT;
      tx.operations.push_back( vote );
      tx.set_expiration( db.head_block_time() + SCORUM_MAX_TIME_UNTIL_EXPIRATION );
      tx.sign( bob_private_key, db.get_chain_id() );
      db.push_transaction( tx, 0 );

      tx.operations.clear();
      tx.signatures.clear();
      vote.voter = "sam";
      tx.operations.push_back( vote );
      tx.sign( sam_private_key, db.get_chain_id() );
      db.push_transaction( tx, 0 );

      tx.operations.clear();
      tx.signatures.clear();
      vote.voter = "dave";
      tx.operations.push_back( vote );
      tx.sign( dave_private_key, db.get_chain_id() );
      db.push_transaction( tx, 0 );

      bob_vest_shares = db.get_account( "bob" ).vesting_shares;
      bob_sbd_balance = db.get_account( "bob" ).sbd_balance;

      validate_database();

      generate_block();

      BOOST_REQUIRE( bob_vest_shares.amount.value == db.get_account( "bob" ).vesting_shares.amount.value );
      BOOST_REQUIRE( bob_sbd_balance.amount.value == db.get_account( "bob" ).sbd_balance.amount.value );
      validate_database();
   }
   FC_LOG_AND_RETHROW()
}*/

/*
BOOST_AUTO_TEST_CASE( comment_payout )
{
   try
   {
      ACTORS( (alice)(bob)(sam)(dave) )
      fund( "alice", 10000 );
      vest( "alice", 10000 );
      fund( "bob", 7500 );
      vest( "bob", 7500 );
      fund( "sam", 8000 );
      vest( "sam", 8000 );
      fund( "dave", 5000 );
      vest( "dave", 5000 );

      price exchange_rate( ASSET( "1.000 TESTS" ), ASSET( "1.000 TBD" ) );
      set_price_feed( exchange_rate );

      auto gpo = db.get_dynamic_global_properties();

      signed_transaction tx;

      BOOST_TEST_MESSAGE( "Creating comments. " );

      comment_operation com;
      com.author = "alice";
      com.permlink = "test";
      com.parent_permlink = "test";
      com.title = "foo";
      com.body = "bar";
      tx.operations.push_back( com );
      tx.set_expiration( db.head_block_time() + SCORUM_MAX_TIME_UNTIL_EXPIRATION );
      tx.sign( alice_private_key, db.get_chain_id() );
      db.push_transaction( tx, 0 );

      tx.operations.clear();
      tx.signatures.clear();
      com.author = "bob";
      tx.operations.push_back( com );
      tx.sign( bob_private_key, db.get_chain_id() );
      db.push_transaction( tx, 0 );

      BOOST_TEST_MESSAGE( "First round of votes." );

      tx.operations.clear();
      tx.signatures.clear();
      vote_operation vote;
      vote.voter = "alice";
      vote.author = "alice";
      vote.permlink = "test";
      vote.weight = SCORUM_100_PERCENT;
      tx.operations.push_back( vote );
      tx.sign( alice_private_key, db.get_chain_id() );
      db.push_transaction( tx, 0 );

      tx.operations.clear();
      tx.signatures.clear();
      vote.voter = "dave";
      tx.operations.push_back( vote );
      tx.sign( dave_private_key, db.get_chain_id() );
      db.push_transaction( tx, 0 );

      tx.operations.clear();
      tx.signatures.clear();
      vote.voter = "bob";
      vote.author = "bob";
      tx.operations.push_back( vote );
      tx.sign( bob_private_key, db.get_chain_id() );
      db.push_transaction( tx, 0 );

      tx.operations.clear();
      tx.signatures.clear();
      vote.voter = "sam";
      tx.operations.push_back( vote );
      tx.sign( sam_private_key, db.get_chain_id() );
      db.push_transaction( tx, 0 );

      BOOST_TEST_MESSAGE( "Generating blocks..." );

      generate_blocks( fc::time_point_sec( db.head_block_time().sec_since_epoch() + SCORUM_CASHOUT_WINDOW_SECONDS / 2 ),
true );

      BOOST_TEST_MESSAGE( "Second round of votes." );

      tx.operations.clear();
      tx.signatures.clear();
      vote.voter = "alice";
      vote.author = "bob";
      tx.set_expiration( db.head_block_time() + SCORUM_MAX_TIME_UNTIL_EXPIRATION );
      tx.operations.push_back( vote );
      tx.sign( alice_private_key, db.get_chain_id() );
      db.push_transaction( tx, 0 );

      tx.operations.clear();
      tx.signatures.clear();
      vote.voter = "bob";
      vote.author = "alice";
      tx.operations.push_back( vote );
      tx.sign( bob_private_key, db.get_chain_id() );
      db.push_transaction( tx, 0 );

      tx.operations.clear();
      tx.signatures.clear();
      vote.voter = "sam";
      tx.operations.push_back( vote );
      tx.sign( sam_private_key, db.get_chain_id() );
      db.push_transaction( tx, 0 );

      BOOST_TEST_MESSAGE( "Generating more blocks..." );

      generate_blocks( db.get_comment( "bob", std::string( "test" ) ).cashout_time - ( SCORUM_BLOCK_INTERVAL / 2 ), true
);

      BOOST_TEST_MESSAGE( "Check comments have not been paid out." );

      const auto& vote_idx = db.get_index< comment_vote_index >().indices().get< by_comment_voter >();

      BOOST_REQUIRE( vote_idx.find( std::make_tuple( db.get_comment( "alice", std::string( "test" ).id, db.get_account(
"alice" ) ).id ) ) != vote_idx.end() );
      BOOST_REQUIRE( vote_idx.find( std::make_tuple( db.get_comment( "alice", std::string( "test" ).id, db.get_account(
"bob" ) ).id   ) ) != vote_idx.end() ); BOOST_REQUIRE( vote_idx.find( std::make_tuple( db.get_comment( "alice",
std::string( "test" ).id, db.get_account( "sam" ) ).id   ) ) != vote_idx.end() ); BOOST_REQUIRE( vote_idx.find(
std::make_tuple( db.get_comment( "alice", std::string( "test" ).id, db.get_account( "dave" ) ).id  ) ) != vote_idx.end()
); BOOST_REQUIRE( vote_idx.find( std::make_tuple( db.get_comment( "bob", std::string( "test" ).id,   db.get_account(
"alice" ) ).id ) ) != vote_idx.end() );
      BOOST_REQUIRE( vote_idx.find( std::make_tuple( db.get_comment( "bob", std::string( "test" ).id,   db.get_account(
"bob" ) ).id   ) ) != vote_idx.end() ); BOOST_REQUIRE( vote_idx.find( std::make_tuple( db.get_comment( "bob",
std::string( "test" ).id,   db.get_account( "sam" ) ).id   ) ) != vote_idx.end() ); BOOST_REQUIRE( db.get_comment(
"alice", std::string( "test" ) ).net_rshares.value > 0 ); BOOST_REQUIRE( db.get_comment( "bob", std::string( "test" )
).net_rshares.value > 0 ); validate_database();

      auto reward_scorum = db.get_dynamic_global_properties().total_reward_fund_scorum + ASSET( "2.000 TESTS" );
      auto total_rshares2 = db.get_dynamic_global_properties().total_reward_shares2;
      auto bob_comment_vote_total = db.get_comment( "bob", std::string( "test" ) ).total_vote_weight;
      auto bob_comment_rshares = db.get_comment( "bob", std::string( "test" ) ).net_rshares;
      auto bob_sbd_balance = db.get_account( "bob" ).sbd_balance;
      auto alice_vest_shares = db.get_account( "alice" ).vesting_shares;
      auto bob_vest_shares = db.get_account( "bob" ).vesting_shares;
      auto sam_vest_shares = db.get_account( "sam" ).vesting_shares;
      auto dave_vest_shares = db.get_account( "dave" ).vesting_shares;

      auto bob_comment_payout = asset( ( ( uint128_t( bob_comment_rshares.value ) * bob_comment_rshares.value *
reward_scorum.amount.value ) / total_rshares2 ).to_uint64(), SCORUM_SYMBOL );
      auto bob_comment_vote_rewards = asset( bob_comment_payout.amount / 2, SCORUM_SYMBOL );
      bob_comment_payout -= bob_comment_vote_rewards;
      auto bob_comment_sbd_reward = asset( bob_comment_payout.amount / 2, SCORUM_SYMBOL ) * exchange_rate;
      auto bob_comment_vesting_reward = ( bob_comment_payout - asset( bob_comment_payout.amount / 2, SCORUM_SYMBOL ) ) *
db.get_dynamic_global_properties().get_vesting_share_price();
      auto unclaimed_payments = bob_comment_vote_rewards;
      auto alice_vote_reward = asset( static_cast< uint64_t >( ( u256( vote_idx.find( std::make_tuple( db.get_comment(
"bob", std::string( "test" ).id, db.get_account( "alice" ) ).id ) )->weight ) * bob_comment_vote_rewards.amount.value )
/ bob_comment_vote_total ), SCORUM_SYMBOL ); auto alice_vote_vesting = alice_vote_reward *
db.get_dynamic_global_properties().get_vesting_share_price(); auto bob_vote_reward = asset( static_cast< uint64_t >( (
u256( vote_idx.find( std::make_tuple( db.get_comment( "bob", std::string( "test" ).id, db.get_account( "bob" ) ).id )
)->weight ) * bob_comment_vote_rewards.amount.value ) / bob_comment_vote_total ), SCORUM_SYMBOL ); auto bob_vote_vesting
= bob_vote_reward * db.get_dynamic_global_properties().get_vesting_share_price(); auto sam_vote_reward = asset(
static_cast< uint64_t >( ( u256( vote_idx.find( std::make_tuple( db.get_comment( "bob", std::string( "test" ).id,
db.get_account( "sam" ) ).id ) )->weight ) * bob_comment_vote_rewards.amount.value ) / bob_comment_vote_total ),
SCORUM_SYMBOL ); auto sam_vote_vesting = sam_vote_reward * db.get_dynamic_global_properties().get_vesting_share_price();
      unclaimed_payments -= ( alice_vote_reward + bob_vote_reward + sam_vote_reward );

      BOOST_TEST_MESSAGE( "Generate one block to cause a payout" );

      generate_block();

      auto bob_comment_reward = get_last_operations( 1 )[0].get< comment_reward_operation >();

      BOOST_REQUIRE( db.get_dynamic_global_properties().total_reward_fund_scorum.amount.value ==
reward_scorum.amount.value - ( bob_comment_payout + bob_comment_vote_rewards - unclaimed_payments ).amount.value );
      BOOST_REQUIRE( db.get_comment( "bob", std::string( "test" ) ).total_payout_value.amount.value == ( (
bob_comment_vesting_reward * db.get_dynamic_global_properties().get_vesting_share_price() ) + ( bob_comment_sbd_reward *
exchange_rate ) ).amount.value );
      BOOST_REQUIRE( db.get_account( "bob" ).sbd_balance.amount.value == ( bob_sbd_balance + bob_comment_sbd_reward
).amount.value );
      BOOST_REQUIRE( db.get_comment( "alice", std::string( "test" ) ).net_rshares.value > 0 );
      BOOST_REQUIRE( db.get_comment( "bob", std::string( "test" ) ).net_rshares.value == 0 );
      BOOST_REQUIRE( db.get_account( "alice" ).vesting_shares.amount.value == ( alice_vest_shares + alice_vote_vesting
).amount.value );
      BOOST_REQUIRE( db.get_account( "bob" ).vesting_shares.amount.value == ( bob_vest_shares + bob_vote_vesting +
bob_comment_vesting_reward ).amount.value );
      BOOST_REQUIRE( db.get_account( "sam" ).vesting_shares.amount.value == ( sam_vest_shares + sam_vote_vesting
).amount.value );
      BOOST_REQUIRE( db.get_account( "dave" ).vesting_shares.amount.value == dave_vest_shares.amount.value );
      BOOST_REQUIRE( bob_comment_reward.author == "bob" );
      BOOST_REQUIRE( bob_comment_reward.permlink == "test" );
      BOOST_REQUIRE( bob_comment_reward.payout.amount.value == bob_comment_sbd_reward.amount.value );
      BOOST_REQUIRE( bob_comment_reward.vesting_payout.amount.value == bob_comment_vesting_reward.amount.value );
      BOOST_REQUIRE( vote_idx.find( std::make_tuple( db.get_comment( "alice", std::string( "test" ).id, db.get_account(
"alice" ) ).id ) ) != vote_idx.end() );
      BOOST_REQUIRE( vote_idx.find( std::make_tuple( db.get_comment( "alice", std::string( "test" ).id, db.get_account(
"bob" ) ).id   ) ) != vote_idx.end() ); BOOST_REQUIRE( vote_idx.find( std::make_tuple( db.get_comment( "alice",
std::string( "test" ).id, db.get_account( "sam" ) ).id   ) ) != vote_idx.end() ); BOOST_REQUIRE( vote_idx.find(
std::make_tuple( db.get_comment( "alice", std::string( "test" ).id, db.get_account( "dave" ) ).id  ) ) != vote_idx.end()
); BOOST_REQUIRE( vote_idx.find( std::make_tuple( db.get_comment( "bob", std::string( "test" ).id,   db.get_account(
"alice" ) ).id ) ) == vote_idx.end() );
      BOOST_REQUIRE( vote_idx.find( std::make_tuple( db.get_comment( "bob", std::string( "test" ).id,   db.get_account(
"bob" ) ).id   ) ) == vote_idx.end() ); BOOST_REQUIRE( vote_idx.find( std::make_tuple( db.get_comment( "bob",
std::string( "test" ).id,   db.get_account( "sam" ) ).id   ) ) == vote_idx.end() ); validate_database();

      BOOST_TEST_MESSAGE( "Generating blocks up to next comment payout" );

      generate_blocks( db.get_comment( "alice", std::string( "test" ) ).cashout_time - ( SCORUM_BLOCK_INTERVAL / 2 ),
true );

      BOOST_REQUIRE( vote_idx.find( std::make_tuple( db.get_comment( "alice", std::string( "test" ).id, db.get_account(
"alice" ) ).id ) ) != vote_idx.end() );
      BOOST_REQUIRE( vote_idx.find( std::make_tuple( db.get_comment( "alice", std::string( "test" ).id, db.get_account(
"bob" ) ).id   ) ) != vote_idx.end() ); BOOST_REQUIRE( vote_idx.find( std::make_tuple( db.get_comment( "alice",
std::string( "test" ).id, db.get_account( "sam" ) ).id   ) ) != vote_idx.end() ); BOOST_REQUIRE( vote_idx.find(
std::make_tuple( db.get_comment( "alice", std::string( "test" ).id, db.get_account( "dave" ) ).id  ) ) != vote_idx.end()
); BOOST_REQUIRE( vote_idx.find( std::make_tuple( db.get_comment( "bob", std::string( "test" ).id,   db.get_account(
"alice" ) ).id ) ) == vote_idx.end() );
      BOOST_REQUIRE( vote_idx.find( std::make_tuple( db.get_comment( "bob", std::string( "test" ).id,   db.get_account(
"bob" ) ).id   ) ) == vote_idx.end() ); BOOST_REQUIRE( vote_idx.find( std::make_tuple( db.get_comment( "bob",
std::string( "test" ).id,   db.get_account( "sam" ) ).id   ) ) == vote_idx.end() ); BOOST_REQUIRE( db.get_comment(
"alice", std::string( "test" ) ).net_rshares.value > 0 ); BOOST_REQUIRE( db.get_comment( "bob", std::string( "test" )
).net_rshares.value == 0 ); validate_database();

      BOOST_TEST_MESSAGE( "Generate block to cause payout" );

      reward_scorum = db.get_dynamic_global_properties().total_reward_fund_scorum + ASSET( "2.000 TESTS" );
      total_rshares2 = db.get_dynamic_global_properties().total_reward_shares2;
      auto alice_comment_vote_total = db.get_comment( "alice", std::string( "test" ) ).total_vote_weight;
      auto alice_comment_rshares = db.get_comment( "alice", std::string( "test" ) ).net_rshares;
      auto alice_sbd_balance = db.get_account( "alice" ).sbd_balance;
      alice_vest_shares = db.get_account( "alice" ).vesting_shares;
      bob_vest_shares = db.get_account( "bob" ).vesting_shares;
      sam_vest_shares = db.get_account( "sam" ).vesting_shares;
      dave_vest_shares = db.get_account( "dave" ).vesting_shares;

      u256 rs( alice_comment_rshares.value );
      u256 rf( reward_scorum.amount.value );
      u256 trs2 = total_rshares2.hi;
      trs2 = ( trs2 << 64 ) + total_rshares2.lo;
      auto rs2 = rs*rs;

      auto alice_comment_payout = asset( static_cast< uint64_t >( ( rf * rs2 ) / trs2 ), SCORUM_SYMBOL );
      auto alice_comment_vote_rewards = asset( alice_comment_payout.amount / 2, SCORUM_SYMBOL );
      alice_comment_payout -= alice_comment_vote_rewards;
      auto alice_comment_sbd_reward = asset( alice_comment_payout.amount / 2, SCORUM_SYMBOL ) * exchange_rate;
      auto alice_comment_vesting_reward = ( alice_comment_payout - asset( alice_comment_payout.amount / 2, SCORUM_SYMBOL
) ) * db.get_dynamic_global_properties().get_vesting_share_price();
      unclaimed_payments = alice_comment_vote_rewards;
      alice_vote_reward = asset( static_cast< uint64_t >( ( u256( vote_idx.find( std::make_tuple( db.get_comment(
"alice", std::string( "test" ).id, db.get_account( "alice" ) ).id ) )->weight ) *
alice_comment_vote_rewards.amount.value ) / alice_comment_vote_total ), SCORUM_SYMBOL ); alice_vote_vesting =
alice_vote_reward * db.get_dynamic_global_properties().get_vesting_share_price(); bob_vote_reward = asset( static_cast<
uint64_t >( ( u256( vote_idx.find( std::make_tuple( db.get_comment( "alice", std::string( "test" ).id, db.get_account(
"bob" ) ).id ) )->weight ) * alice_comment_vote_rewards.amount.value ) / alice_comment_vote_total ), SCORUM_SYMBOL );
      bob_vote_vesting = bob_vote_reward * db.get_dynamic_global_properties().get_vesting_share_price();
      sam_vote_reward = asset( static_cast< uint64_t >( ( u256( vote_idx.find( std::make_tuple( db.get_comment( "alice",
std::string( "test" ).id, db.get_account( "sam" ) ).id ) )->weight ) * alice_comment_vote_rewards.amount.value ) /
alice_comment_vote_total ), SCORUM_SYMBOL );
      sam_vote_vesting = sam_vote_reward * db.get_dynamic_global_properties().get_vesting_share_price();
      auto dave_vote_reward = asset( static_cast< uint64_t >( ( u256( vote_idx.find( std::make_tuple( db.get_comment(
"alice", std::string( "test" ).id, db.get_account( "dave" ) ).id ) )->weight ) * alice_comment_vote_rewards.amount.value
) / alice_comment_vote_total ), SCORUM_SYMBOL ); auto dave_vote_vesting = dave_vote_reward *
db.get_dynamic_global_properties().get_vesting_share_price(); unclaimed_payments -= ( alice_vote_reward +
bob_vote_reward + sam_vote_reward + dave_vote_reward );

      generate_block();
      auto alice_comment_reward = get_last_operations( 1 )[0].get< comment_reward_operation >();

      BOOST_REQUIRE( ( db.get_dynamic_global_properties().total_reward_fund_scorum + alice_comment_payout +
alice_comment_vote_rewards - unclaimed_payments ).amount.value == reward_scorum.amount.value );
      BOOST_REQUIRE( db.get_comment( "alice", std::string( "test" ) ).total_payout_value.amount.value == ( (
alice_comment_vesting_reward * db.get_dynamic_global_properties().get_vesting_share_price() ) + (
alice_comment_sbd_reward * exchange_rate ) ).amount.value );
      BOOST_REQUIRE( db.get_account( "alice" ).sbd_balance.amount.value == ( alice_sbd_balance +
alice_comment_sbd_reward ).amount.value );
      BOOST_REQUIRE( db.get_comment( "alice", std::string( "test" ) ).net_rshares.value == 0 );
      BOOST_REQUIRE( db.get_comment( "alice", std::string( "test" ) ).net_rshares.value == 0 );
      BOOST_REQUIRE( db.get_account( "alice" ).vesting_shares.amount.value == ( alice_vest_shares + alice_vote_vesting +
alice_comment_vesting_reward ).amount.value );
      BOOST_REQUIRE( db.get_account( "bob" ).vesting_shares.amount.value == ( bob_vest_shares + bob_vote_vesting
).amount.value );
      BOOST_REQUIRE( db.get_account( "sam" ).vesting_shares.amount.value == ( sam_vest_shares + sam_vote_vesting
).amount.value );
      BOOST_REQUIRE( db.get_account( "dave" ).vesting_shares.amount.value == ( dave_vest_shares + dave_vote_vesting
).amount.value );
      BOOST_REQUIRE( alice_comment_reward.author == "alice" );
      BOOST_REQUIRE( alice_comment_reward.permlink == "test" );
      BOOST_REQUIRE( alice_comment_reward.payout.amount.value == alice_comment_sbd_reward.amount.value );
      BOOST_REQUIRE( alice_comment_reward.vesting_payout.amount.value == alice_comment_vesting_reward.amount.value );
      BOOST_REQUIRE( vote_idx.find( std::make_tuple( db.get_comment( "alice", std::string( "test" ).id, db.get_account(
"alice" ) ).id ) ) == vote_idx.end() );
      BOOST_REQUIRE( vote_idx.find( std::make_tuple( db.get_comment( "alice", std::string( "test" ).id, db.get_account(
"bob" ) ).id   ) ) == vote_idx.end() ); BOOST_REQUIRE( vote_idx.find( std::make_tuple( db.get_comment( "alice",
std::string( "test" ).id, db.get_account( "sam" ) ).id   ) ) == vote_idx.end() ); BOOST_REQUIRE( vote_idx.find(
std::make_tuple( db.get_comment( "alice", std::string( "test" ).id, db.get_account( "dave" ) ).id  ) ) == vote_idx.end()
); BOOST_REQUIRE( vote_idx.find( std::make_tuple( db.get_comment( "bob", std::string( "test" ).id,   db.get_account(
"alice" ) ).id ) ) == vote_idx.end() );
      BOOST_REQUIRE( vote_idx.find( std::make_tuple( db.get_comment( "bob", std::string( "test" ).id,   db.get_account(
"bob" ) ).id   ) ) == vote_idx.end() ); BOOST_REQUIRE( vote_idx.find( std::make_tuple( db.get_comment( "bob",
std::string( "test" ).id,   db.get_account( "sam" ) ).id   ) ) == vote_idx.end() ); validate_database();

      BOOST_TEST_MESSAGE( "Testing no payout when less than $0.02" );

      tx.operations.clear();
      tx.signatures.clear();
      vote.voter = "alice";
      vote.author = "alice";
      tx.operations.push_back( vote );
      tx.set_expiration( db.head_block_time() + SCORUM_MAX_TIME_UNTIL_EXPIRATION );
      tx.sign( alice_private_key, db.get_chain_id() );
      db.push_transaction( tx, 0 );

      tx.operations.clear();
      tx.signatures.clear();
      vote.voter = "dave";
      vote.author = "bob";
      vote.weight = SCORUM_1_PERCENT;
      tx.operations.push_back( vote );
      tx.sign( dave_private_key, db.get_chain_id() );
      db.push_transaction( tx, 0 );

      generate_blocks( db.get_comment( "bob", std::string( "test" ) ).cashout_time - SCORUM_BLOCK_INTERVAL, true );

      tx.operations.clear();
      tx.signatures.clear();
      vote.voter = "bob";
      vote.author = "alice";
      vote.weight = SCORUM_100_PERCENT;
      tx.operations.push_back( vote );
      tx.set_expiration( db.head_block_time() + SCORUM_MAX_TIME_UNTIL_EXPIRATION );
      tx.sign( bob_private_key, db.get_chain_id() );
      db.push_transaction( tx, 0 );

      tx.operations.clear();
      tx.signatures.clear();
      vote.voter = "sam";
      tx.operations.push_back( vote );
      tx.sign( sam_private_key, db.get_chain_id() );
      db.push_transaction( tx, 0 );

      tx.operations.clear();
      tx.signatures.clear();
      vote.voter = "dave";
      tx.operations.push_back( vote );
      tx.sign( dave_private_key, db.get_chain_id() );
      db.push_transaction( tx, 0 );

      bob_vest_shares = db.get_account( "bob" ).vesting_shares;
      auto bob_sbd = db.get_account( "bob" ).sbd_balance;

      BOOST_REQUIRE( vote_idx.find( std::make_tuple( db.get_comment( "bob", std::string( "test" ).id, db.get_account(
"dave" ) ).id ) ) != vote_idx.end() ); validate_database();

      generate_block();

      BOOST_REQUIRE( vote_idx.find( std::make_tuple( db.get_comment( "bob", std::string( "test" ).id, db.get_account(
"dave" ) ).id ) ) == vote_idx.end() ); BOOST_REQUIRE( bob_vest_shares.amount.value == db.get_account( "bob"
).vesting_shares.amount.value ); BOOST_REQUIRE( bob_sbd.amount.value == db.get_account( "bob" ).sbd_balance.amount.value
); validate_database();
   }
   FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE( nested_comments )
{
   try
   {
      ACTORS( (alice)(bob)(sam)(dave) )
      fund( "alice", 10000 );
      vest( "alice", 10000 );
      fund( "bob", 10000 );
      vest( "bob", 10000 );
      fund( "sam", 10000 );
      vest( "sam", 10000 );
      fund( "dave", 10000 );
      vest( "dave", 10000 );

      price exchange_rate( ASSET( "1.000 TESTS" ), ASSET( "1.000 TBD" ) );
      set_price_feed( exchange_rate );

      signed_transaction tx;
      comment_operation comment_op;
      comment_op.author = "alice";
      comment_op.permlink = "test";
      comment_op.parent_permlink = "test";
      comment_op.title = "foo";
      comment_op.body = "bar";
      tx.set_expiration( db.head_block_time() + SCORUM_MAX_TIME_UNTIL_EXPIRATION );
      tx.operations.push_back( comment_op );
      tx.sign( alice_private_key, db.get_chain_id() );
      db.push_transaction( tx, 0 );

      comment_op.author = "bob";
      comment_op.parent_author = "alice";
      comment_op.parent_permlink = "test";
      tx.operations.clear();
      tx.signatures.clear();
      tx.operations.push_back( comment_op );
      tx.sign( bob_private_key, db.get_chain_id() );
      db.push_transaction( tx, 0 );

      comment_op.author = "sam";
      comment_op.parent_author = "bob";
      tx.operations.clear();
      tx.signatures.clear();
      tx.operations.push_back( comment_op );
      tx.sign( sam_private_key, db.get_chain_id() );
      db.push_transaction( tx, 0 );

      comment_op.author = "dave";
      comment_op.parent_author = "sam";
      tx.operations.clear();
      tx.signatures.clear();
      tx.operations.push_back( comment_op );
      tx.sign( dave_private_key, db.get_chain_id() );
      db.push_transaction( tx, 0 );

      tx.operations.clear();
      tx.signatures.clear();

      vote_operation vote_op;
      vote_op.voter = "alice";
      vote_op.author = "alice";
      vote_op.permlink = "test";
      vote_op.weight = SCORUM_100_PERCENT;
      tx.operations.push_back( vote_op );
      vote_op.author = "bob";
      tx.operations.push_back( vote_op );
      tx.sign( alice_private_key, db.get_chain_id() );
      db.push_transaction( tx, 0 );

      tx.operations.clear();
      tx.signatures.clear();
      vote_op.voter = "bob";
      vote_op.author = "alice";
      tx.operations.push_back( vote_op );
      vote_op.author = "bob";
      tx.operations.push_back( vote_op );
      vote_op.author = "dave";
      vote_op.weight = SCORUM_1_PERCENT * 20;
      tx.operations.push_back( vote_op );
      tx.sign( bob_private_key, db.get_chain_id() );
      db.push_transaction( tx, 0 );

      tx.operations.clear();
      tx.signatures.clear();
      vote_op.voter = "sam";
      vote_op.author = "bob";
      vote_op.weight = SCORUM_100_PERCENT;
      tx.operations.push_back( vote_op );
      tx.sign( sam_private_key, db.get_chain_id() );
      db.push_transaction( tx, 0 );

      generate_blocks( db.get_comment( "alice", std::string( "test" ) ).cashout_time - fc::seconds(
SCORUM_BLOCK_INTERVAL ), true );

      auto gpo = db.get_dynamic_global_properties();
      uint128_t reward_scorum = gpo.total_reward_fund_scorum.amount.value + ASSET( "2.000 TESTS" ).amount.value;
      uint128_t total_rshares2 = gpo.total_reward_shares2;

      auto alice_comment = db.get_comment( "alice", std::string( "test" ) );
      auto bob_comment = db.get_comment( "bob", std::string( "test" ) );
      auto sam_comment = db.get_comment( "sam", std::string( "test" ) );
      auto dave_comment = db.get_comment( "dave", std::string( "test" ) );

      const auto& vote_idx = db.get_index< comment_vote_index >().indices().get< by_comment_voter >();

      // Calculate total comment rewards and voting rewards.
      auto alice_comment_reward = ( ( reward_scorum * alice_comment.net_rshares.value * alice_comment.net_rshares.value
) / total_rshares2 ).to_uint64();
      total_rshares2 -= uint128_t( alice_comment.net_rshares.value ) * ( alice_comment.net_rshares.value );
      reward_scorum -= alice_comment_reward;
      auto alice_comment_vote_rewards = alice_comment_reward / 2;
      alice_comment_reward -= alice_comment_vote_rewards;

      auto alice_vote_alice_reward = asset( static_cast< uint64_t >( ( u256( vote_idx.find( std::make_tuple(
db.get_comment( "alice", std::string( "test" ).id, db.get_account( "alice" ) ).id ) )->weight ) *
alice_comment_vote_rewards ) / alice_comment.total_vote_weight ), SCORUM_SYMBOL ); auto bob_vote_alice_reward = asset(
static_cast< uint64_t >( ( u256( vote_idx.find( std::make_tuple( db.get_comment( "alice", std::string( "test" ).id,
db.get_account( "bob" ) ).id ) )->weight ) * alice_comment_vote_rewards ) / alice_comment.total_vote_weight ),
SCORUM_SYMBOL ); reward_scorum += alice_comment_vote_rewards - ( alice_vote_alice_reward + bob_vote_alice_reward
).amount.value;

      auto bob_comment_reward = ( ( reward_scorum * bob_comment.net_rshares.value * bob_comment.net_rshares.value ) /
total_rshares2 ).to_uint64();
      total_rshares2 -= uint128_t( bob_comment.net_rshares.value ) * bob_comment.net_rshares.value;
      reward_scorum -= bob_comment_reward;
      auto bob_comment_vote_rewards = bob_comment_reward / 2;
      bob_comment_reward -= bob_comment_vote_rewards;

      auto alice_vote_bob_reward = asset( static_cast< uint64_t >( ( u256( vote_idx.find( std::make_tuple(
db.get_comment( "bob", std::string( "test" ).id, db.get_account( "alice" ) ).id ) )->weight ) * bob_comment_vote_rewards
) / bob_comment.total_vote_weight ), SCORUM_SYMBOL ); auto bob_vote_bob_reward = asset( static_cast< uint64_t >( ( u256(
vote_idx.find( std::make_tuple( db.get_comment( "bob", std::string( "test" ).id, db.get_account( "bob" ) ).id )
)->weight ) * bob_comment_vote_rewards ) / bob_comment.total_vote_weight ), SCORUM_SYMBOL ); auto sam_vote_bob_reward =
asset( static_cast< uint64_t >( ( u256( vote_idx.find( std::make_tuple( db.get_comment( "bob", std::string( "test" ).id,
db.get_account( "sam" ) ).id ) )->weight ) * bob_comment_vote_rewards ) / bob_comment.total_vote_weight ), SCORUM_SYMBOL
); reward_scorum += bob_comment_vote_rewards - ( alice_vote_bob_reward + bob_vote_bob_reward + sam_vote_bob_reward
).amount.value;

      auto dave_comment_reward = ( ( reward_scorum * dave_comment.net_rshares.value * dave_comment.net_rshares.value ) /
total_rshares2 ).to_uint64();
      total_rshares2 -= uint128_t( dave_comment.net_rshares.value ) * dave_comment.net_rshares.value;
      reward_scorum -= dave_comment_reward;
      auto dave_comment_vote_rewards = dave_comment_reward / 2;
      dave_comment_reward -= dave_comment_vote_rewards;

      auto bob_vote_dave_reward = asset( static_cast< uint64_t >( ( u256( vote_idx.find( std::make_tuple(
db.get_comment( "dave", std::string( "test" ).id, db.get_account( "bob" ) ).id ) )->weight ) * dave_comment_vote_rewards
) / dave_comment.total_vote_weight ), SCORUM_SYMBOL ); reward_scorum += dave_comment_vote_rewards -
bob_vote_dave_reward.amount.value;

      // Calculate rewards paid to parent posts
      auto alice_pays_alice_sbd = alice_comment_reward / 2;
      auto alice_pays_alice_vest = alice_comment_reward - alice_pays_alice_sbd;
      auto bob_pays_bob_sbd = bob_comment_reward / 2;
      auto bob_pays_bob_vest = bob_comment_reward - bob_pays_bob_sbd;
      auto dave_pays_dave_sbd = dave_comment_reward / 2;
      auto dave_pays_dave_vest = dave_comment_reward - dave_pays_dave_sbd;

      auto bob_pays_alice_sbd = bob_pays_bob_sbd / 2;
      auto bob_pays_alice_vest = bob_pays_bob_vest / 2;
      bob_pays_bob_sbd -= bob_pays_alice_sbd;
      bob_pays_bob_vest -= bob_pays_alice_vest;

      auto dave_pays_sam_sbd = dave_pays_dave_sbd / 2;
      auto dave_pays_sam_vest = dave_pays_dave_vest / 2;
      dave_pays_dave_sbd -= dave_pays_sam_sbd;
      dave_pays_dave_vest -= dave_pays_sam_vest;
      auto dave_pays_bob_sbd = dave_pays_sam_sbd / 2;
      auto dave_pays_bob_vest = dave_pays_sam_vest / 2;
      dave_pays_sam_sbd -= dave_pays_bob_sbd;
      dave_pays_sam_vest -= dave_pays_bob_vest;
      auto dave_pays_alice_sbd = dave_pays_bob_sbd / 2;
      auto dave_pays_alice_vest = dave_pays_bob_vest / 2;
      dave_pays_bob_sbd -= dave_pays_alice_sbd;
      dave_pays_bob_vest -= dave_pays_alice_vest;

      // Calculate total comment payouts
      auto alice_comment_total_payout = db.to_sbd( asset( alice_pays_alice_sbd + alice_pays_alice_vest, SCORUM_SYMBOL )
);
      alice_comment_total_payout += db.to_sbd( asset( bob_pays_alice_sbd + bob_pays_alice_vest, SCORUM_SYMBOL ) );
      alice_comment_total_payout += db.to_sbd( asset( dave_pays_alice_sbd + dave_pays_alice_vest, SCORUM_SYMBOL ) );
      auto bob_comment_total_payout = db.to_sbd( asset( bob_pays_bob_sbd + bob_pays_bob_vest, SCORUM_SYMBOL ) );
      bob_comment_total_payout += db.to_sbd( asset( dave_pays_bob_sbd + dave_pays_bob_vest, SCORUM_SYMBOL ) );
      auto sam_comment_total_payout = db.to_sbd( asset( dave_pays_sam_sbd + dave_pays_sam_vest, SCORUM_SYMBOL ) );
      auto dave_comment_total_payout = db.to_sbd( asset( dave_pays_dave_sbd + dave_pays_dave_vest, SCORUM_SYMBOL ) );

      auto alice_starting_vesting = db.get_account( "alice" ).vesting_shares;
      auto alice_starting_sbd = db.get_account( "alice" ).sbd_balance;
      auto bob_starting_vesting = db.get_account( "bob" ).vesting_shares;
      auto bob_starting_sbd = db.get_account( "bob" ).sbd_balance;
      auto sam_starting_vesting = db.get_account( "sam" ).vesting_shares;
      auto sam_starting_sbd = db.get_account( "sam" ).sbd_balance;
      auto dave_starting_vesting = db.get_account( "dave" ).vesting_shares;
      auto dave_starting_sbd = db.get_account( "dave" ).sbd_balance;

      generate_block();

      gpo = db.get_dynamic_global_properties();

      // Calculate vesting share rewards from voting.
      auto alice_vote_alice_vesting = alice_vote_alice_reward * gpo.get_vesting_share_price();
      auto bob_vote_alice_vesting = bob_vote_alice_reward * gpo.get_vesting_share_price();
      auto alice_vote_bob_vesting = alice_vote_bob_reward * gpo.get_vesting_share_price();
      auto bob_vote_bob_vesting = bob_vote_bob_reward * gpo.get_vesting_share_price();
      auto sam_vote_bob_vesting = sam_vote_bob_reward * gpo.get_vesting_share_price();
      auto bob_vote_dave_vesting = bob_vote_dave_reward * gpo.get_vesting_share_price();

      BOOST_REQUIRE( db.get_comment( "alice", std::string( "test" ) ).total_payout_value.amount.value ==
alice_comment_total_payout.amount.value );
      BOOST_REQUIRE( db.get_comment( "bob", std::string( "test" ) ).total_payout_value.amount.value ==
bob_comment_total_payout.amount.value );
      BOOST_REQUIRE( db.get_comment( "sam", std::string( "test" ) ).total_payout_value.amount.value ==
sam_comment_total_payout.amount.value );
      BOOST_REQUIRE( db.get_comment( "dave", std::string( "test" ) ).total_payout_value.amount.value ==
dave_comment_total_payout.amount.value );

      // ops 0-3, 5-6, and 10 are comment reward ops
      auto ops = get_last_operations( 13 );

      BOOST_TEST_MESSAGE( "Checking Virtual Operation Correctness" );

      curate_reward_operation cur_vop;
      comment_reward_operation com_vop = ops[0].get< comment_reward_operation >();

      BOOST_REQUIRE( com_vop.author == "alice" );
      BOOST_REQUIRE( com_vop.permlink == "test" );
      BOOST_REQUIRE( com_vop.originating_author == "dave" );
      BOOST_REQUIRE( com_vop.originating_permlink == "test" );
      BOOST_REQUIRE( com_vop.payout.amount.value == dave_pays_alice_sbd );
      BOOST_REQUIRE( ( com_vop.vesting_payout * gpo.get_vesting_share_price() ).amount.value == dave_pays_alice_vest );

      com_vop = ops[1].get< comment_reward_operation >();
      BOOST_REQUIRE( com_vop.author == "bob" );
      BOOST_REQUIRE( com_vop.permlink == "test" );
      BOOST_REQUIRE( com_vop.originating_author == "dave" );
      BOOST_REQUIRE( com_vop.originating_permlink == "test" );
      BOOST_REQUIRE( com_vop.payout.amount.value == dave_pays_bob_sbd );
      BOOST_REQUIRE( ( com_vop.vesting_payout * gpo.get_vesting_share_price() ).amount.value == dave_pays_bob_vest );

      com_vop = ops[2].get< comment_reward_operation >();
      BOOST_REQUIRE( com_vop.author == "sam" );
      BOOST_REQUIRE( com_vop.permlink == "test" );
      BOOST_REQUIRE( com_vop.originating_author == "dave" );
      BOOST_REQUIRE( com_vop.originating_permlink == "test" );
      BOOST_REQUIRE( com_vop.payout.amount.value == dave_pays_sam_sbd );
      BOOST_REQUIRE( ( com_vop.vesting_payout * gpo.get_vesting_share_price() ).amount.value == dave_pays_sam_vest );

      com_vop = ops[3].get< comment_reward_operation >();
      BOOST_REQUIRE( com_vop.author == "dave" );
      BOOST_REQUIRE( com_vop.permlink == "test" );
      BOOST_REQUIRE( com_vop.originating_author == "dave" );
      BOOST_REQUIRE( com_vop.originating_permlink == "test" );
      BOOST_REQUIRE( com_vop.payout.amount.value == dave_pays_dave_sbd );
      BOOST_REQUIRE( ( com_vop.vesting_payout * gpo.get_vesting_share_price() ).amount.value == dave_pays_dave_vest );

      cur_vop = ops[4].get< curate_reward_operation >();
      BOOST_REQUIRE( cur_vop.curator == "bob" );
      BOOST_REQUIRE( cur_vop.reward.amount.value == bob_vote_dave_vesting.amount.value );
      BOOST_REQUIRE( cur_vop.comment_author == "dave" );
      BOOST_REQUIRE( cur_vop.comment_permlink == "test" );

      com_vop = ops[5].get< comment_reward_operation >();
      BOOST_REQUIRE( com_vop.author == "alice" );
      BOOST_REQUIRE( com_vop.permlink == "test" );
      BOOST_REQUIRE( com_vop.originating_author == "bob" );
      BOOST_REQUIRE( com_vop.originating_permlink == "test" );
      BOOST_REQUIRE( com_vop.payout.amount.value == bob_pays_alice_sbd );
      BOOST_REQUIRE( ( com_vop.vesting_payout * gpo.get_vesting_share_price() ).amount.value == bob_pays_alice_vest );

      com_vop = ops[6].get< comment_reward_operation >();
      BOOST_REQUIRE( com_vop.author == "bob" );
      BOOST_REQUIRE( com_vop.permlink == "test" );
      BOOST_REQUIRE( com_vop.originating_author == "bob" );
      BOOST_REQUIRE( com_vop.originating_permlink == "test" );
      BOOST_REQUIRE( com_vop.payout.amount.value == bob_pays_bob_sbd );
      BOOST_REQUIRE( ( com_vop.vesting_payout * gpo.get_vesting_share_price() ).amount.value == bob_pays_bob_vest );

      cur_vop = ops[7].get< curate_reward_operation >();
      BOOST_REQUIRE( cur_vop.curator == "sam" );
      BOOST_REQUIRE( cur_vop.reward.amount.value == sam_vote_bob_vesting.amount.value );
      BOOST_REQUIRE( cur_vop.comment_author == "bob" );
      BOOST_REQUIRE( cur_vop.comment_permlink == "test" );

      cur_vop = ops[8].get< curate_reward_operation >();
      BOOST_REQUIRE( cur_vop.curator == "bob" );
      BOOST_REQUIRE( cur_vop.reward.amount.value == bob_vote_bob_vesting.amount.value );
      BOOST_REQUIRE( cur_vop.comment_author == "bob" );
      BOOST_REQUIRE( cur_vop.comment_permlink == "test" );

      cur_vop = ops[9].get< curate_reward_operation >();
      BOOST_REQUIRE( cur_vop.curator == "alice" );
      BOOST_REQUIRE( cur_vop.reward.amount.value == alice_vote_bob_vesting.amount.value );
      BOOST_REQUIRE( cur_vop.comment_author == "bob" );
      BOOST_REQUIRE( cur_vop.comment_permlink == "test" );

      com_vop = ops[10].get< comment_reward_operation >();
      BOOST_REQUIRE( com_vop.author == "alice" );
      BOOST_REQUIRE( com_vop.permlink == "test" );
      BOOST_REQUIRE( com_vop.originating_author == "alice" );
      BOOST_REQUIRE( com_vop.originating_permlink == "test" );
      BOOST_REQUIRE( com_vop.payout.amount.value == alice_pays_alice_sbd );
      BOOST_REQUIRE( ( com_vop.vesting_payout * gpo.get_vesting_share_price() ).amount.value == alice_pays_alice_vest );

      cur_vop = ops[11].get< curate_reward_operation >();
      BOOST_REQUIRE( cur_vop.curator == "bob" );
      BOOST_REQUIRE( cur_vop.reward.amount.value == bob_vote_alice_vesting.amount.value );
      BOOST_REQUIRE( cur_vop.comment_author == "alice" );
      BOOST_REQUIRE( cur_vop.comment_permlink == "test" );

      cur_vop = ops[12].get< curate_reward_operation >();
      BOOST_REQUIRE( cur_vop.curator == "alice" );
      BOOST_REQUIRE( cur_vop.reward.amount.value == alice_vote_alice_vesting.amount.value );
      BOOST_REQUIRE( cur_vop.comment_author == "alice" );
      BOOST_REQUIRE( cur_vop.comment_permlink == "test" );

      BOOST_TEST_MESSAGE( "Checking account balances" );

      auto alice_total_sbd = alice_starting_sbd + asset( alice_pays_alice_sbd + bob_pays_alice_sbd +
dave_pays_alice_sbd, SCORUM_SYMBOL ) * exchange_rate;
      auto alice_total_vesting = alice_starting_vesting + asset( alice_pays_alice_vest + bob_pays_alice_vest +
dave_pays_alice_vest + alice_vote_alice_reward.amount + alice_vote_bob_reward.amount, SCORUM_SYMBOL ) *
gpo.get_vesting_share_price();
      BOOST_REQUIRE( db.get_account( "alice" ).sbd_balance.amount.value == alice_total_sbd.amount.value );
      BOOST_REQUIRE( db.get_account( "alice" ).vesting_shares.amount.value == alice_total_vesting.amount.value );

      auto bob_total_sbd = bob_starting_sbd + asset( bob_pays_bob_sbd + dave_pays_bob_sbd, SCORUM_SYMBOL ) *
exchange_rate;
      auto bob_total_vesting = bob_starting_vesting + asset( bob_pays_bob_vest + dave_pays_bob_vest +
bob_vote_alice_reward.amount + bob_vote_bob_reward.amount + bob_vote_dave_reward.amount, SCORUM_SYMBOL ) *
gpo.get_vesting_share_price();
      BOOST_REQUIRE( db.get_account( "bob" ).sbd_balance.amount.value == bob_total_sbd.amount.value );
      BOOST_REQUIRE( db.get_account( "bob" ).vesting_shares.amount.value == bob_total_vesting.amount.value );

      auto sam_total_sbd = sam_starting_sbd + asset( dave_pays_sam_sbd, SCORUM_SYMBOL ) * exchange_rate;
      auto sam_total_vesting = bob_starting_vesting + asset( dave_pays_sam_vest + sam_vote_bob_reward.amount,
SCORUM_SYMBOL ) * gpo.get_vesting_share_price();
      BOOST_REQUIRE( db.get_account( "sam" ).sbd_balance.amount.value == sam_total_sbd.amount.value );
      BOOST_REQUIRE( db.get_account( "sam" ).vesting_shares.amount.value == sam_total_vesting.amount.value );

      auto dave_total_sbd = dave_starting_sbd + asset( dave_pays_dave_sbd, SCORUM_SYMBOL ) * exchange_rate;
      auto dave_total_vesting = dave_starting_vesting + asset( dave_pays_dave_vest, SCORUM_SYMBOL ) *
gpo.get_vesting_share_price();
      BOOST_REQUIRE( db.get_account( "dave" ).sbd_balance.amount.value == dave_total_sbd.amount.value );
      BOOST_REQUIRE( db.get_account( "dave" ).vesting_shares.amount.value == dave_total_vesting.amount.value );
   }
   FC_LOG_AND_RETHROW()
}
*/

BOOST_AUTO_TEST_CASE(vesting_withdrawals)
{
    try
    {
        ACTORS((alice))
        fund("alice", 100000);
        vest("alice", 100000);

        const auto& new_alice = db.get_account("alice");

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

        BOOST_REQUIRE(db.get_account("alice").vesting_shares.amount.value == vesting_shares.amount.value);

        BOOST_TEST_MESSAGE("Generating block to cause withdrawal");
        generate_block();

        auto fill_op = get_last_operations(1)[0].get<fill_vesting_withdraw_operation>();
        auto gpo = db.get_dynamic_global_properties();

        BOOST_REQUIRE(db.get_account("alice").vesting_shares.amount.value
                      == (vesting_shares - withdraw_rate).amount.value);
        BOOST_REQUIRE((withdraw_rate * gpo.get_vesting_share_price()).amount.value
                          - db.get_account("alice").balance.amount.value
                      <= 1); // Check a range due to differences in the share price
        BOOST_REQUIRE(fill_op.from_account == "alice");
        BOOST_REQUIRE(fill_op.to_account == "alice");
        BOOST_REQUIRE(fill_op.withdrawn.amount.value == withdraw_rate.amount.value);
        BOOST_REQUIRE(std::abs((fill_op.deposited - fill_op.withdrawn * gpo.get_vesting_share_price()).amount.value)
                      <= 1);
        validate_database();

        BOOST_TEST_MESSAGE("Generating the rest of the blocks in the withdrawal");

        vesting_shares = db.get_account("alice").vesting_shares;
        auto balance = db.get_account("alice").balance;
        auto old_next_vesting = db.get_account("alice").next_vesting_withdrawal;

        for (int i = 1; i < SCORUM_VESTING_WITHDRAW_INTERVALS - 1; i++)
        {
            generate_blocks(db.head_block_time() + SCORUM_VESTING_WITHDRAW_INTERVAL_SECONDS);

            const auto& alice = db.get_account("alice");

            gpo = db.get_dynamic_global_properties();
            fill_op = get_last_operations(1)[0].get<fill_vesting_withdraw_operation>();

            BOOST_REQUIRE(alice.vesting_shares.amount.value == (vesting_shares - withdraw_rate).amount.value);
            BOOST_REQUIRE(balance.amount.value + (withdraw_rate * gpo.get_vesting_share_price()).amount.value
                              - alice.balance.amount.value
                          <= 1);
            BOOST_REQUIRE(fill_op.from_account == "alice");
            BOOST_REQUIRE(fill_op.to_account == "alice");
            BOOST_REQUIRE(fill_op.withdrawn.amount.value == withdraw_rate.amount.value);
            BOOST_REQUIRE(std::abs((fill_op.deposited - fill_op.withdrawn * gpo.get_vesting_share_price()).amount.value)
                          <= 1);

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

            BOOST_REQUIRE(db.get_account("alice").next_vesting_withdrawal.sec_since_epoch()
                          == (old_next_vesting + SCORUM_VESTING_WITHDRAW_INTERVAL_SECONDS).sec_since_epoch());
            BOOST_REQUIRE(fill_op.from_account == "alice");
            BOOST_REQUIRE(fill_op.to_account == "alice");
            BOOST_REQUIRE(fill_op.withdrawn.amount.value == withdraw_rate.amount.value);
            BOOST_REQUIRE(std::abs((fill_op.deposited - fill_op.withdrawn * gpo.get_vesting_share_price()).amount.value)
                          <= 1);

            generate_blocks(db.head_block_time() + SCORUM_VESTING_WITHDRAW_INTERVAL_SECONDS, true);
            gpo = db.get_dynamic_global_properties();
            fill_op = get_last_operations(1)[0].get<fill_vesting_withdraw_operation>();

            BOOST_REQUIRE(db.get_account("alice").next_vesting_withdrawal.sec_since_epoch()
                          == fc::time_point_sec::maximum().sec_since_epoch());
            BOOST_REQUIRE(fill_op.to_account == "alice");
            BOOST_REQUIRE(fill_op.from_account == "alice");
            BOOST_REQUIRE(fill_op.withdrawn.amount.value == to_withdraw.amount.value % withdraw_rate.amount.value);
            BOOST_REQUIRE(std::abs((fill_op.deposited - fill_op.withdrawn * gpo.get_vesting_share_price()).amount.value)
                          <= 1);

            validate_database();
        }
        else
        {
            generate_blocks(db.head_block_time() + SCORUM_VESTING_WITHDRAW_INTERVAL_SECONDS, true);

            BOOST_REQUIRE(db.get_account("alice").next_vesting_withdrawal.sec_since_epoch()
                          == fc::time_point_sec::maximum().sec_since_epoch());

            fill_op = get_last_operations(1)[0].get<fill_vesting_withdraw_operation>();
            BOOST_REQUIRE(fill_op.from_account == "alice");
            BOOST_REQUIRE(fill_op.to_account == "alice");
            BOOST_REQUIRE(fill_op.withdrawn.amount.value == withdraw_rate.amount.value);
            BOOST_REQUIRE(std::abs((fill_op.deposited - fill_op.withdrawn * gpo.get_vesting_share_price()).amount.value)
                          <= 1);
        }

        BOOST_REQUIRE(db.get_account("alice").vesting_shares.amount.value
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

        fund("alice", 1040000);
        vest("alice", 1040000);

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
        set_withdraw_vesting_route_operation op;
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
            const auto& alice = db.get_account("alice");
            const auto& bob = db.get_account("bob");
            const auto& sam = db.get_account("sam");

            BOOST_REQUIRE(alice.vesting_shares == old_alice_vesting - vesting_withdraw_rate);
            BOOST_REQUIRE(
                alice.balance
                == old_alice_balance
                    + asset((vesting_withdraw_rate.amount * SCORUM_1_PERCENT * 20) / SCORUM_100_PERCENT, VESTS_SYMBOL)
                        * db.get_dynamic_global_properties().get_vesting_share_price());
            BOOST_REQUIRE(
                bob.vesting_shares
                == old_bob_vesting
                    + asset((vesting_withdraw_rate.amount * SCORUM_1_PERCENT * 50) / SCORUM_100_PERCENT, VESTS_SYMBOL));
            BOOST_REQUIRE(bob.balance == old_bob_balance);
            BOOST_REQUIRE(sam.vesting_shares == old_sam_vesting);
            BOOST_REQUIRE(
                sam.balance
                == old_sam_balance
                    + asset((vesting_withdraw_rate.amount * SCORUM_1_PERCENT * 30) / SCORUM_100_PERCENT, VESTS_SYMBOL)
                        * db.get_dynamic_global_properties().get_vesting_share_price());

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

        generate_blocks(db.get_account("alice").next_vesting_withdrawal, true);
        {
            const auto& alice = db.get_account("alice");
            const auto& bob = db.get_account("bob");
            const auto& sam = db.get_account("sam");

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
                    + asset((vesting_withdraw_rate.amount * SCORUM_1_PERCENT * 50) / SCORUM_100_PERCENT, VESTS_SYMBOL)
                        * db.get_dynamic_global_properties().get_vesting_share_price());
        }
    }
    FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE(post_rate_limit)
{
    try
    {
        ACTORS((alice))

        fund("alice", 10000);
        vest("alice", 10000);

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
        fund("alice", 10000);
        fund("bob", 10000);
        fund("sam", 10000);
        fund("dave", 10000);

        vest("alice", 10000);
        vest("bob", 10000);
        vest("sam", 10000);
        vest("dave", 10000);

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
        vote.weight = SCORUM_100_PERCENT;
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
        vote.weight = SCORUM_100_PERCENT * -1;

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
        vote.weight = 0;

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
