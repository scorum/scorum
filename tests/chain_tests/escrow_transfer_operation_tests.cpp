#ifdef IS_TEST_NET
#include <boost/test/unit_test.hpp>

#include <scorum/protocol/exceptions.hpp>

#include <scorum/chain/database/database.hpp>
#include <scorum/chain/database_exceptions.hpp>
#include <scorum/chain/hardfork.hpp>
#include <scorum/chain/schema/scorum_objects.hpp>

#include <scorum/chain/util/reward.hpp>

#include <scorum/witness/witness_objects.hpp>

#include <fc/crypto/digest.hpp>

#include "database_default_integration.hpp"

#include <cmath>
#include <iostream>
#include <stdexcept>

using namespace scorum;
using namespace scorum::chain;
using namespace scorum::protocol;
using fc::string;

struct escrow_transfer_apply_fixture : public database_default_integration_fixture
{
    escrow_transfer_apply_fixture()
    {
        op.from = "alice";
        op.to = "bob";
        op.scorum_amount = ASSET_SCR(1e+3);
        op.escrow_id = 0;
        op.agent = "sam";
        op.fee = SUFFICIENT_FEE;
        op.json_meta = "";
        op.ratification_deadline = db.head_block_time() + 100;
        op.escrow_expiration = db.head_block_time() + 200;
    }

    ~escrow_transfer_apply_fixture()
    {
    }

    escrow_transfer_operation op;
};

BOOST_FIXTURE_TEST_SUITE(escrow_transfer_apply_tests, escrow_transfer_apply_fixture)

BOOST_AUTO_TEST_CASE(failure_when_from_cannot_cover_amount_plus_fee)
{
    try
    {
        ACTORS((alice)(bob)(sam))
        fund("alice", 10000);

        op.scorum_amount.amount = 10000;

        signed_transaction tx;
        tx.operations.push_back(op);
        tx.sign(alice_private_key, db.get_chain_id());

        SCORUM_REQUIRE_THROW(db.push_transaction(tx, 0), fc::exception);
    }
    FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE(failure_when_ratification_deadline_is_in_the_past)
{
    try
    {
        ACTORS((alice)(bob)(sam))
        fund("alice", 10000);

        op.scorum_amount.amount = 1000;
        op.ratification_deadline = db.head_block_time() - 200;

        signed_transaction tx;
        tx.operations.push_back(op);
        tx.sign(alice_private_key, db.get_chain_id());

        SCORUM_REQUIRE_THROW(db.push_transaction(tx, 0), fc::exception);
    }
    FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE(failure_when_expiration_is_in_the_past)
{
    try
    {
        ACTORS((alice)(bob)(sam))
        fund("alice", 10000);

        op.escrow_expiration = db.head_block_time() - 100;

        signed_transaction tx;
        tx.operations.push_back(op);
        tx.sign(alice_private_key, db.get_chain_id());

        SCORUM_REQUIRE_THROW(db.push_transaction(tx, 0), fc::exception);
    }
    FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_CASE(success_escrow_transfer_apply)
{
    try
    {
        ACTORS((alice)(bob)(sam))

        fund("alice", 10000);

        op.ratification_deadline = db.head_block_time() + 100;
        op.escrow_expiration = db.head_block_time() + 200;

        signed_transaction tx;
        tx.set_expiration(db.head_block_time() + SCORUM_MAX_TIME_UNTIL_EXPIRATION);
        tx.operations.push_back(op);
        tx.sign(alice_private_key, db.get_chain_id());

        auto alice_scorum_balance = alice.balance - op.scorum_amount - op.fee;
        auto bob_scorum_balance = bob.balance;
        auto sam_scorum_balance = sam.balance;

        db.push_transaction(tx, 0);

        const auto& escrow = db.get_escrow(op.from, op.escrow_id);

        BOOST_REQUIRE(escrow.escrow_id == op.escrow_id);
        BOOST_REQUIRE(escrow.from == op.from);
        BOOST_REQUIRE(escrow.to == op.to);
        BOOST_REQUIRE(escrow.agent == op.agent);
        BOOST_REQUIRE(escrow.ratification_deadline == op.ratification_deadline);
        BOOST_REQUIRE(escrow.escrow_expiration == op.escrow_expiration);
        BOOST_REQUIRE(escrow.scorum_balance == op.scorum_amount);
        BOOST_REQUIRE(escrow.pending_fee == op.fee);
        BOOST_REQUIRE(!escrow.to_approved);
        BOOST_REQUIRE(!escrow.agent_approved);
        BOOST_REQUIRE(!escrow.disputed);
        BOOST_REQUIRE(alice.balance == alice_scorum_balance);
        BOOST_REQUIRE(bob.balance == bob_scorum_balance);
        BOOST_REQUIRE(sam.balance == sam_scorum_balance);

        validate_database();
    }
    FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_SUITE_END()

#endif
