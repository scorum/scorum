#ifdef IS_TEST_NET
#include <boost/test/unit_test.hpp>

#include <scorum/chain/schema/account_objects.hpp>
#include <scorum/chain/services/account.hpp>
#include <scorum/chain/services/witness.hpp>
#include <scorum/chain/services/dynamic_global_property.hpp>

#include "database_default_integration.hpp"

namespace database_fixture {

class witness_data_service_fixture : public database_default_integration_fixture
{
public:
    witness_data_service_fixture()
        : account_service(db.obtain_service<dbs_account>())
        , witness_service(db.obtain_service<dbs_witness>())
        , user("user")
    {
    }

    void create_account()
    {
        account_service.create_account(user.name, initdelegate.name, user.public_key, "", authority(), authority(),
                                       authority(), asset(0, SCORUM_SYMBOL));
    }

    share_type calc_fee()
    {
        const auto& dprops = db.obtain_service<dbs_dynamic_global_property>().get();
        return std::max(dprops.median_chain_props.account_creation_fee.amount
                            * SCORUM_CREATE_ACCOUNT_WITH_SCORUM_MODIFIER,
                        share_type(100));
    }

    dbs_account& account_service;
    dbs_witness& witness_service;

    const Actor user;
};

} // database_fixture

using namespace scorum::chain;
using namespace scorum::protocol;

BOOST_FIXTURE_TEST_SUITE(witness_data_service, database_fixture::witness_data_service_fixture)

SCORUM_TEST_CASE(check_create_witness)
{
    try
    {
        const char* url = "http://my_url";
        chain_properties chain_props;
        chain_props.account_creation_fee = asset(1000, SCORUM_SYMBOL);
        chain_props.maximum_block_size = 500;
        public_key_type signing_key = private_key_type::regenerate(fc::sha256::hash(user.name)).get_public_key();

        create_account();

        const account_object& account = db.obtain_service<dbs_account>().get_account(user.name);

        const witness_object& witness = witness_service.create_witness(account.name, url, signing_key, chain_props);

        BOOST_REQUIRE(witness.signing_key == signing_key);
        BOOST_REQUIRE_EQUAL(witness.url, url);
        BOOST_REQUIRE_EQUAL(witness.proposed_chain_props.account_creation_fee, chain_props.account_creation_fee);
        BOOST_REQUIRE_EQUAL(witness.proposed_chain_props.maximum_block_size, chain_props.maximum_block_size);
    }
    FC_LOG_AND_RETHROW()
}

SCORUM_TEST_CASE(check_create_witness_with_empty_key)
{
    try
    {
        const char* url = "http://my_url";
        chain_properties chain_props;
        chain_props.account_creation_fee = asset(1000, SCORUM_SYMBOL);
        chain_props.maximum_block_size = 500;
        public_key_type signing_key = public_key_type();

        create_account();

        const account_object& account = db.obtain_service<dbs_account>().get_account(user.name);

        const witness_object& witness = witness_service.create_witness(account.name, url, signing_key, chain_props);

        BOOST_REQUIRE(witness.signing_key == signing_key);
        BOOST_REQUIRE_EQUAL(witness.url, url);
        BOOST_REQUIRE_EQUAL(witness.proposed_chain_props.account_creation_fee, chain_props.account_creation_fee);
        BOOST_REQUIRE_EQUAL(witness.proposed_chain_props.maximum_block_size, chain_props.maximum_block_size);
    }
    FC_LOG_AND_RETHROW()
}

SCORUM_TEST_CASE(check_update_witness)
{
    try
    {
        const char* url = "http://my_url";
        chain_properties chain_props;
        chain_props.account_creation_fee = asset(1000, SCORUM_SYMBOL);
        chain_props.maximum_block_size = 500;
        public_key_type signing_key = private_key_type::regenerate(fc::sha256::hash(user.name)).get_public_key();

        create_account();

        const account_object& account = db.obtain_service<dbs_account>().get_account(user.name);

        const witness_object& witness
            = witness_service.create_witness(account.name, "", signing_key, chain_properties());
        BOOST_REQUIRE(witness.signing_key == signing_key);
        BOOST_REQUIRE_EQUAL(witness.url, "");
        BOOST_REQUIRE_EQUAL(witness.proposed_chain_props.account_creation_fee, SCORUM_MIN_ACCOUNT_CREATION_FEE);
        BOOST_REQUIRE_EQUAL(witness.proposed_chain_props.maximum_block_size, (uint32_t)SCORUM_MIN_BLOCK_SIZE_LIMIT * 2);

        signing_key = private_key_type::regenerate(fc::sha256::hash("abrakadabra")).get_public_key();

        witness_service.update_witness(witness, url, signing_key, chain_props);
        BOOST_REQUIRE(witness.signing_key == signing_key);
        BOOST_REQUIRE_EQUAL(witness.url, url);
        BOOST_REQUIRE_EQUAL(witness.proposed_chain_props.account_creation_fee, chain_props.account_creation_fee);
        BOOST_REQUIRE_EQUAL(witness.proposed_chain_props.maximum_block_size, chain_props.maximum_block_size);

        // check signing_key reset
        witness_service.update_witness(witness, url, public_key_type(), chain_props);
        BOOST_REQUIRE(witness.signing_key == public_key_type());
    }
    FC_LOG_AND_RETHROW()
}

SCORUM_TEST_CASE(check_signing_key_reset)
{
    try
    {
        public_key_type signing_key = private_key_type::regenerate(fc::sha256::hash(user.name)).get_public_key();

        create_account();

        const account_object& account = db.obtain_service<dbs_account>().get_account(user.name);

        const witness_object& witness
            = witness_service.create_witness(account.name, "", signing_key, chain_properties());
        BOOST_REQUIRE(witness.signing_key == signing_key);

        // check signing_key reset
        witness_service.update_witness(witness, "", public_key_type(), chain_properties());
        BOOST_REQUIRE(witness.signing_key == public_key_type());
    }
    FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_SUITE_END()

#endif
