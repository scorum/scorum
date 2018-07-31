#include <boost/test/unit_test.hpp>

#include "defines.hpp"

#include <scorum/utils/extra_high_bit_operations.hpp>

#include "performance_common.hpp"

namespace multiply_by_fractional_tests {

using performance_common::cpu_profiler;

BOOST_AUTO_TEST_SUITE(multiply_by_fractional_tests)

SCORUM_TEST_CASE(diff_with_optimized_check)
{
    const size_t cycles = 100'000;

    size_t case1 = 0u;
    {
        cpu_profiler prof;

        for (size_t ci = 0; ci < cycles; ++ci)
        {
            scorum::utils::multiply_by_fractional(999 + ci, 20 + ci, 100);
        }

        case1 = prof.elapsed();
        BOOST_TEST_MESSAGE("multiply_by_fractional' with uint128 use: " << case1 << "ms");
    }

    size_t case2 = 0u;
    {
        cpu_profiler prof;

        for (size_t ci = 0; ci < cycles; ++ci)
        {
            scorum::utils::multiply_by_fractional(999 + ci, 20 + ci, 1);
        }

        case2 = prof.elapsed();
        BOOST_TEST_MESSAGE("multiply_by_fractional' with safe use: " << case2 << "ms");
    }

    BOOST_REQUIRE_LT(case2, case1);
}

BOOST_AUTO_TEST_SUITE_END()
}
