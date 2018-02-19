#include <boost/test/unit_test.hpp>

#include <scorum/chain/genesis/initializators/initializators.hpp>
#include <scorum/chain/data_service_factory.hpp>
#include <scorum/chain/genesis/genesis_state.hpp>

#include <hippomocks.h>

using scorum::chain::genesis::initializator;
using scorum::chain::genesis::initializator_context;
using scorum::chain::data_service_factory_i;
using scorum::chain::genesis_state_type;

struct test_initializator : public initializator
{
    void on_apply(initializator_context&)
    {
        _times_applied++;
    }

    const int times_applied() const
    {
        return _times_applied;
    }

private:
    int _times_applied = 0;
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
    BOOST_CHECK_EQUAL(intz.times_applied(), 1);
    intz.apply(ctx);
    BOOST_CHECK_EQUAL(intz.times_applied(), 1);
}

BOOST_FIXTURE_TEST_CASE(test_chain_applied, genesis_initializator_fixture)
{
    const int sz = 5;
    test_initializator intz[sz];

    intz[0].before(intz[1]).before(intz[2]).after(intz[3]).after(intz[4]).apply(ctx);

    for (int ci = 0; ci < sz; ++ci)
    {
        BOOST_CHECK_EQUAL(intz[ci].times_applied(), 1);
    }
}

BOOST_AUTO_TEST_SUITE_END()
