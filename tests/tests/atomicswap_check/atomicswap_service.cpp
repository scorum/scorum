#ifdef IS_TEST_NET
#include <boost/test/unit_test.hpp>

#include <scorum/chain/dbs_atomicswap.hpp>
#include <scorum/chain/dbs_account.hpp>

#include <fc/time.hpp>

#include "database_fixture.hpp"

using namespace scorum;
using namespace scorum::chain;
using namespace scorum::protocol;
using fc::string;

//
// usage for all budget tests 'chain_test  -t atomicswap_*'
//

class atomicswap_service_check_fixture : public timed_blocks_database_fixture
{
public:
    atomicswap_service_check_fixture()
        : atomicswap_service(db.obtain_service<dbs_atomicswap>())
        , account_service(db.obtain_service<dbs_account>())
        , public_key(database_fixture::generate_private_key("user private key").get_public_key())
        , alice(account_service.create_account(
              "alice", "initdelegate", public_key, "", authority(), authority(), authority(), ASSET_NULL_SCR))
        , bob(account_service.create_account(
              "bob", "initdelegate", public_key, "", authority(), authority(), authority(), ASSET_NULL_SCR))
    {
        account_service.increase_balance(alice, ALICE_BALANCE);
    }

    dbs_atomicswap& atomicswap_service;
    dbs_account& account_service;

    const public_key_type public_key;
    const account_object& alice;
    const account_object& bob;

    const std::string ALICE_SECRET = "CHOCHO GUNJ UNSOUR EARLY MUSHER ...";
    const asset ALICE_BALANCE = asset(2 * SCORUM_ATOMICSWAP_LIMIT_REQUESTED_CONTRACTS + 1, SCORUM_SYMBOL);
    const asset ALICE_SHARE_FOR_BOB = asset(2, SCORUM_SYMBOL);
};

BOOST_FIXTURE_TEST_SUITE(atomicswap_service_create_initiator_check, atomicswap_service_check_fixture)

SCORUM_TEST_CASE(create_initiator_contract_check_balance)
{
    BOOST_REQUIRE_EQUAL(alice.balance, ALICE_BALANCE);

    BOOST_REQUIRE_NO_THROW(atomicswap_service.create_initiator_contract(alice, bob, ALICE_SHARE_FOR_BOB,
                                                                        atomicswap::get_secret_hash(ALICE_SECRET)));

    BOOST_REQUIRE_EQUAL(alice.balance, ALICE_BALANCE - ALICE_SHARE_FOR_BOB);
}

SCORUM_TEST_CASE(create_initiator_contract_check_get_contracts)
{
    BOOST_REQUIRE(atomicswap_service.get_contracts(alice).empty());

    BOOST_REQUIRE_NO_THROW(atomicswap_service.create_initiator_contract(alice, bob, ALICE_SHARE_FOR_BOB,
                                                                        atomicswap::get_secret_hash(ALICE_SECRET)));

    BOOST_REQUIRE_EQUAL(atomicswap_service.get_contracts(alice).size(), 1);
}

SCORUM_TEST_CASE(create_initiator_contract_check_get_contract)
{
    std::string secret_hash = atomicswap::get_secret_hash(ALICE_SECRET);

    BOOST_REQUIRE_THROW(atomicswap_service.get_contract(bob, secret_hash), fc::exception);

    BOOST_REQUIRE_NO_THROW(atomicswap_service.create_initiator_contract(alice, bob, ALICE_SHARE_FOR_BOB, secret_hash));

    const atomicswap_contract_object& contract = atomicswap_service.get_contract(bob, secret_hash);
    BOOST_REQUIRE_EQUAL(fc::microseconds(contract.deadline - contract.created).to_seconds(),
                        SCORUM_ATOMICSWAP_INITIATOR_REFUND_LOCK_SECS);
}

SCORUM_TEST_CASE(create_initiator_contract_check_double)
{
    std::string secret_hash = atomicswap::get_secret_hash(ALICE_SECRET);

    BOOST_REQUIRE_NO_THROW(atomicswap_service.create_initiator_contract(alice, bob, ALICE_SHARE_FOR_BOB, secret_hash));
    BOOST_REQUIRE_THROW(atomicswap_service.create_initiator_contract(alice, bob, ALICE_SHARE_FOR_BOB, secret_hash),
                        fc::assert_exception);
    secret_hash = atomicswap::get_secret_hash(ALICE_SECRET + "2");
    BOOST_REQUIRE_NO_THROW(atomicswap_service.create_initiator_contract(alice, bob, ALICE_SHARE_FOR_BOB, secret_hash));
}

SCORUM_TEST_CASE(create_initiator_contract_check_limit)
{
    std::string secret_hash = atomicswap::get_secret_hash(ALICE_SECRET);

    for (int ci = 0; ci < SCORUM_ATOMICSWAP_LIMIT_REQUESTED_CONTRACTS; ++ci)
    {
        BOOST_REQUIRE_NO_THROW(
            atomicswap_service.create_initiator_contract(alice, bob, ALICE_SHARE_FOR_BOB, secret_hash));
        std::stringstream data;
        data << secret_hash << ci;
        secret_hash = atomicswap::get_secret_hash(data.str());
    }

    BOOST_REQUIRE_EQUAL(atomicswap_service.get_contracts(alice).size(), SCORUM_ATOMICSWAP_LIMIT_REQUESTED_CONTRACTS);

    BOOST_REQUIRE_THROW(atomicswap_service.create_initiator_contract(alice, bob, ALICE_SHARE_FOR_BOB, secret_hash),
                        fc::assert_exception);
}

BOOST_AUTO_TEST_SUITE_END()

class atomicswap_service_check_redeem_fixture : public atomicswap_service_check_fixture
{
public:
    atomicswap_service_check_redeem_fixture()
    {
        alice_secret_hash = atomicswap::get_secret_hash(ALICE_SECRET);
        atomicswap_service.create_initiator_contract(alice, bob, ALICE_SHARE_FOR_BOB, alice_secret_hash);
    }

    std::string alice_secret_hash;
};

BOOST_FIXTURE_TEST_SUITE(atomicswap_service_redeem_check, atomicswap_service_check_redeem_fixture)

SCORUM_TEST_CASE(create_redeem_close_contract)
{
    BOOST_REQUIRE_EQUAL(atomicswap_service.get_contracts(alice).size(), 1);

    BOOST_REQUIRE_NO_THROW(atomicswap_service.redeem_contract(atomicswap_service.get_contracts(alice)[0]));

    BOOST_REQUIRE(atomicswap_service.get_contracts(alice).empty());
}

SCORUM_TEST_CASE(create_redeem_check_balance)
{
    const atomicswap_contract_object& contract = atomicswap_service.get_contract(bob, alice_secret_hash);

    BOOST_REQUIRE_EQUAL(bob.balance, ASSET_NULL_SCR);
    BOOST_REQUIRE_EQUAL(alice.balance, ALICE_BALANCE - ALICE_SHARE_FOR_BOB);

    BOOST_REQUIRE_NO_THROW(atomicswap_service.redeem_contract(contract));

    BOOST_REQUIRE_EQUAL(bob.balance, ALICE_SHARE_FOR_BOB);
    BOOST_REQUIRE_EQUAL(alice.balance, ALICE_BALANCE - ALICE_SHARE_FOR_BOB);

    BOOST_REQUIRE_THROW(atomicswap_service.get_contract(bob, alice_secret_hash), fc::exception);
}

BOOST_AUTO_TEST_SUITE_END()

#endif
