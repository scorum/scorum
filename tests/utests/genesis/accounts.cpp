#include <boost/test/unit_test.hpp>

#include <scorum/chain/genesis/genesis_state.hpp>
#include <scorum/chain/genesis/initializators/accounts_initializator.hpp>

#include <scorum/chain/data_service_factory.hpp>
#include <scorum/chain/services/account.hpp>

#include "defines.hpp"
#include "genesis.hpp"
#include "fakes.hpp"

#include <hippomocks.h>

using namespace scorum::chain;
using namespace scorum::chain::genesis;
using namespace scorum::protocol;

struct genesis_initiate_accounts_fixture
{
    MockRepository mocks;

    data_service_factory_i* pservices = mocks.Mock<data_service_factory_i>();

    account_service_i* paccount_service = mocks.Mock<account_service_i>();

    genesis_initiate_accounts_fixture()
    {
        mocks.ExpectCall(pservices, data_service_factory_i::account_service).ReturnByRef(*paccount_service);
    }

    accounts_initializator_impl test_it;
};

BOOST_AUTO_TEST_SUITE(accounts_initializator_tests)

BOOST_FIXTURE_TEST_CASE(check_empty_genesis, genesis_initiate_accounts_fixture)
{
    genesis_state_type input_genesis = Genesis::create().generate();

    BOOST_REQUIRE_NO_THROW(test_it.apply(*pservices, input_genesis));
}

BOOST_FIXTURE_TEST_CASE(check_empty_account_list, genesis_initiate_accounts_fixture)
{
    genesis_state_type input_genesis = Genesis::create().accounts_supply(ASSET_SCR(1e+6)).generate();

    SCORUM_REQUIRE_THROW(test_it.apply(*pservices, input_genesis), fc::assert_exception);
}

struct genesis_initiate_accounts_with_actors_fixture : public genesis_initiate_accounts_fixture
{
    genesis_initiate_accounts_with_actors_fixture()
        : alice("alice")
        , bob("bob")
    {
    }

    fake_account_object alice;
    fake_account_object bob;
};

/*
BOOST_FIXTURE_TEST_CASE(check_invalid_account_sum, genesis_initiate_accounts_with_actors_fixture)
{
    asset total = ASSET_SCR(1e+6);
    asset pie = total / 2;

    alice.config.scorum(pie);

    genesis_state_type input_genesis
        = Genesis::create().accounts(alice.config, bob.config).accounts_supply(total).generate();

    SCORUM_REQUIRE_THROW(test_it.apply(*pservices, input_genesis), fc::assert_exception);
}

BOOST_FIXTURE_TEST_CASE(check_valid_account_sum, genesis_initiate_accounts_with_actors_fixture)
{
    asset total = ASSET_SCR(1e+6);
    asset pie = total / 2;

    alice.config.scorum(pie);
    bob.config.scorum(total - pie);

    genesis_state_type input_genesis
        = Genesis::create().accounts(alice.config, bob.config).accounts_supply(total).generate();

    BOOST_REQUIRE_NO_THROW(test_it.apply(*pservices, input_genesis));
}
*/

BOOST_AUTO_TEST_SUITE_END()
