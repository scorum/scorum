#include <boost/test/unit_test.hpp>

#include <scorum/chain/services/budgets.hpp>
#include <scorum/protocol/types.hpp>
#include <scorum/protocol/asset.hpp>

#include "defines.hpp"

namespace {

using namespace scorum::chain;

BOOST_AUTO_TEST_SUITE(per_block_evaluation_tests)

SCORUM_TEST_CASE(start_time_aligned_deadline_is_not_test)
{
    auto start = fc::time_point_sec(10);
    auto deadline = fc::time_point_sec(15);
    auto block_time = fc::time_point_sec(7);

    auto per_block = scorum::chain::detail::adv_calculate_per_block(start, deadline, block_time, ASSET_SCR(900));
    BOOST_CHECK_EQUAL(per_block.amount, 300u);
}

SCORUM_TEST_CASE(start_time_is_not_aligned_deadline_is_test)
{
    auto start = fc::time_point_sec(8);
    auto deadline = fc::time_point_sec(16);
    auto block_time = fc::time_point_sec(7);

    auto per_block = scorum::chain::detail::adv_calculate_per_block(start, deadline, block_time, ASSET_SCR(900));
    BOOST_CHECK_EQUAL(per_block.amount, 300u);
}

SCORUM_TEST_CASE(start_time_and_deadline_are_not_aligned_test)
{
    auto start = fc::time_point_sec(8);
    auto deadline = fc::time_point_sec(14);
    auto block_time = fc::time_point_sec(7);

    auto per_block = scorum::chain::detail::adv_calculate_per_block(start, deadline, block_time, ASSET_SCR(900));
    BOOST_CHECK_EQUAL(per_block.amount, 300u);
}

SCORUM_TEST_CASE(balance_do_not_divide_completely_test)
{
    auto start = fc::time_point_sec(10);
    auto deadline = fc::time_point_sec(19);
    auto block_time = fc::time_point_sec(7);

    auto per_block = scorum::chain::detail::adv_calculate_per_block(start, deadline, block_time, ASSET_SCR(90));
    BOOST_CHECK_EQUAL(per_block.amount, 22u);
}

SCORUM_TEST_CASE(start_time_equal_deadline_test)
{
    auto start = fc::time_point_sec(10);
    auto deadline = fc::time_point_sec(10);
    auto block_time = fc::time_point_sec(7);

    auto per_block = scorum::chain::detail::adv_calculate_per_block(start, deadline, block_time, ASSET_SCR(90));
    BOOST_CHECK_EQUAL(per_block.amount, 90u);
}

SCORUM_TEST_CASE(balance_is_too_small_to_fit_each_per_block_payment_test)
{
    auto start = fc::time_point_sec(10);
    auto deadline = fc::time_point_sec(300);
    auto block_time = fc::time_point_sec(7);

    auto per_block = scorum::chain::detail::adv_calculate_per_block(start, deadline, block_time, ASSET_SCR(90));
    BOOST_CHECK_EQUAL(per_block.amount, 1u);
}

BOOST_AUTO_TEST_SUITE_END()
}
