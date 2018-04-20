#include <boost/test/unit_test.hpp>

#include <scorum/chain/services/atomicswap.hpp>
#include <scorum/chain/services/account.hpp>

#include <scorum/chain/schema/account_objects.hpp>
#include <scorum/chain/schema/atomicswap_objects.hpp>

#include <sstream>

#include <fc/time.hpp>

#include "database_default_integration.hpp"

using namespace database_fixture;

class atomicswap_service_check_fixture : public database_default_integration_fixture
{
public:
    atomicswap_service_check_fixture()
        : atomicswap_service(db.obtain_service<dbs_atomicswap>())
        , account_service(db.obtain_service<dbs_account>())
        , public_key(database_integration_fixture::generate_private_key("user private key").get_public_key())
        , alice(account_service.create_account(
              "alice", "initdelegate", public_key, "", authority(), authority(), authority(), ASSET_NULL_SCR))
        , bob(account_service.create_account(
              "bob", "initdelegate", public_key, "", authority(), authority(), authority(), ASSET_NULL_SCR))
    {
        account_service.increase_balance(alice, ALICE_BALANCE);
        account_service.increase_balance(bob, BOB_BALANCE);

        for (uint32_t ci = 0; ci < SCORUM_ATOMICSWAP_LIMIT_REQUESTED_CONTRACTS_PER_OWNER + 1; ++ci)
        {
            std::stringstream store;
            store << "man" << ci;
            account_name_type next_name = store.str();
            const account_object& man = account_service.create_account(
                next_name, "initdelegate", public_key, "", authority(), authority(), authority(), ASSET_NULL_SCR);
            account_service.increase_balance(man, MAN_BALANCE);
            people.push_back(next_name);
        }

        alice_secret_hash = atomicswap::get_secret_hash(ALICE_SECRET);
    }

    dbs_atomicswap& atomicswap_service;
    dbs_account& account_service;

    const public_key_type public_key;
    const account_object& alice;
    const account_object& bob;

    using recipients_type = std::vector<account_name_type>;
    recipients_type people;

    std::string alice_secret_hash;

    const std::string ALICE_SECRET = "ab74c5c5";
    const asset ALICE_BALANCE = asset(2 * SCORUM_ATOMICSWAP_LIMIT_REQUESTED_CONTRACTS_PER_OWNER + 1, SCORUM_SYMBOL);
    const asset BOB_BALANCE = asset(3 * SCORUM_ATOMICSWAP_LIMIT_REQUESTED_CONTRACTS_PER_OWNER + 1, SCORUM_SYMBOL);
    const asset ALICE_SHARE_FOR_BOB = asset(2, SCORUM_SYMBOL);
    const asset BOB_SHARE_FOR_ALICE = asset(3, SCORUM_SYMBOL);

    const std::string MAN_SECRET = ALICE_SECRET;
    const asset MAN_BALANCE = asset(SCORUM_ATOMICSWAP_LIMIT_REQUESTED_CONTRACTS_PER_OWNER + 1, SCORUM_SYMBOL);
    const asset MAN_SHARE_FOR_BOB = asset(1, SCORUM_SYMBOL);
};

BOOST_FIXTURE_TEST_SUITE(atomicswap_service_create_initiator_check, atomicswap_service_check_fixture)

SCORUM_TEST_CASE(create_initiator_contract_check_balance)
{
    BOOST_REQUIRE_EQUAL(alice.balance, ALICE_BALANCE);

    BOOST_REQUIRE_NO_THROW(atomicswap_service.create_contract(
        atomicswap_contract_initiator, alice, bob, ALICE_SHARE_FOR_BOB, atomicswap::get_secret_hash(ALICE_SECRET)));

    BOOST_REQUIRE_EQUAL(alice.balance, ALICE_BALANCE - ALICE_SHARE_FOR_BOB);
}

SCORUM_TEST_CASE(create_initiator_contract_check_get_contracts)
{
    BOOST_REQUIRE(atomicswap_service.get_contracts(alice).empty());

    BOOST_REQUIRE_NO_THROW(atomicswap_service.create_contract(
        atomicswap_contract_initiator, alice, bob, ALICE_SHARE_FOR_BOB, atomicswap::get_secret_hash(ALICE_SECRET)));

    BOOST_REQUIRE_EQUAL(atomicswap_service.get_contracts(alice).size(), (size_t)1);
}

