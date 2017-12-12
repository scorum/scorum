#include <boost/test/unit_test.hpp>

#include <graphene/utilities/key_conversion.hpp>
#include <scorum/protocol/types.hpp>
#include <scorum/wallet/wallet.hpp>

namespace sp = scorum::protocol;
namespace sw = scorum::wallet;

class keys_fixture
{
public:
    // clang-format off
    // this data generated in cli wallet with "suggest_brain_key"
    const std::string secret = "PHANIC TRIBRAC WASHPOT BISTATE CHOCHO GUNJ UNSOUR EARLY MUSHER REITBOK BURLAP AUXOTOX SNIB BLONDE PAPESS OARMAN";
    const std::string wif = "5JSc8Dgi9mxNnVmz5S2eG28pENy1GNLnvpL8dMa1K1eUdtLK6oe";
    const std::string pub_key_str = "SCR6dZ53sSZPEdyb7CgL6gppML5hCpMwvaEMUFe9Rt89euPv6DPg6";
    // clang-format on
};

BOOST_FIXTURE_TEST_SUITE(test_keys_generation, keys_fixture)

BOOST_AUTO_TEST_CASE(secret_to_wif)
{
    const fc::ecc::private_key private_key = sw::utils::derive_private_key(secret, 0);

    BOOST_CHECK_EQUAL(wif, graphene::utilities::key_to_wif(private_key));
}

BOOST_AUTO_TEST_CASE(check_wif_to_public_key)
{
    const fc::optional<fc::ecc::private_key> private_key = graphene::utilities::wif_to_key(wif);

    const sp::public_key_type public_key = private_key->get_public_key();

    BOOST_CHECK_EQUAL(pub_key_str, (std::string)public_key);
}

BOOST_AUTO_TEST_SUITE_END()

class suggest_brain_key_fixture
{
public:
    const sw::brain_key_info brain_key_data = sw::utils::suggest_brain_key();
};

BOOST_FIXTURE_TEST_SUITE(suggest_brain_key_tests, suggest_brain_key_fixture)

BOOST_AUTO_TEST_CASE(secret_to_wif)
{
    const fc::ecc::private_key private_key = sw::utils::derive_private_key(brain_key_data.brain_priv_key, 0);
    BOOST_CHECK_EQUAL(brain_key_data.wif_priv_key, graphene::utilities::key_to_wif(private_key));
}

BOOST_AUTO_TEST_CASE(check_wif_to_public_key)
{
    const fc::optional<fc::ecc::private_key> private_key = graphene::utilities::wif_to_key(brain_key_data.wif_priv_key);
    const sp::public_key_type public_key = private_key->get_public_key();
    BOOST_CHECK(brain_key_data.pub_key == public_key);
}

BOOST_AUTO_TEST_SUITE_END()
