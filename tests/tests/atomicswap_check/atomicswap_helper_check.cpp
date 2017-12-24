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

SCORUM_TEST_CASE(hash)
{
    BOOST_REQUIRE_EQUAL(fc::sha256().data_size(), sizeof(hash_index_type().data));

    const char* secret = "bob secret";
    const std::string secret_hash = get_secret_hash(secret);

    // must equal to 'echo -n "bob secret" |openssl dgst -ripemd160'
    BOOST_REQUIRE_EQUAL(secret_hash, "a643e5f663957c013b52bcf5ba6ef341f67da33e");

    std::string contract_hash_hex = get_contract_hash_hex("bob", secret_hash);

    // must equal to 'echo -n "boba643e5f663957c013b52bcf5ba6ef341f67da33e" |openssl dgst -sha256'
    BOOST_REQUIRE_EQUAL(contract_hash_hex, "d20d8e59234953596a47782833c7b9d9a39b3936b9f11c2a5cc6cbae4aec9b73");

    hash_index_type contract_hash = get_contract_hash("bob", secret_hash);

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
