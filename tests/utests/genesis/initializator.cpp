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

BOOST_FIXTURE_TEST_CASE(test_appled, genesis_initializator_fixture)
{
    test_initializator intz;

    intz.apply(ctx);

    BOOST_CHECK(intz.is_applied());
}

BOOST_FIXTURE_TEST_CASE(test_double_apply, genesis_initializator_fixture)
{
    test_initializator intz;

    intz.apply(ctx);
    BOOST_CHECK_NO_THROW(intz.apply(ctx));

    BOOST_CHECK(intz.is_applied());
}

BOOST_FIXTURE_TEST_CASE(test_after_appled, genesis_initializator_fixture)
{
    test_initializator intz_a;
    test_initializator intz_b;

    intz_a.after(intz_b).apply(ctx);

    BOOST_CHECK(intz_a.is_applied());
    BOOST_CHECK(intz_b.is_applied());
}

struct test_initializator_order : public test_initializator
{
    void on_apply(initializator_context& ctx)
    {
        test_initializator::on_apply(ctx);
        _order = ++_next_order;
    }

    int order() const
    {
        return _order;
    }

private:
    static int _next_order;
    int _order = 0;
};

int test_initializator_order::_next_order = 0;

BOOST_FIXTURE_TEST_CASE(test_after_order, genesis_initializator_fixture)
{
    test_initializator_order intz_a;
    test_initializator_order intz_b;

    intz_a.after(intz_b).apply(ctx);

    BOOST_CHECK_LT(intz_b.order(), intz_a.order());
}

BOOST_AUTO_TEST_SUITE_END()
