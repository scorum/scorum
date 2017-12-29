#include <boost/test/unit_test.hpp>

#include "database_fixture.hpp"

#include <scorum/protocol/atomicswap_helper.hpp>

#include <cstdlib>

using namespace scorum::chain;
using namespace scorum::protocol;
using namespace scorum::protocol::atomicswap;

//
// usage for all budget tests 'chain_test  -t atomicswap_*'
//

BOOST_AUTO_TEST_SUITE(atomicswap_helpers)

SCORUM_TEST_CASE(get_hash)
{
    BOOST_REQUIRE_EQUAL(fc::sha256().data_size(), sizeof(hash_index_type().data));

    const char* secret = "alice secret";
    const std::string secret_hash = get_secret_hash(secret);

    // must equal to 'echo -n "alice secret" |openssl dgst -ripemd160'
    BOOST_REQUIRE_EQUAL(secret_hash, "9cbe2869c0e63c51a0ad77bbd2725d8beb184464");

    std::string contract_hash_hex = get_contract_hash_hex("alice", "bob", secret_hash);

    // must equal to 'echo -n "alicebob9cbe2869c0e63c51a0ad77bbd2725d8beb184464" |openssl dgst -sha256'
    BOOST_REQUIRE_EQUAL(contract_hash_hex, "a79c1994edb8c9cdc06a211e40113ff019a710d1ab5d0a99008bdfb4ac37190a");

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
