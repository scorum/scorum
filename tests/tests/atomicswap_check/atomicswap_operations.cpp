#ifdef IS_TEST_NET
#include <boost/test/unit_test.hpp>

#include <scorum/chain/scorum_objects.hpp>

#include "database_fixture.hpp"

#include <scorum/protocol/atomicswap_helper.hpp>
#include <scorum/chain/dbs_atomicswap.hpp>
#include <scorum/chain/dbs_account.hpp>

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

        initiate_op.initiator = "alice";
        initiate_op.participant = "bob";
        initiate_op.amount = ALICE_SHARE_FOR_BOB;
        initiate_op.secret_hash = alice_secret_hash;

        redeem_op.recipient = "bob";
        redeem_op.secret = ALICE_SECRET;
    }

    std::string alice_secret_hash;
    atomicswap_initiate_operation initiate_op;
    atomicswap_redeem_operation redeem_op;

    const std::string ALICE_SECRET = "CHOCHO GUNJ UNSOUR EARLY MUSHER ...";
    const asset ALICE_SHARE_FOR_BOB = asset(2, SCORUM_SYMBOL);
};

BOOST_FIXTURE_TEST_SUITE(atomicswap_operation_check, atomicswap_operation_check_fixture)

SCORUM_TEST_CASE(initiate_operation_check)
{
    BOOST_REQUIRE_NO_THROW(initiate_op.validate());
}

SCORUM_TEST_CASE(redeem_operation_check)
{
    BOOST_REQUIRE_NO_THROW(redeem_op.validate());
}

BOOST_AUTO_TEST_SUITE_END()

class atomicswap_operation_initiate_fixture : public atomicswap_operation_check_fixture
{
public:
    atomicswap_operation_initiate_fixture()
        : atomicswap_service(db.obtain_service<dbs_atomicswap>())
        , account_service(db.obtain_service<dbs_account>())
    {
        ACTOR(alice);
        ACTOR(bob);

        m_alice_private_key = alice_private_key;
        m_bob_private_key = bob_private_key;
        generate_block();

        fund("alice", ALICE_BALANCE);
    }

    dbs_atomicswap& atomicswap_service;
    dbs_account& account_service;

    private_key_type m_alice_private_key;
    private_key_type m_bob_private_key;

    const asset ALICE_BALANCE = asset(2 * SCORUM_ATOMICSWAP_LIMIT_REQUESTED_CONTRACTS + 1, SCORUM_SYMBOL);
};

BOOST_FIXTURE_TEST_SUITE(atomicswap_operation_initiate_check, atomicswap_operation_initiate_fixture)

SCORUM_TEST_CASE(initiate_check)
{
    const account_object& bob = account_service.get_account("bob");

    BOOST_REQUIRE_THROW(atomicswap_service.get_contract(bob, alice_secret_hash), fc::exception);

    signed_transaction tx;

    tx.set_expiration(db.head_block_time() + SCORUM_MAX_TIME_UNTIL_EXPIRATION);
    tx.operations.push_back(initiate_op);

    BOOST_REQUIRE_NO_THROW(tx.sign(m_alice_private_key, db.get_chain_id()));
    BOOST_REQUIRE_NO_THROW(tx.validate());

    BOOST_REQUIRE_NO_THROW(db.push_transaction(tx, 0));

    BOOST_REQUIRE_NO_THROW(validate_database());

    BOOST_REQUIRE_NO_THROW(atomicswap_service.get_contract(bob, alice_secret_hash));
}

BOOST_AUTO_TEST_SUITE_END()

class atomicswap_operation_redeem_fixture : public atomicswap_operation_initiate_fixture
{
public:
    atomicswap_operation_redeem_fixture()
    {
        signed_transaction tx;
        tx.set_expiration(db.head_block_time() + SCORUM_MAX_TIME_UNTIL_EXPIRATION);
        tx.operations.push_back(initiate_op);
        tx.sign(m_alice_private_key, db.get_chain_id());
        db.push_transaction(tx, 0);
    }
};

BOOST_FIXTURE_TEST_SUITE(atomicswap_operation_redeem_check, atomicswap_operation_redeem_fixture)

SCORUM_TEST_CASE(redeem_check)
{
    BOOST_REQUIRE_NO_THROW(validate_database());

    const account_object& bob = account_service.get_account("bob");

    BOOST_REQUIRE_NO_THROW(atomicswap_service.get_contract(bob, alice_secret_hash));

    signed_transaction tx;

    tx.set_expiration(db.head_block_time() + SCORUM_MAX_TIME_UNTIL_EXPIRATION);
    tx.operations.push_back(redeem_op);

    BOOST_REQUIRE_NO_THROW(tx.sign(m_bob_private_key, db.get_chain_id()));
    BOOST_REQUIRE_NO_THROW(tx.validate());

    BOOST_REQUIRE_NO_THROW(db.push_transaction(tx, 0));

    BOOST_REQUIRE_NO_THROW(validate_database());

    BOOST_REQUIRE_THROW(atomicswap_service.get_contract(bob, alice_secret_hash), fc::exception);
}

BOOST_AUTO_TEST_SUITE_END()

#endif