SCORUM_TEST_CASE(create_initiator_contract_check_get_contract)
{
    std::string secret_hash = atomicswap::get_secret_hash(ALICE_SECRET);

    BOOST_REQUIRE_THROW(atomicswap_service.get_contract(alice, bob, secret_hash), fc::exception);

    BOOST_REQUIRE_NO_THROW(atomicswap_service.create_contract(atomicswap_contract_initiator, alice, bob,
                                                              ALICE_SHARE_FOR_BOB, secret_hash));

    const atomicswap_contract_object& contract = atomicswap_service.get_contract(alice, bob, secret_hash);
    BOOST_REQUIRE_EQUAL(fc::microseconds(contract.deadline - contract.created).to_seconds(),
                        SCORUM_ATOMICSWAP_INITIATOR_REFUND_LOCK_SECS);
}

SCORUM_TEST_CASE(create_initiator_contract_check_double)
{
    std::string secret_hash = atomicswap::get_secret_hash(ALICE_SECRET);

    BOOST_REQUIRE_NO_THROW(atomicswap_service.create_contract(atomicswap_contract_initiator, alice, bob,
                                                              ALICE_SHARE_FOR_BOB, secret_hash));
    BOOST_REQUIRE_THROW(
        atomicswap_service.create_contract(atomicswap_contract_initiator, alice, bob, ALICE_SHARE_FOR_BOB, secret_hash),
        fc::assert_exception);
    secret_hash = atomicswap::get_secret_hash(ALICE_SECRET + "2");
    BOOST_REQUIRE_NO_THROW(atomicswap_service.create_contract(atomicswap_contract_initiator, alice, bob,
                                                              ALICE_SHARE_FOR_BOB, secret_hash));
}

SCORUM_TEST_CASE(create_initiator_contract_check_limit_per_owner)
{
    std::string secret_hash = atomicswap::get_secret_hash(ALICE_SECRET);

    uint32_t ci = 0;
    for (; ci < SCORUM_ATOMICSWAP_LIMIT_REQUESTED_CONTRACTS_PER_OWNER; ++ci)
    {
        const account_object& man = account_service.get_account(people[ci]);
        BOOST_REQUIRE_NO_THROW(atomicswap_service.create_contract(atomicswap_contract_initiator, alice, man,
                                                                  ALICE_SHARE_FOR_BOB, secret_hash));
        std::stringstream data;
        data << secret_hash << ci;
        secret_hash = atomicswap::get_secret_hash(data.str());
    }

    BOOST_REQUIRE_EQUAL(atomicswap_service.get_contracts(alice).size(),
                        (size_t)SCORUM_ATOMICSWAP_LIMIT_REQUESTED_CONTRACTS_PER_OWNER);

    BOOST_REQUIRE_THROW(atomicswap_service.create_contract(atomicswap_contract_initiator, alice,
                                                           account_service.get_account(people[ci]), ALICE_SHARE_FOR_BOB,
                                                           secret_hash),
                        fc::assert_exception);
}

SCORUM_TEST_CASE(create_initiator_contract_check_limit_per_recipient)
{
    std::string secret_hash = atomicswap::get_secret_hash(MAN_SECRET);

    uint32_t ci = 0;
    for (; ci < SCORUM_ATOMICSWAP_LIMIT_REQUESTED_CONTRACTS_PER_RECIPIENT; ++ci)
    {
        const account_object& man = account_service.get_account(people[ci]);
        BOOST_REQUIRE_NO_THROW(atomicswap_service.create_contract(atomicswap_contract_initiator, man, bob,
                                                                  MAN_SHARE_FOR_BOB, secret_hash));
        std::stringstream data;
        data << secret_hash << ci;
        secret_hash = atomicswap::get_secret_hash(data.str());
    }

    BOOST_REQUIRE_THROW(atomicswap_service.create_contract(atomicswap_contract_initiator,
                                                           account_service.get_account(people[ci]), bob,
                                                           MAN_SHARE_FOR_BOB, secret_hash),
                        fc::assert_exception);
}

BOOST_AUTO_TEST_SUITE_END()

