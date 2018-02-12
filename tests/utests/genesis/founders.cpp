#include <boost/test/unit_test.hpp>

#include <scorum/chain/genesis/genesis_state.hpp>
#include <scorum/chain/genesis/initializators/accounts_initializator.hpp>
#include <scorum/chain/genesis/initializators/founders_initializator.hpp>

#include <scorum/chain/data_service_factory.hpp>

#include <scorum/chain/services/account.hpp>
#include <scorum/chain/services/dynamic_global_property.hpp>

#include "defines.hpp"
#include "genesis.hpp"
#include "fakes.hpp"

#include <hippomocks.h>

using namespace scorum::chain;
using namespace scorum::chain::genesis;
using namespace scorum::protocol;

struct genesis_initiate_founders_fixture
{
    MockRepository mocks;

    data_service_factory_i* pservices = mocks.Mock<data_service_factory_i>();

    account_service_i* paccount_service = mocks.Mock<account_service_i>();
    dynamic_global_property_service_i* pdgp_service = mocks.Mock<dynamic_global_property_service_i>();

    genesis_initiate_founders_fixture()
    {
        mocks.ExpectCall(pservices, data_service_factory_i::account_service).ReturnByRef(*paccount_service);
        mocks.ExpectCall(pservices, data_service_factory_i::dynamic_global_property_service).ReturnByRef(*pdgp_service);
    }

    founders_initializator_impl test_it;
};

BOOST_AUTO_TEST_SUITE(founders_initializator_tests)

BOOST_FIXTURE_TEST_CASE(check_empty_genesis, genesis_initiate_founders_fixture)
{
    genesis_state_type input_genesis = Genesis::create().generate();

    BOOST_REQUIRE_NO_THROW(test_it.apply(*pservices, input_genesis));
}

BOOST_FIXTURE_TEST_CASE(check_empty_founders_list, genesis_initiate_founders_fixture)
{
    genesis_state_type input_genesis = Genesis::create().founders_supply(ASSET_SP(1e+6)).generate();

    SCORUM_REQUIRE_THROW(test_it.apply(*pservices, input_genesis), fc::assert_exception);
}

struct genesis_initiate_founders_with_actors_fixture : public genesis_initiate_founders_fixture
{
    genesis_initiate_founders_with_actors_fixture()
        : alice("alice")
        , bob("bob")
        , mike("mike")
        , jake("jake")
    {
    }

    void init_required()
    {
        genesis_state_type input_genesis
            = Genesis::create().accounts(alice.config, bob.config, mike.config, jake.config).generate();

        BOOST_REQUIRE_NO_THROW(required_i.apply(*pservices, input_genesis));
    }

    fake_account_object alice;
    fake_account_object bob;
    fake_account_object mike;
    fake_account_object jake;

    accounts_initializator_impl required_i;
};

/*
BOOST_FIXTURE_TEST_CASE(check_invalid_founders_sum, genesis_initiate_founders_with_actors_fixture)
{
    init_required();

    asset total_sp = ASSET_SP(1e+6);
    uint16_t total = 100u;
    uint16_t pie = total / 2u;

    alice.config.percent(pie);
    mike.config.percent(pie);
    jake.config.percent(pie); // 150%

    genesis_state_type input_genesis = Genesis::create()
                                           .founders(alice.config, bob.config, mike.config, jake.config)
                                           .founders_supply(total_sp)
                                           .generate();

    SCORUM_REQUIRE_THROW(test_it.apply(*pservices, input_genesis), fc::assert_exception);
}

BOOST_FIXTURE_TEST_CASE(check_valid_founders_sum, genesis_initiate_founders_with_actors_fixture)
{
    init_required();

    asset total_sp = ASSET_SP(1e+6);
    uint16_t total = 100u;
    uint16_t pie = total / 4u;

    alice.config.percent(pie);
    bob.config.percent(pie);
    mike.config.percent(pie);
    jake.config.percent(total - pie); // 100%

    genesis_state_type input_genesis = Genesis::create()
                                           .founders(alice.config, bob.config, mike.config, jake.config)
                                           .founders_supply(total_sp)
                                           .generate();

    BOOST_REQUIRE_NO_THROW(test_it.apply(*pservices, input_genesis));
}
*/

BOOST_AUTO_TEST_SUITE_END()
