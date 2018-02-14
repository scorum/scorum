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
    initializator_context ctx(*pservices, input_genesis);
    BOOST_REQUIRE_NO_THROW(test_it.apply(ctx));
}

BOOST_FIXTURE_TEST_CASE(check_empty_founders_list, genesis_initiate_founders_fixture)
{
    genesis_state_type input_genesis = Genesis::create().founders_supply(ASSET_SP(1e+6)).generate();
    initializator_context ctx(*pservices, input_genesis);
    SCORUM_REQUIRE_THROW(test_it.apply(ctx), fc::assert_exception);
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
        initializator_context ctx(*pservices, input_genesis);
        BOOST_REQUIRE_NO_THROW(required_i.apply(ctx));
    }

    fake_account_object alice;
    fake_account_object bob;
    fake_account_object mike;
    fake_account_object jake;

    accounts_initializator_impl required_i;
};

// TODO: genesis_initiate_founders_with_actors_fixture

BOOST_AUTO_TEST_SUITE_END()
