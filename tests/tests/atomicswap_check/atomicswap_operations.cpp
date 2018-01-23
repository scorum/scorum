#ifdef IS_TEST_NET
#include <boost/test/unit_test.hpp>

#include "database_fixture.hpp"

#include <scorum/protocol/atomicswap_helper.hpp>
#include <scorum/chain/services/atomicswap.hpp>
#include <scorum/chain/services/account.hpp>

#include <scorum/chain/schema/scorum_objects.hpp>
#include <scorum/chain/schema/account_objects.hpp>
#include <scorum/chain/schema/atomicswap_objects.hpp>

#include <functional>
#include <map>

using namespace scorum;
using namespace scorum::chain;
using namespace scorum::protocol;
using fc::string;

//
// usage for all budget tests 'chain_test  -t atomicswap_*'
//

class atomicswap_operation_check_fixture : public timed_blocks_database_fixture
{
public:
    atomicswap_operation_check_fixture()
    {
        alice_secret_hash = atomicswap::get_secret_hash(ALICE_SECRET);

        initiate_op.type = atomicswap_by_initiator;
        initiate_op.owner = "alice";
        initiate_op.recipient = "bob";
        initiate_op.amount = ALICE_SHARE_FOR_BOB;
        initiate_op.secret_hash = alice_secret_hash;

        participate_op.type = atomicswap_by_participant;
        participate_op.owner = "bob";
        participate_op.recipient = "alice";
        participate_op.amount = BOB_SHARE_FOR_ALICE;
        participate_op.secret_hash = alice_secret_hash;

        redeem_by_initiator_op.from = "bob";
        redeem_by_initiator_op.to = "alice";
        redeem_by_initiator_op.secret = ALICE_SECRET;

        redeem_by_participant_op.from = "alice";
        redeem_by_participant_op.to = "bob";
        redeem_by_participant_op.secret = ALICE_SECRET;

        refund_op.initiator = "alice";
        refund_op.participant = "bob";
        refund_op.secret_hash = alice_secret_hash;
    }

    std::string alice_secret_hash;
    atomicswap_initiate_operation initiate_op;
    atomicswap_initiate_operation participate_op;
    atomicswap_redeem_operation redeem_by_initiator_op;
    atomicswap_redeem_operation redeem_by_participant_op;
    atomicswap_refund_operation refund_op;

    const std::string ALICE_SECRET = "ab74c5c5";
    const asset ALICE_SHARE_FOR_BOB = asset(2, SCORUM_SYMBOL);
    const asset BOB_SHARE_FOR_ALICE = asset(3, SCORUM_SYMBOL);
};

BOOST_FIXTURE_TEST_SUITE(atomicswap_operation_check, atomicswap_operation_check_fixture)

SCORUM_TEST_CASE(initiate_operation_check)
{
    BOOST_REQUIRE_NO_THROW(initiate_op.validate());
}

SCORUM_TEST_CASE(redeem_operation_check)
{
    BOOST_REQUIRE_NO_THROW(redeem_by_initiator_op.validate());
    BOOST_REQUIRE_NO_THROW(redeem_by_participant_op.validate());
}

SCORUM_TEST_CASE(refund_operation_check)
{
    BOOST_REQUIRE_NO_THROW(refund_op.validate());
}

BOOST_AUTO_TEST_SUITE_END()

class atomicswap_operation_create_contract_fixture : public atomicswap_operation_check_fixture
{
public:
    atomicswap_operation_create_contract_fixture()
        : atomicswap_service(db.obtain_service<dbs_atomicswap>())
        , account_service(db.obtain_service<dbs_account>())
    {
        ACTORS((alice)(bob));

        m_alice_private_key = alice_private_key;
        m_bob_private_key = bob_private_key;
        generate_block();

        fund("alice", ALICE_BALANCE);
        fund("bob", BOB_BALANCE);
    }

    dbs_atomicswap& atomicswap_service;
    dbs_account& account_service;

    private_key_type m_alice_private_key;
    private_key_type m_bob_private_key;

    const asset ALICE_BALANCE
        = asset(ALICE_SHARE_FOR_BOB.amount * SCORUM_ATOMICSWAP_LIMIT_REQUESTED_CONTRACTS_PER_OWNER + 1, SCORUM_SYMBOL);
    const asset BOB_BALANCE
        = asset(BOB_SHARE_FOR_ALICE.amount * SCORUM_ATOMICSWAP_LIMIT_REQUESTED_CONTRACTS_PER_OWNER + 1, SCORUM_SYMBOL);
};

