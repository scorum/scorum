#include <boost/test/unit_test.hpp>

#include <scorum/chain/services/withdraw_vesting.hpp>
#include <scorum/chain/services/dev_pool.hpp>

#include <scorum/chain/evaluators/withdraw_vesting_evaluator.hpp>

#include <scorum/chain/schema/withdraw_vesting_objects.hpp>
#include <scorum/chain/schema/dev_committee_object.hpp>

#include "withdraw_vesting_check_common.hpp"

using namespace scorum::protocol;
using namespace scorum::chain;

BOOST_AUTO_TEST_SUITE(withdraw_vesting_route_from_dev_pool_to_dev_pool_tests)

struct withdraw_vesting_route_from_dev_pool_to_dev_pool_tests_fixture : public withdraw_vesting_check_fixture
{
    withdraw_vesting_route_from_dev_pool_to_dev_pool_tests_fixture()
    {
        pool_to_withdraw_sp = ASSET_SP(1e+4) * SCORUM_VESTING_WITHDRAW_INTERVALS;
        pool_to_withdraw_scr = asset(pool_to_withdraw_sp.amount, SCORUM_SYMBOL);
        create_dev_pool(pool_to_withdraw_sp);
    }

    asset pool_to_withdraw_sp;
    asset pool_to_withdraw_scr;
};

BOOST_FIXTURE_TEST_CASE(withdraw_all_check, withdraw_vesting_route_from_dev_pool_to_dev_pool_tests_fixture)
{
    const auto& pool = pool_service.get();

    db_plugin->debug_update(
        [&](database&) {
            withdraw_vesting_dev_pool_task create_withdraw;
            withdraw_vesting_context ctx(db, pool_to_withdraw_sp);
            create_withdraw.apply(ctx);
        },
        default_skip);

    fc::time_point_sec start_time = db.head_block_time();

    auto next_withdrawal = db.head_block_time() + SCORUM_VESTING_WITHDRAW_INTERVAL_SECONDS;

    generate_blocks(next_withdrawal + (SCORUM_BLOCK_INTERVAL / 2), true);

    BOOST_CHECK_EQUAL(pool.scr_balance, pool_to_withdraw_scr / SCORUM_VESTING_WITHDRAW_INTERVALS);

    BOOST_REQUIRE(withdraw_vesting_service.is_exists(pool.id));

    for (int ci = 1; ci < SCORUM_VESTING_WITHDRAW_INTERVALS; ++ci)
    {
        next_withdrawal = db.head_block_time() + SCORUM_VESTING_WITHDRAW_INTERVAL_SECONDS;
        generate_blocks(next_withdrawal + (SCORUM_BLOCK_INTERVAL / 2), true);
    }

    BOOST_REQUIRE(!withdraw_vesting_service.is_exists(pool.id));

    BOOST_CHECK_EQUAL(pool.scr_balance, pool_to_withdraw_scr);

    fc::time_point_sec end_time = db.head_block_time();

    BOOST_CHECK_EQUAL((end_time - start_time).to_seconds(),
                      SCORUM_VESTING_WITHDRAW_INTERVAL_SECONDS * SCORUM_VESTING_WITHDRAW_INTERVALS);
}

BOOST_AUTO_TEST_SUITE_END()
