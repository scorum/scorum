#include <boost/test/unit_test.hpp>

#include <fc/io/json.hpp>
#include <scorum/chain/genesis/genesis_state.hpp>

namespace sc = scorum::chain;
namespace sp = scorum::protocol;

BOOST_AUTO_TEST_SUITE(deserialize_genesis_state)

BOOST_AUTO_TEST_CASE(check_accounts_fields)
{
    const std::string genesis_str = R"json(
                                    {
                                        "accounts":[
                                        {
                                            "name":"user",
                                            "public_key":"SCR1111111111111111111111111111111114T1Anm",
                                            "scr_amount":"0.000001000 SCR"
                                        }]
                                    }
                                    )json";

    const sc::genesis_state_type genesis_state = fc::json::from_string(genesis_str).as<sc::genesis_state_type>();

    BOOST_REQUIRE(genesis_state.accounts.size() == 1);

    sc::genesis_state_type::account_type account = genesis_state.accounts.front();

    BOOST_CHECK(account.name == "user");
    BOOST_CHECK(account.public_key == sp::public_key_type("SCR1111111111111111111111111111111114T1Anm"));
    BOOST_CHECK(account.scr_amount == sp::asset(1000, SCORUM_SYMBOL));
}

BOOST_AUTO_TEST_CASE(check_witness_fields)
{
    const std::string genesis_str = R"json(
                                    {
                                        "witness_candidates":[
                                        {
                                            "name":"user",
                                            "block_signing_key":"SCR1111111111111111111111111111111114T1Anm"
                                        }]
                                    }
                                    )json";

    const sc::genesis_state_type genesis_state = fc::json::from_string(genesis_str).as<sc::genesis_state_type>();

    BOOST_REQUIRE(genesis_state.witness_candidates.size() == 1);

    sc::genesis_state_type::witness_type w = genesis_state.witness_candidates.front();

    BOOST_CHECK(w.name == "user");
    BOOST_CHECK(w.block_signing_key == sp::public_key_type("SCR1111111111111111111111111111111114T1Anm"));
}

BOOST_AUTO_TEST_CASE(check_initial_timestamp)
{
    const std::string genesis_str = R"json({"initial_timestamp": "2017-11-28T14:48:10"})json";

    const sc::genesis_state_type genesis_state = fc::json::from_string(genesis_str).as<sc::genesis_state_type>();

    BOOST_CHECK(genesis_state.initial_timestamp == fc::time_point_sec(1511880490));
}

BOOST_AUTO_TEST_CASE(check_initial_supply)
{
    std::string genesis_str = R"json({ "accounts_supply": "1.000000000 SCR"})json";

    sc::genesis_state_type genesis_state = fc::json::from_string(genesis_str).as<sc::genesis_state_type>();

    BOOST_CHECK(genesis_state.accounts_supply.amount == 1000000000);
    BOOST_CHECK(genesis_state.accounts_supply.symbol() == SCORUM_SYMBOL);
}

BOOST_AUTO_TEST_CASE(check_rewards_supply)
{
    std::string genesis_str = R"json({ "rewards_supply": "1.000000000 SCR"})json";

    sc::genesis_state_type genesis_state = fc::json::from_string(genesis_str).as<sc::genesis_state_type>();

    BOOST_CHECK(genesis_state.rewards_supply.amount == 1000000000);
    BOOST_CHECK(genesis_state.rewards_supply.symbol() == SCORUM_SYMBOL);
}

BOOST_AUTO_TEST_SUITE_END()
