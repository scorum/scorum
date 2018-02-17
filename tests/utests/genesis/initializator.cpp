#include <boost/test/unit_test.hpp>

#include <scorum/chain/genesis/initializators/initializators.hpp>
#include <scorum/chain/data_service_factory.hpp>
#include <scorum/chain/genesis/genesis_state.hpp>
#include <fc/exception/exception.hpp>

#include <hippomocks.h>

#include "defines.hpp"

using scorum::chain::genesis::initializator;
using scorum::chain::genesis::initializator_context;
using scorum::chain::data_service_factory_i;
using scorum::chain::genesis_state_type;

struct test_initializator : public initializator
{
    void on_apply(initializator_context&)
    {
        FC_ASSERT(!_test_applied);
        _test_applied = true;
    }

    bool is_applied() const
    {
        return _test_applied;
    }

private:
    bool _test_applied = false;
};

struct genesis_initializator_fixture
{
    genesis_initializator_fixture()
        : ctx(*pservices, gs)
    {
    }

    initializator_context ctx;

private:
    MockRepository mocks;

    data_service_factory_i* pservices = mocks.Mock<data_service_factory_i>();
    genesis_state_type gs;
};

BOOST_AUTO_TEST_SUITE(genesis_initializator_tests)

BOOST_FIXTURE_TEST_CASE(test_applied, genesis_initializator_fixture)
{
    test_initializator intz;

    intz.apply(ctx);

    BOOST_CHECK(intz.is_applied());
}

BOOST_FIXTURE_TEST_CASE(test_double_apply, genesis_initializator_fixture)
{
    test_initializator intz;

    intz.apply(ctx);
    BOOST_CHECK_NO_THROW(intz.apply(ctx)); // on_apply must not be called

    BOOST_CHECK(intz.is_applied());
}

BOOST_FIXTURE_TEST_CASE(test_chain_applied, genesis_initializator_fixture)
{
    const int sz = 5;
    test_initializator intz[sz];

    intz[0].before(intz[1]).before(intz[2]).after(intz[3]).after(intz[4]).apply(ctx);

    for (int ci = 0; ci < sz; ++ci)
    {
        BOOST_CHECK(intz[ci].is_applied());
    }
}

BOOST_AUTO_TEST_SUITE_END()
