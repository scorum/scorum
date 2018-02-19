#include <boost/test/unit_test.hpp>

#include <scorum/chain/tasks_base.hpp>

using scorum::chain::task;
using scorum::chain::task_censor_i;

struct test_context
{
    bool stop_apply = false;
    bool test_censor_applied = false;
    int next_order = 0;
};

struct test_censor : public task_censor_i<test_context>
{
    virtual bool is_allowed(test_context& ctx)
    {
        return !ctx.stop_apply;
    }
    virtual void apply(test_context& ctx)
    {
        ctx.test_censor_applied = true;
    }
};

struct test_task : public task<test_context>
{
    test_task()
    {
        set_censor(&_censor);
    }

    void on_apply(test_context&)
    {
        _times_applied++;
    }

    const int times_applied() const
    {
        return _times_applied;
    }

private:
    int _times_applied = 0;
    test_censor _censor;
};

BOOST_AUTO_TEST_SUITE(tasks_base_tests)

struct tasks_base_tests_fixture
{
    test_context ctx;
};

BOOST_FIXTURE_TEST_CASE(test_censored_apply, tasks_base_tests_fixture)
{
    test_task t;

    t.apply(ctx);
    BOOST_CHECK_EQUAL(t.times_applied(), 1);

    t.apply(ctx);
    BOOST_CHECK_EQUAL(t.times_applied(), 2);

    ctx.stop_apply = true;
    t.apply(ctx);
    BOOST_CHECK_EQUAL(t.times_applied(), 2);
}

BOOST_FIXTURE_TEST_CASE(test_after_applied, tasks_base_tests_fixture)
{
    test_task t_a;
    test_task t_b;

    t_a.after(t_b).apply(ctx);

    BOOST_CHECK_EQUAL(t_a.times_applied(), 1);
    BOOST_CHECK_EQUAL(t_b.times_applied(), 1);
}

BOOST_FIXTURE_TEST_CASE(test_before_applied, tasks_base_tests_fixture)
{
    test_task t_a;
    test_task t_b;

    t_a.before(t_b).apply(ctx);

    BOOST_CHECK_EQUAL(t_a.times_applied(), 1);
    BOOST_CHECK_EQUAL(t_b.times_applied(), 1);
}

BOOST_FIXTURE_TEST_CASE(test_chain_appled, tasks_base_tests_fixture)
{
    const int sz = 5;
    test_task t[sz];

    t[0].before(t[1]).before(t[2]).after(t[3]).after(t[4]).apply(ctx);

    for (int ci = 0; ci < sz; ++ci)
    {
        BOOST_CHECK_EQUAL(t[ci].times_applied(), 1);
    }
}

struct test_task_order : public test_task
{
    void on_apply(test_context& ctx)
    {
        test_task::on_apply(ctx);
        test_order = ++ctx.next_order;
    }

    int test_order = -1;
};

BOOST_FIXTURE_TEST_CASE(test_after_order, tasks_base_tests_fixture)
{
    test_task_order t_a;
    test_task_order t_b;

    t_a.after(t_b).apply(ctx);

    BOOST_CHECK_GT(t_a.test_order, t_b.test_order);
}

BOOST_FIXTURE_TEST_CASE(test_before_order, tasks_base_tests_fixture)
{
    test_task_order t_a;
    test_task_order t_b;

    t_a.before(t_b).apply(ctx);

    BOOST_CHECK_GT(t_b.test_order, t_a.test_order);
}

BOOST_FIXTURE_TEST_CASE(test_chain_order, tasks_base_tests_fixture)
{
    const int sz = 5;
    test_task_order t[sz];

    t[0].before(t[1]).before(t[2]).after(t[3]).after(t[4]).apply(ctx);

    BOOST_CHECK_LT(t[0].test_order, t[1].test_order);
    BOOST_CHECK_LT(t[0].test_order, t[2].test_order);

    BOOST_CHECK_LT(t[1].test_order, t[2].test_order);

    BOOST_CHECK_GT(t[0].test_order, t[3].test_order);
    BOOST_CHECK_GT(t[0].test_order, t[4].test_order);

    BOOST_CHECK_LT(t[3].test_order, t[4].test_order);
}

BOOST_AUTO_TEST_SUITE_END()