BOOST_FIXTURE_TEST_SUITE(atomicswap_operation_create_contract_check, atomicswap_operation_create_contract_fixture)

SCORUM_TEST_CASE(initiate_check)
{
    const account_object& alice = account_service.get_account("alice");
    const account_object& bob = account_service.get_account("bob");

    BOOST_REQUIRE_THROW(atomicswap_service.get_contract(alice, bob, alice_secret_hash), fc::exception);

    signed_transaction tx;

    tx.set_expiration(db.head_block_time() + SCORUM_MAX_TIME_UNTIL_EXPIRATION);
    tx.operations.push_back(initiate_op);

    BOOST_REQUIRE_NO_THROW(tx.sign(m_alice_private_key, db.get_chain_id()));
    BOOST_REQUIRE_NO_THROW(tx.validate());

    BOOST_REQUIRE_NO_THROW(db.push_transaction(tx, 0));

    BOOST_REQUIRE_NO_THROW(validate_database());

    BOOST_REQUIRE_NO_THROW(atomicswap_service.get_contract(alice, bob, alice_secret_hash));
}

SCORUM_TEST_CASE(participate_check)
{
    const account_object& alice = account_service.get_account("alice");
    const account_object& bob = account_service.get_account("bob");

    BOOST_CHECK_THROW(atomicswap_service.get_contract(bob, alice, alice_secret_hash), fc::exception);

    signed_transaction tx;

    tx.set_expiration(db.head_block_time() + SCORUM_MAX_TIME_UNTIL_EXPIRATION);
    tx.operations.push_back(participate_op);

    BOOST_REQUIRE_NO_THROW(tx.sign(m_bob_private_key, db.get_chain_id()));
    BOOST_REQUIRE_NO_THROW(tx.validate());

    BOOST_REQUIRE_NO_THROW(db.push_transaction(tx, 0));

    BOOST_REQUIRE_NO_THROW(validate_database());

    BOOST_REQUIRE_NO_THROW(atomicswap_service.get_contract(bob, alice, alice_secret_hash));
}

BOOST_AUTO_TEST_SUITE_END()

class atomicswap_operation_redeem_fixture : public atomicswap_operation_create_contract_fixture
{
public:
    atomicswap_operation_redeem_fixture()
    {
        initiate();
        participate();
    }

    void initiate()
    {
        signed_transaction tx;
        tx.set_expiration(db.head_block_time() + SCORUM_MAX_TIME_UNTIL_EXPIRATION);
        tx.operations.push_back(initiate_op);
        tx.sign(m_alice_private_key, db.get_chain_id());
        db.push_transaction(tx, 0);
    }

    void participate()
    {
        signed_transaction tx;
        tx.set_expiration(db.head_block_time() + SCORUM_MAX_TIME_UNTIL_EXPIRATION);
        tx.operations.push_back(participate_op);
        tx.sign(m_bob_private_key, db.get_chain_id());
        db.push_transaction(tx, 0);
    }
};

BOOST_FIXTURE_TEST_SUITE(atomicswap_operation_redeem_check, atomicswap_operation_redeem_fixture)

SCORUM_TEST_CASE(hide_secret_check)
{
    const account_object& alice = account_service.get_account("alice");
    const account_object& bob = account_service.get_account("bob");

    const atomicswap_contract_object& participant_contract
        = atomicswap_service.get_contract(bob, alice, alice_secret_hash);
    const atomicswap_contract_object& initiator_contract
        = atomicswap_service.get_contract(alice, bob, alice_secret_hash);

    // secrets on both contracts must be hidden
    BOOST_REQUIRE(participant_contract.secret.empty());
    BOOST_REQUIRE(initiator_contract.secret.empty());
}

SCORUM_TEST_CASE(redeem_by_initiator_check)
{
    const account_object& alice = account_service.get_account("alice");
    const account_object& bob = account_service.get_account("bob");

    BOOST_REQUIRE_NO_THROW(atomicswap_service.get_contract(alice, bob, alice_secret_hash));
    BOOST_REQUIRE_NO_THROW(atomicswap_service.get_contract(bob, alice, alice_secret_hash));

    signed_transaction tx;

    tx.set_expiration(db.head_block_time() + SCORUM_MAX_TIME_UNTIL_EXPIRATION);
    tx.operations.push_back(redeem_by_initiator_op);

    BOOST_REQUIRE_NO_THROW(tx.sign(m_alice_private_key, db.get_chain_id()));
    BOOST_REQUIRE_NO_THROW(tx.validate());

    BOOST_REQUIRE_NO_THROW(db.push_transaction(tx, 0));

    BOOST_REQUIRE_NO_THROW(validate_database());

    BOOST_REQUIRE_NO_THROW(atomicswap_service.get_contract(alice, bob, alice_secret_hash));
    BOOST_REQUIRE_NO_THROW(atomicswap_service.get_contract(bob, alice, alice_secret_hash));
}

