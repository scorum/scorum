#include <boost/test/unit_test.hpp>

#include <scorum/chain/services/account.hpp>
#include <scorum/chain/services/withdraw_scorumpower.hpp>
#include <scorum/chain/services/dev_pool.hpp>

#include <scorum/chain/evaluators/withdraw_scorumpower_evaluator.hpp>
#include <scorum/chain/evaluators/set_withdraw_scorumpower_route_evaluators.hpp>

#include <scorum/chain/schema/account_objects.hpp>
#include <scorum/chain/schema/withdraw_scorumpower_objects.hpp>
#include <scorum/chain/schema/dev_committee_object.hpp>

#include "withdraw_scorumpower_check_common.hpp"

using namespace database_fixture;

BOOST_AUTO_TEST_SUITE(withdraw_scorumpower_route_from_dev_pool_to_account_tests)

struct withdraw_scorumpower_route_from_dev_pool_to_account_tests_fixture : public withdraw_scorumpower_check_fixture
{
    withdraw_scorumpower_route_from_dev_pool_to_account_tests_fixture()
    {
        pool_to_withdraw_sp = ASSET_SP(1e+4) * SCORUM_VESTING_WITHDRAW_INTERVALS;
        pool_to_withdraw_scr = asset(pool_to_withdraw_sp.amount, SCORUM_SYMBOL);
        set_dev_pool_balance(pool_to_withdraw_sp);

        ACTORS((bob)(sam));

        generate_block();
    }

    asset pool_to_withdraw_sp;
    asset pool_to_withdraw_scr;
};

BOOST_FIXTURE_TEST_CASE(withdrawal_tree_check, withdraw_scorumpower_route_from_dev_pool_to_account_tests_fixture)
{
    static const int bob_pie_percent = 10;
    static const int sam_pie_percent = 20;
    static const int pool_withdrawal_percent = 100 - (bob_pie_percent + sam_pie_percent);

    const auto& pool = pool_service.get();
    const auto& bob = account_service.get_account("bob");
    const auto& sam = account_service.get_account("sam");

    asset old_pool_balance_scr = pool.scr_balance;
    asset old_bob_balance_scr = bob.balance;
    asset old_sam_balance_sp = sam.scorumpower;

    // manually trigger 'blockchain_monitoring_plugin' with 'proposal_virtual_operation' operation
    proposal_virtual_operation op;
    development_committee_withdraw_vesting_operation inner_op;
    inner_op.vesting_shares = pool_to_withdraw_sp;
    op.op = inner_op;

    db_plugin->debug_update(
        [&](database&) {
            db.notify_pre_apply_operation(op);

            withdraw_scorumpower_dev_pool_task create_withdraw;
            withdraw_scorumpower_context ctx(db, pool_to_withdraw_sp);
            create_withdraw.apply(ctx);

            db.notify_post_apply_operation(op);
        },
        default_skip);

    db_plugin->debug_update(
        [&](database&) {
            set_withdraw_scorumpower_route_from_dev_pool_task create_withdraw_route;
            set_withdraw_scorumpower_route_context ctx(db, bob.name, bob_pie_percent * SCORUM_1_PERCENT, false);
            create_withdraw_route.apply(ctx);
        },
        default_skip);

    db_plugin->debug_update(
        [&](database&) {
            set_withdraw_scorumpower_route_from_dev_pool_task create_withdraw_route;
            set_withdraw_scorumpower_route_context ctx(db, sam.name, sam_pie_percent * SCORUM_1_PERCENT, true);
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
    BOOST_CHECK_EQUAL(sam.scorumpower - old_sam_balance_sp, asset(sam_recived.amount, SP_SYMBOL));

    BOOST_REQUIRE(withdraw_scorumpower_service.is_exists(pool.id));

    for (uint32_t ci = 1; ci < SCORUM_VESTING_WITHDRAW_INTERVALS; ++ci)
    {
        next_withdrawal = db.head_block_time() + SCORUM_VESTING_WITHDRAW_INTERVAL_SECONDS;
        generate_blocks(next_withdrawal + (SCORUM_BLOCK_INTERVAL / 2), true);
    }

    BOOST_REQUIRE(!withdraw_scorumpower_service.is_exists(pool.id));

    BOOST_CHECK_EQUAL(pool.scr_balance - old_pool_balance_scr, pool_to_withdraw_scr * pool_withdrawal_percent / 100);
    BOOST_CHECK_EQUAL(bob.balance - old_bob_balance_scr, pool_to_withdraw_scr * bob_pie_percent / 100);
    sam_recived = pool_to_withdraw_scr * sam_pie_percent / 100;
    BOOST_CHECK_EQUAL(sam.scorumpower - old_sam_balance_sp, asset(sam_recived.amount, SP_SYMBOL));

    fc::time_point_sec end_time = db.head_block_time();

    BOOST_CHECK_EQUAL((end_time - start_time).to_seconds(),
                      SCORUM_VESTING_WITHDRAW_INTERVAL_SECONDS * SCORUM_VESTING_WITHDRAW_INTERVALS);

    BOOST_REQUIRE_EQUAL(api.get_lifetime_stats().finished_vesting_withdrawals, 1u);
    BOOST_REQUIRE_EQUAL(api.get_lifetime_stats().scorumpower_withdrawn,
                        pool_to_withdraw_sp.amount * (100 - sam_pie_percent) / 100);
    BOOST_REQUIRE_EQUAL(api.get_lifetime_stats().scorumpower_transferred,
                        pool_to_withdraw_sp.amount * sam_pie_percent / 100);
    BOOST_REQUIRE_EQUAL(api.get_lifetime_stats().vesting_withdraw_rate_delta, 0);
    BOOST_REQUIRE_EQUAL(api.get_lifetime_stats().vesting_withdrawals_processed, SCORUM_VESTING_WITHDRAW_INTERVALS * 3);
}

BOOST_AUTO_TEST_SUITE_END()
