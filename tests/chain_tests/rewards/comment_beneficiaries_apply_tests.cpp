#include <boost/test/unit_test.hpp>

#include <scorum/chain/database/database.hpp>

#include <scorum/chain/database_exceptions.hpp>
#include <scorum/chain/schema/scorum_objects.hpp>

#include <scorum/chain/schema/account_objects.hpp>
#include <scorum/chain/services/account.hpp>
#include <scorum/chain/services/comment.hpp>
#include <scorum/chain/services/comment_statistic.hpp>

#include "database_default_integration.hpp"

#include <string>
#include <map>

using namespace database_fixture;

BOOST_FIXTURE_TEST_SUITE(comment_beneficiaries_tests, database_default_integration_fixture)

struct comment_benefactor_reward_visitor
{
    typedef void result_type;

    database& _db;

    std::map<account_name_type, asset> reward_map;

    comment_benefactor_reward_visitor(database& db)
        : _db(db)
    {
    }

    void operator()(const comment_benefactor_reward_operation& op)
    {
        reward_map.insert(std::make_pair(op.benefactor, op.reward));
    }

    template <typename Op> void operator()(Op&&) const
    {
    } /// ignore all other ops
};

BOOST_AUTO_TEST_CASE(old_tests)
{
    try
    {
        BOOST_TEST_MESSAGE("Test Comment Beneficiaries");
        ACTORS((alice)(bob)(sam)(dave))

        vest("alice", ASSET_SCR(100e+3));
        vest("bob", ASSET_SCR(100e+3));
        vest("sam", ASSET_SCR(100e+3));
        vest("dave", ASSET_SCR(100e+3));

        generate_block();

        comment_operation comment;
        vote_operation vote;
        comment_options_operation op;
        comment_payout_beneficiaries b;
        signed_transaction tx;

        comment.author = "alice";
        comment.permlink = "test";
        comment.parent_permlink = "test";
        comment.title = "test";
        comment.body = "foobar";

        tx.operations.push_back(comment);
        tx.set_expiration(db.head_block_time() + SCORUM_MIN_TRANSACTION_EXPIRATION_LIMIT);
        tx.sign(alice_private_key, db.get_chain_id());
        db.push_transaction(tx);

        BOOST_TEST_MESSAGE("--- Test failure on more than 8 benefactors");
        b.beneficiaries.push_back(beneficiary_route_type(account_name_type("bob"), SCORUM_1_PERCENT));

        for (size_t i = 0; i < 8; i++)
        {
            b.beneficiaries.push_back(beneficiary_route_type(
                account_name_type(TEST_INIT_DELEGATE_NAME + fc::to_string(i)), SCORUM_1_PERCENT));
        }

        op.author = "alice";
        op.permlink = "test";
        op.allow_curation_rewards = false;
        op.extensions.insert(b);
        tx.clear();
        tx.operations.push_back(op);
        tx.sign(alice_private_key, db.get_chain_id());
        SCORUM_REQUIRE_THROW(db.push_transaction(tx), plugin_exception);

        BOOST_TEST_MESSAGE("--- Test specifying a non-existent benefactor");
        b.beneficiaries.clear();
        b.beneficiaries.push_back(beneficiary_route_type(account_name_type("doug"), SCORUM_1_PERCENT));
        op.extensions.clear();
        op.extensions.insert(b);
        tx.clear();
        tx.operations.push_back(op);
        tx.sign(alice_private_key, db.get_chain_id());
        SCORUM_REQUIRE_THROW(db.push_transaction(tx), fc::assert_exception);

        BOOST_TEST_MESSAGE("--- Test setting when comment has been voted on");
        vote.author = "alice";
        vote.permlink = "test";
        vote.voter = "bob";
        vote.weight = SCORUM_PERCENT(100);

        b.beneficiaries.clear();
        b.beneficiaries.push_back(beneficiary_route_type(account_name_type("bob"), 25 * SCORUM_1_PERCENT));
        b.beneficiaries.push_back(beneficiary_route_type(account_name_type("sam"), 50 * SCORUM_1_PERCENT));
        op.extensions.clear();
        op.extensions.insert(b);

        tx.clear();
        tx.operations.push_back(vote);
        tx.operations.push_back(op);
        tx.sign(alice_private_key, db.get_chain_id());
        tx.sign(bob_private_key, db.get_chain_id());
        SCORUM_REQUIRE_THROW(db.push_transaction(tx), fc::assert_exception);

        BOOST_TEST_MESSAGE("--- Test success");
        tx.clear();
        tx.operations.push_back(op);
        tx.sign(alice_private_key, db.get_chain_id());
        db.push_transaction(tx);

        BOOST_TEST_MESSAGE("--- Test setting when there are already beneficiaries");
        b.beneficiaries.clear();
        b.beneficiaries.push_back(beneficiary_route_type(account_name_type("dave"), 25 * SCORUM_1_PERCENT));
        op.extensions.clear();
        op.extensions.insert(b);
        tx.sign(alice_private_key, db.get_chain_id());
        SCORUM_REQUIRE_THROW(db.push_transaction(tx), fc::assert_exception);

        BOOST_TEST_MESSAGE("--- Payout and verify rewards were split properly");
        tx.clear();
        tx.operations.push_back(vote);
        tx.sign(bob_private_key, db.get_chain_id());
        db.push_transaction(tx, 0);

        generate_blocks(db.obtain_service<dbs_comment>().get("alice", std::string("test")).cashout_time
                        - SCORUM_BLOCK_INTERVAL);

        BOOST_REQUIRE_EQUAL(db.obtain_service<dbs_account>().get_account("bob").balance, ASSET_SCR(0));
        BOOST_REQUIRE_EQUAL(db.obtain_service<dbs_account>().get_account("sam").balance, ASSET_SCR(0));

        asset bob_sp_before = db.obtain_service<dbs_account>().get_account("bob").scorumpower;
        asset sam_sp_before = db.obtain_service<dbs_account>().get_account("sam").scorumpower;

        comment_benefactor_reward_visitor visitor(db);

        db.post_apply_operation.connect([&](const operation_notification& note) { note.op.visit(visitor); });

        generate_block();

        validate_database();

        BOOST_REQUIRE_EQUAL(visitor.reward_map.size(), size_t(2));

        BOOST_REQUIRE(visitor.reward_map.find("bob") != visitor.reward_map.end());
        BOOST_REQUIRE(visitor.reward_map.find("sam") != visitor.reward_map.end());

        BOOST_REQUIRE_EQUAL(visitor.reward_map["bob"],
                            (db.obtain_service<dbs_account>().get_account("bob").scorumpower - bob_sp_before));
        BOOST_REQUIRE_EQUAL(visitor.reward_map["sam"],
                            (db.obtain_service<dbs_account>().get_account("sam").scorumpower - sam_sp_before));

        // clang-format off
        const auto &alice_post_id = db.obtain_service<dbs_comment>().get("alice", std::string("test")).id;
        BOOST_REQUIRE_EQUAL(db.obtain_service<dbs_comment_statistic_sp>().get(alice_post_id).beneficiary_payout_value,
                            (visitor.reward_map["sam"] + visitor.reward_map["bob"]));
        // clang-format on
    }
    FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_SUITE_END()