SCORUM_TEST_CASE(participant_refund)
{
    const account_object& alice = account_service.get_account("alice");
    const account_object& bob = account_service.get_account("bob");

    BOOST_REQUIRE_NO_THROW(atomicswap_service.get_contract(bob, alice, alice_secret_hash));

    signed_transaction tx;

    tx.set_expiration(db.head_block_time() + SCORUM_MAX_TIME_UNTIL_EXPIRATION);
    tx.operations.push_back(refund_op);

    BOOST_REQUIRE_NO_THROW(tx.sign(m_bob_private_key, db.get_chain_id()));
    BOOST_REQUIRE_NO_THROW(tx.validate());

    BOOST_REQUIRE_NO_THROW(db.push_transaction(tx, 0));

    BOOST_REQUIRE_THROW(atomicswap_service.get_contract(bob, alice, alice_secret_hash), fc::exception);
}

SCORUM_TEST_CASE(initiator_cant_refund)
{
    const account_object& alice = account_service.get_account("alice");
    const account_object& bob = account_service.get_account("bob");

    BOOST_REQUIRE_NO_THROW(atomicswap_service.get_contract(alice, bob, alice_secret_hash));

    signed_transaction tx;

    tx.set_expiration(db.head_block_time() + SCORUM_MAX_TIME_UNTIL_EXPIRATION);
    tx.operations.push_back(refund_op);

    BOOST_REQUIRE_NO_THROW(tx.sign(m_alice_private_key, db.get_chain_id()));
    BOOST_REQUIRE_NO_THROW(tx.validate());

    // try to refund initiator contract
    BOOST_REQUIRE_THROW(db.push_transaction(tx, 0), fc::exception);
}

BOOST_AUTO_TEST_SUITE_END()

class atomicswap_operation_close_fixture : public atomicswap_operation_redeem_fixture
{
public:
    atomicswap_operation_close_fixture()
    {
        redeem_by_initiator();
    }

    void redeem_by_initiator()
    {
        signed_transaction tx;
        tx.set_expiration(db.head_block_time() + SCORUM_MAX_TIME_UNTIL_EXPIRATION);
        tx.operations.push_back(redeem_by_initiator_op);
        tx.sign(m_alice_private_key, db.get_chain_id());
        db.push_transaction(tx, 0);
    }
};

BOOST_FIXTURE_TEST_SUITE(atomicswap_operation_close_check, atomicswap_operation_close_fixture)

SCORUM_TEST_CASE(extract_secret_check)
{
    const account_object& alice = account_service.get_account("alice");
    const account_object& bob = account_service.get_account("bob");

    const atomicswap_contract_object& contract = atomicswap_service.get_contract(bob, alice, alice_secret_hash);

    // secret redeemed for participant contract (by redeem_by_initiator)
    BOOST_REQUIRE(!contract.secret.empty());
}

SCORUM_TEST_CASE(redeem_by_participant_check)
{
    const account_object& alice = account_service.get_account("alice");
    const account_object& bob = account_service.get_account("bob");

    BOOST_REQUIRE_NO_THROW(atomicswap_service.get_contract(alice, bob, alice_secret_hash));
    BOOST_REQUIRE_NO_THROW(atomicswap_service.get_contract(bob, alice, alice_secret_hash));

    signed_transaction tx;

    tx.set_expiration(db.head_block_time() + SCORUM_MAX_TIME_UNTIL_EXPIRATION);
    tx.operations.push_back(redeem_by_participant_op);

    BOOST_REQUIRE_NO_THROW(tx.sign(m_bob_private_key, db.get_chain_id()));
    BOOST_REQUIRE_NO_THROW(tx.validate());

    BOOST_REQUIRE_NO_THROW(db.push_transaction(tx, 0));

    BOOST_REQUIRE_NO_THROW(validate_database());

    // initiator contract must be closed
    BOOST_REQUIRE_THROW(atomicswap_service.get_contract(alice, bob, alice_secret_hash), fc::exception);
}

BOOST_AUTO_TEST_SUITE_END()

#endif
