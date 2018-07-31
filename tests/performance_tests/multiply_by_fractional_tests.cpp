#include <boost/test/unit_test.hpp>

#include "defines.hpp"

#include <scorum/utils/extra_high_bit_operations.hpp>

#include "performance_common.hpp"

namespace multiply_by_fractional_tests {

using performance_common::cpu_profiler;

BOOST_AUTO_TEST_SUITE(get_discussions_performance_tests)

SCORUM_TEST_CASE(diff_with_optimized_check)
{
    cpu_profiler prof;

    scorum::utils::multiply_by_fractional(1000, 20, 100);

    // TODO

    BOOST_REQUIRE_LE(prof.elapses(), 100);
}

BOOST_AUTO_TEST_SUITE_END()
}
