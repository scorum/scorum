#include <boost/test/unit_test.hpp>

#include <scorum/chain/services/account.hpp>
#include <scorum/chain/services/withdraw_vesting.hpp>
#include <scorum/chain/services/dev_pool.hpp>

#include <scorum/chain/evaluators/withdraw_vesting_evaluator.hpp>
#include <scorum/chain/evaluators/set_withdraw_vesting_route_evaluators.hpp>

#include <scorum/chain/schema/account_objects.hpp>
#include <scorum/chain/schema/withdraw_vesting_objects.hpp>
#include <scorum/chain/schema/dev_committee_object.hpp>

#include "withdraw_vesting_check_common.hpp"

using namespace scorum::protocol;
using namespace scorum::chain;

BOOST_AUTO_TEST_SUITE(withdraw_vesting_route_from_dev_pool_to_account_tests)

struct withdraw_vesting_route_from_dev_pool_to_account_tests_fixture : public withdraw_vesting_check_fixture
{
    withdraw_vesting_route_from_dev_pool_to_account_tests_fixture()
    {
        pool_to_withdraw_sp = ASSET_SP(1e+4) * SCORUM_VESTING_WITHDRAW_INTERVALS;
        pool_to_withdraw_scr = asset(pool_to_withdraw_sp.amount, SCORUM_SYMBOL);
        create_dev_pool(pool_to_withdraw_sp);

        ACTORS((bob)(sam));

        generate_block();
    }

    asset pool_to_withdraw_sp;
    asset pool_to_withdraw_scr;
};

BOOST_FIXTURE_TEST_CASE(withdrawal_tree_check, withdraw_vesting_route_from_dev_pool_to_account_tests_fixture)
{
    static const int bob_pie_percent = 10;
    static const int sam_pie_percent = 20;
    static const int pool_withdrawal_percent = 100 - (bob_pie_percent + sam_pie_percent);

    const auto& pool = pool_service.get();
    const auto& bob = account_service.get_account("bob");
    const auto& sam = account_service.get_account("sam");

    asset old_pool_balance_scr = pool.scr_balance;
    asset old_bob_balance_scr = bob.balance;
    asset old_sam_balance_sp = sam.vesting_shares;

    db_plugin->debug_update(
        [&](database&) {
            withdraw_vesting_dev_pool_task create_withdraw;
            withdraw_vesting_context ctx(db, pool_to_withdraw_sp);
            create_withdraw.apply(ctx);
        },
        default_skip);

    db_plugin->debug_update(
        [&](database&) {
            set_withdraw_vesting_route_from_dev_pool_task create_withdraw_route;
            set_withdraw_vesting_route_context ctx(db, bob.name, bob_pie_percent * SCORUM_1_PERCENT, false);
            create_withdraw_route.apply(ctx);
        },
        default_skip);

    db_plugin->debug_update(
        [&](database&) {
            set_withdraw_vesting_route_from_dev_pool_task create_withdraw_route;
            set_withdraw_vesting_route_context ctx(db, sam.name, sam_pie_percent * SCORUM_1_PERCENT, true);
            create_withdraw_route.apply(ctx);
        },
        default_skip);

    fc::time_point_sec start_time = db.head_block_time();

    auto next_withdrawal = db.head_block_time() + SCORUM_VESTING_WITHDRAW_INTERVAL_SECONDS;

    generate_blocks(next_withdrawal + (SCORUM_BLOCK_INTERVAL / 2), true);

    BOOST_CHECK_EQUAL(pool.scr_balance - old_pool_balance_scr,
                      pool_to_withdraw_scr * pool_withdrawal_percent / 100 / SCORUM_VESTING_WITHDRAW_INTERVALS);
    BOOST_CHECK_EQUAL(bob.balance - old_bob_balance_scr,
                      pool_to_withdraw_scr * bob_pie_percent / 100 / SCORUM_VESTING_WITHDRAW_INTERVALS);
    asset sam_recived = pool_to_withdraw_scr * sam_pie_percent / 100 / SCORUM_VESTING_WITHDRAW_INTERVALS;
    BOOST_CHECK_EQUAL(sam.vesting_shares - old_sam_balance_sp, asset(sam_recived.amount, VESTS_SYMBOL));

    BOOST_REQUIRE(withdraw_vesting_service.is_exists(pool.id));

    for (int ci = 1; ci < SCORUM_VESTING_WITHDRAW_INTERVALS; ++ci)
    {
        next_withdrawal = db.head_block_time() + SCORUM_VESTING_WITHDRAW_INTERVAL_SECONDS;
        generate_blocks(next_withdrawal + (SCORUM_BLOCK_INTERVAL / 2), true);
    }

    BOOST_REQUIRE(!withdraw_vesting_service.is_exists(pool.id));

    BOOST_CHECK_EQUAL(pool.scr_balance - old_pool_balance_scr, pool_to_withdraw_scr * pool_withdrawal_percent / 100);
    BOOST_CHECK_EQUAL(bob.balance - old_bob_balance_scr, pool_to_withdraw_scr * bob_pie_percent / 100);
    sam_recived = pool_to_withdraw_scr * sam_pie_percent / 100;
    BOOST_CHECK_EQUAL(sam.vesting_shares - old_sam_balance_sp, asset(sam_recived.amount, VESTS_SYMBOL));

    fc::time_point_sec end_time = db.head_block_time();

    BOOST_CHECK_EQUAL((end_time - start_time).to_seconds(),
                      SCORUM_VESTING_WITHDRAW_INTERVAL_SECONDS * SCORUM_VESTING_WITHDRAW_INTERVALS);
}

BOOST_AUTO_TEST_SUITE_END()
