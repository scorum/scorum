#include <boost/test/unit_test.hpp>

#include <scorum/chain/database/block_tasks/block_tasks.hpp>
#include <scorum/chain/database/database_virtual_operations.hpp>
#include <scorum/chain/data_service_factory.hpp>

#include <hippomocks.h>

#if 0 // per_block_num_apply_guard - has not used

using scorum::chain::database_ns::block_task_context;
using scorum::chain::database_ns::block_task_type;
using scorum::chain::data_service_factory_i;
using scorum::chain::database_virtual_operations_emmiter_i;

template <uint32_t per_block_num> struct test_block_task : public block_task_type<per_block_num>
{
    void on_apply(block_task_context&)
    {
        _times_applied++;
    }

    const uint32_t times_applied() const
    {
        return _times_applied;
    }

private:
    uint32_t _times_applied = 0;
};

struct block_task_tests_fixture
{
    block_task_tests_fixture()
    {
        pservices = mocks.Mock<data_service_factory_i>();
        pdb = mocks.Mock<database_virtual_operations_emmiter_i>();
    }

    data_service_factory_i* pservices = nullptr;
    database_virtual_operations_emmiter_i* pdb = nullptr;

private:
    MockRepository mocks;
};

BOOST_AUTO_TEST_SUITE(block_task_tests)

BOOST_FIXTURE_TEST_CASE(test_block_per_1_applied, block_task_tests_fixture)
{
    test_block_task<1u> t;

    for (uint32_t block_num = 1u; block_num < 10u; ++block_num)
    {
        block_task_context ctx(*pservices, *pdb, block_num);

        t.apply(ctx);
        BOOST_CHECK_EQUAL(t.times_applied(), block_num);
        t.apply(ctx);
        BOOST_CHECK_EQUAL(t.times_applied(), block_num);
    }
}

BOOST_FIXTURE_TEST_CASE(test_block_per_3_applied, block_task_tests_fixture)
{
    static const uint32_t odd = 3u;

    test_block_task<odd> t;

    for (uint32_t block_num = 1u; block_num < 10u; ++block_num)
    {
        block_task_context ctx(*pservices, *pdb, block_num);

        t.apply(ctx);
        BOOST_CHECK_EQUAL(t.times_applied(), block_num / odd);
        t.apply(ctx);
        BOOST_CHECK_EQUAL(t.times_applied(), block_num / odd);
    }
}

BOOST_AUTO_TEST_SUITE_END()

#endif