class atomicswap_service_check_redeem_alice_contract_fixture : public atomicswap_service_check_fixture
{
public:
    atomicswap_service_check_redeem_alice_contract_fixture()
    {
        atomicswap_service.create_contract(atomicswap_contract_initiator, alice, bob, ALICE_SHARE_FOR_BOB,
                                           alice_secret_hash);
    }
};

BOOST_FIXTURE_TEST_SUITE(atomicswap_service_redeem_alice_contract_check,
                         atomicswap_service_check_redeem_alice_contract_fixture)

SCORUM_TEST_CASE(create_redeem_close_contract)
{
    BOOST_REQUIRE_EQUAL(atomicswap_service.get_contracts(alice).size(), (size_t)1);

    BOOST_REQUIRE_NO_THROW(
        atomicswap_service.redeem_contract(atomicswap_service.get_contracts(alice)[0], ALICE_SECRET));

    BOOST_REQUIRE(atomicswap_service.get_contracts(alice).empty());
}

SCORUM_TEST_CASE(create_redeem_by_participant_check_balance)
{
    const atomicswap_contract_object& contract = atomicswap_service.get_contract(alice, bob, alice_secret_hash);

    BOOST_REQUIRE_EQUAL(bob.balance, BOB_BALANCE);
    BOOST_REQUIRE_EQUAL(alice.balance, ALICE_BALANCE - ALICE_SHARE_FOR_BOB);

    BOOST_REQUIRE_NO_THROW(atomicswap_service.redeem_contract(contract, ALICE_SECRET));

    BOOST_REQUIRE_EQUAL(bob.balance, BOB_BALANCE + ALICE_SHARE_FOR_BOB);
    BOOST_REQUIRE_EQUAL(alice.balance, ALICE_BALANCE - ALICE_SHARE_FOR_BOB);

    BOOST_REQUIRE_THROW(atomicswap_service.get_contract(alice, bob, alice_secret_hash), fc::exception);
}

BOOST_AUTO_TEST_SUITE_END()

class atomicswap_service_check_redeem_bob_contract_fixture : public atomicswap_service_check_fixture
{
public:
    atomicswap_service_check_redeem_bob_contract_fixture()
    {
        atomicswap_service.create_contract(atomicswap_contract_participant, bob, alice, BOB_SHARE_FOR_ALICE,
                                           alice_secret_hash);
    }
};

BOOST_FIXTURE_TEST_SUITE(atomicswap_service_redeem_bob_contract_check,
                         atomicswap_service_check_redeem_bob_contract_fixture)

SCORUM_TEST_CASE(create_redeem_no_close_contract)
{
    const atomicswap_contract_object& contract = atomicswap_service.get_contract(bob, alice, alice_secret_hash);

    BOOST_REQUIRE_NO_THROW(atomicswap_service.redeem_contract(contract, ALICE_SECRET));

    BOOST_REQUIRE_NO_THROW(atomicswap_service.get_contract(bob, alice, alice_secret_hash));
}

SCORUM_TEST_CASE(create_redeem_by_initiator_check_balance)
{
    const atomicswap_contract_object& contract = atomicswap_service.get_contract(bob, alice, alice_secret_hash);

    BOOST_REQUIRE_EQUAL(bob.balance, BOB_BALANCE - BOB_SHARE_FOR_ALICE);
    BOOST_REQUIRE_EQUAL(alice.balance, ALICE_BALANCE);

    BOOST_REQUIRE_NO_THROW(atomicswap_service.redeem_contract(contract, ALICE_SECRET));

    BOOST_REQUIRE_EQUAL(bob.balance, BOB_BALANCE - BOB_SHARE_FOR_ALICE);
    BOOST_REQUIRE_EQUAL(alice.balance, ALICE_BALANCE + BOB_SHARE_FOR_ALICE);

    BOOST_REQUIRE_NO_THROW(atomicswap_service.get_contract(bob, alice, alice_secret_hash));
}

SCORUM_TEST_CASE(refund)
{
    const atomicswap_contract_object& contract = atomicswap_service.get_contract(bob, alice, alice_secret_hash);

    BOOST_REQUIRE_EQUAL(bob.balance, BOB_BALANCE - BOB_SHARE_FOR_ALICE);

    BOOST_REQUIRE_NO_THROW(atomicswap_service.refund_contract(contract));

    BOOST_REQUIRE_EQUAL(bob.balance, BOB_BALANCE);
}

BOOST_AUTO_TEST_SUITE_END()
