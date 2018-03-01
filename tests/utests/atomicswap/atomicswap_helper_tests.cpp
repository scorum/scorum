#include <boost/test/unit_test.hpp>

#include <scorum/protocol/atomicswap_helper.hpp>

#include <cstdlib>

#include <fc/crypto/sha512.hpp>

#include "defines.hpp"

using namespace scorum::protocol;
using namespace scorum::protocol::atomicswap;

//
// usage for all budget tests 'chain_test  -t atomicswap_*'
//

BOOST_AUTO_TEST_SUITE(atomicswap_helpers)

SCORUM_TEST_CASE(invalidate_secret_hash)
{
    const std::string valid_secret_hash("9bcc1253fe1e61d7ea1f7229a39add56d9929620");

    BOOST_REQUIRE_NO_THROW(validate_secret_hash(valid_secret_hash));

    std::string invalid_secret_hash;

    BOOST_CHECK_THROW(validate_secret_hash(invalid_secret_hash), fc::assert_exception);

    invalid_secret_hash = valid_secret_hash.substr(0, valid_secret_hash.size() - 1);
    BOOST_CHECK_THROW(validate_secret_hash(invalid_secret_hash), fc::assert_exception);

    invalid_secret_hash = valid_secret_hash;
    invalid_secret_hash[valid_secret_hash.size() / 2] = '?';
    BOOST_CHECK_THROW(validate_secret_hash(invalid_secret_hash), fc::exception);
}

SCORUM_TEST_CASE(invalidate_secret)
{
    const std::string valid_secret("cc714d36e2f0cb23fc74e6885cc00f031aeaa038599a7579d62623da6f74c5c5");

    BOOST_REQUIRE_NO_THROW(validate_secret(valid_secret));

    std::string invalid_secret;

    BOOST_CHECK_THROW(validate_secret(invalid_secret), fc::assert_exception);

    invalid_secret = valid_secret + std::string(SCORUM_ATOMICSWAP_SECRET_MAX_LENGTH, 'f');
    BOOST_CHECK_THROW(validate_secret(invalid_secret), fc::assert_exception);

    invalid_secret = valid_secret;
    invalid_secret[valid_secret.size() / 2] = '?';
    BOOST_CHECK_THROW(validate_secret(invalid_secret), fc::exception);
}

SCORUM_TEST_CASE(invalidate_contact_metadata)
{
    const std::string valid_metadata = R"json(
            {"owner": "Alice", "title": "Exchange operation", "from": "15 LTC", "to": "10 SCR"}
                                       )json";

    BOOST_REQUIRE_NO_THROW(validate_contract_metadata(valid_metadata));

    std::string invalid_metadata;

    invalid_metadata = valid_metadata + std::string(SCORUM_ATOMICSWAP_CONTRACT_METADATA_MAX_LENGTH, ' ');
    BOOST_CHECK_THROW(validate_contract_metadata(invalid_metadata), fc::assert_exception);
}

SCORUM_TEST_CASE(secret_generator)
{
    std::string entropy(SCORUM_ATOMICSWAP_SECRET_MAX_LENGTH * 10, ' ');

    BOOST_CHECK_LE(get_secret_hex(entropy).size(), (size_t)SCORUM_ATOMICSWAP_SECRET_MAX_LENGTH);

    entropy = std::string(SCORUM_ATOMICSWAP_SECRET_MAX_LENGTH / 10, ' ');

    BOOST_CHECK_LE(get_secret_hex(entropy).size(), (size_t)SCORUM_ATOMICSWAP_SECRET_MAX_LENGTH);

    fc::sha512 hash;

    BOOST_REQUIRE(hash.data_size() >= 32);

    BOOST_CHECK_EQUAL(get_secret_hex(entropy, 32).size(), (size_t)32 * 2);

    BOOST_CHECK_EQUAL(get_secret_hex(entropy, hash.data_size()).size(), hash.data_size() * 2);

    BOOST_CHECK_THROW(get_secret_hex(entropy, hash.data_size() + 1), fc::assert_exception);
}

SCORUM_TEST_CASE(gen_secret_hash)
{
    const char* secret = "alice secret";
    const std::string secret_hash = get_secret_hash(get_secret_hex(secret));

    BOOST_REQUIRE_NO_THROW(validate_secret_hash(secret_hash));
}

SCORUM_TEST_CASE(check_secret_hash)
{
    const char* secret_hex = "cc714d36e2f0cb23fc74e6885cc00f031aeaa038599a7579d62623da6f74c5c5";
    const std::string secret_hash = get_secret_hash(secret_hex);

    BOOST_REQUIRE_EQUAL(secret_hash, "9bcc1253fe1e61d7ea1f7229a39add56d9929620");
}

SCORUM_TEST_CASE(contract_hash)
{
    BOOST_REQUIRE_EQUAL(fc::sha256().data_size(), sizeof(hash_index_type().data));

    const std::string secret_hash = "9bcc1253fe1e61d7ea1f7229a39add56d9929620";

    std::string contract_hash_hex = get_contract_hash_hex("alice", "bob", secret_hash);

    // must equal to 'echo -n "alicebob9bcc1253fe1e61d7ea1f7229a39add56d9929620" |openssl dgst -sha256'
    BOOST_REQUIRE_EQUAL(contract_hash_hex, "e57bcda340c6744a12bcf947a242584e1dcc6e85e85d45062169a1e25bf4c491");

    hash_index_type contract_hash = get_contract_hash("alice", "bob", secret_hash);

    // check pack algorithm
    std::string contract_hash_raw = contract_hash;
    bool odd = false;
    char bt_hex[3]{ '\0' };
    std::size_t ci = 0;
    for (char ch : contract_hash_hex)
    {
        bt_hex[odd] = ch;
        odd = !odd;
        if (!odd)
        {
            char bt = (char)std::strtoul(bt_hex, 0, 16);
            BOOST_REQUIRE_EQUAL(contract_hash_raw[ci++], bt);
        }
    }
}

BOOST_AUTO_TEST_SUITE_END()
