#include <boost/test/unit_test.hpp>

#include <scorum/chain/schema/account_objects.hpp>
#include <scorum/chain/schema/budget_object.hpp>
#include <scorum/chain/services/account.hpp>
#include <scorum/chain/services/budget.hpp>
#include <scorum/chain/services/dynamic_global_property.hpp>
#include <scorum/chain/services/reward_balancer.hpp>
#include <scorum/rewards_math/formulas.hpp>

#include "database_blog_integration.hpp"
#include "actor.hpp"

namespace active_sp_holders_reward_tests {

using namespace database_fixture;

class active_sp_holders_reward_fixture : public database_blog_integration_fixture
{
public:
    active_sp_holders_reward_fixture()
        : budget_service(db.obtain_service<dbs_budget>())
        , account_service(db.obtain_service<dbs_account>())
        , dprops_service(db.obtain_service<dbs_dynamic_global_property>())
        , voters_reward_sp_service(db.obtain_service<dbs_voters_reward_sp>())
        , voters_reward_scr_service(db.obtain_service<dbs_voters_reward_scr>())
        , alice("alice")
        , bob("bob")
    {
        alice.percent(70);
        bob.percent(30);

        asset founders_supply = ASSET_SP(100000000);

        genesis_state_type genesis = database_integration_fixture::default_genesis_state()
                                         .accounts(alice, bob)
                                         .founders_supply(founders_supply)
                                         .founders(alice, bob)
                                         .generate();

        open_database(genesis);

        db.post_apply_operation.connect([&](const operation_notification& note) { on_operation(note.op); });
    }

    virtual void open_database_impl(const genesis_state_type& genesis) override
    {
        database_integration_fixture::open_database_impl(genesis);

        db.set_hardfork(SCORUM_NUM_HARDFORKS);
    }

    inline asset get_active_voters_reward(const asset& total)
    {
        return total * SCORUM_ACTIVE_SP_HOLDERS_PER_BLOCK_REWARD_PERCENT / SCORUM_100_PERCENT;
    }

    asset create_advertising_budget(uint32_t number_of_block)
    {
        const auto& account = account_service.get_account(initdelegate.name);
        auto advertising_budget = ASSET_SCR(2e+9);
        auto deadline = db.get_slot_time(number_of_block);

        const auto& ret = budget_service.create_budget(account, advertising_budget, deadline);

        auto& reward_service = db.obtain_service<dbs_content_reward_scr>();
        reward_service.update(
            [&](content_reward_balancer_scr_object& rp) { rp.current_per_block_reward = advertising_budget; });

        return ret.per_block;
    }

    dbs_budget& budget_service;
    dbs_account& account_service;
    dbs_dynamic_global_property& dprops_service;
    dbs_voters_reward_sp& voters_reward_sp_service;
    dbs_voters_reward_scr& voters_reward_scr_service;

    Actor alice;
    Actor bob;

    void set_voter(const Actor& voter)
    {
        _voter = voter;
    }

    int op_times() const
    {
        return _op_times;
    }

private:
    void on_operation(const operation& op)
    {
        op.weak_visit([&](const active_sp_holders_reward_operation& o) {
            if (_voter.name != account_name_type())
            {
                BOOST_REQUIRE_EQUAL(o.sp_holder, bob.name);
            }
            ++_op_times;
        });
    }

    Actor _voter;
    int _op_times = 0;
};

using namespace scorum::chain;
using namespace scorum::protocol;

BOOST_FIXTURE_TEST_SUITE(active_sp_holders_reward_tests, active_sp_holders_reward_fixture)

SCORUM_TEST_CASE(per_block_sp_payment_from_fund_budget)
{
    asset bob_sp_before = account_service.get_account(bob.name).scorumpower;

    auto post = create_post(alice).push();
    post.vote(bob).in_block();

    auto initial_blocks = db.head_block_num();
    generate_blocks(db.head_block_time() + SCORUM_ACTIVE_SP_HOLDERS_REWARD_PERIOD);
    auto pass_blocks = db.head_block_num() - initial_blocks;

    auto active_sp_holders_reward = get_active_voters_reward(budget_service.get_fund_budget().per_block);
    active_sp_holders_reward *= pass_blocks;

    BOOST_REQUIRE_EQUAL(account_service.get_account(bob.name).scorumpower, bob_sp_before + active_sp_holders_reward);
}

SCORUM_TEST_CASE(per_block_scr_payment_from_budget)
{
    auto advertising_budget = create_advertising_budget(100);

    auto post = create_post(alice).push();
    post.vote(bob).in_block();

    auto initial_blocks = db.head_block_num();
    generate_blocks(db.head_block_time() + SCORUM_ACTIVE_SP_HOLDERS_REWARD_PERIOD);
    auto pass_blocks = db.head_block_num() - initial_blocks;

    auto dev_team_reward_scr = advertising_budget * SCORUM_DEV_TEAM_PER_BLOCK_REWARD_PERCENT / SCORUM_100_PERCENT;
    auto user_reward_scr = advertising_budget - dev_team_reward_scr;

    auto active_sp_holders_reward = get_active_voters_reward(user_reward_scr);
    active_sp_holders_reward *= pass_blocks;

    BOOST_REQUIRE_EQUAL(account_service.get_account(bob.name).balance, active_sp_holders_reward);
}

SCORUM_TEST_CASE(per_block_sp_payment_from_fund_budget_if_no_active_voters_exist)
{
    generate_block();

    auto active_sp_holders_reward = get_active_voters_reward(budget_service.get_fund_budget().per_block);

    auto& balancer = voters_reward_sp_service.get();

    BOOST_REQUIRE_EQUAL(balancer.balance, active_sp_holders_reward);
}

SCORUM_TEST_CASE(per_block_scr_payment_from_budget_if_no_active_voters_exist)
{
    auto advertising_budget = create_advertising_budget(1);

    generate_block();

    auto dev_team_reward_scr = advertising_budget * SCORUM_DEV_TEAM_PER_BLOCK_REWARD_PERCENT / SCORUM_100_PERCENT;
    auto user_reward_scr = advertising_budget - dev_team_reward_scr;

    auto active_sp_holders_reward = get_active_voters_reward(user_reward_scr);

    auto& balancer = voters_reward_scr_service.get();

    BOOST_REQUIRE_EQUAL(balancer.balance, active_sp_holders_reward);
}

SCORUM_TEST_CASE(per_block_sp_payment_division_from_fund_budget)
{
    asset alice_sp_before = account_service.get_account(alice.name).scorumpower;
    asset bob_sp_before = account_service.get_account(bob.name).scorumpower;

    auto post = create_post(alice).push();
    post.vote(bob).push();
    post.vote(alice).in_block();

    auto initial_blocks = db.head_block_num();
    generate_blocks(db.head_block_time() + SCORUM_ACTIVE_SP_HOLDERS_REWARD_PERIOD);
    auto pass_blocks = db.head_block_num() - initial_blocks;

    auto active_sp_holders_reward = get_active_voters_reward(budget_service.get_fund_budget().per_block);
    active_sp_holders_reward *= pass_blocks;

    auto alice_reward = active_sp_holders_reward * alice.sp_percent / 100;
    auto bob_reward = active_sp_holders_reward - alice_reward;

    BOOST_REQUIRE_EQUAL(account_service.get_account(alice.name).scorumpower, alice_sp_before + alice_reward);

    BOOST_REQUIRE_EQUAL(account_service.get_account(bob.name).scorumpower, bob_sp_before + bob_reward);
}

SCORUM_TEST_CASE(per_block_payments_are_stopped_after_battary_restored)
{
    const auto& voter = account_service.get_account(bob.name);
    asset bob_sp_before = voter.scorumpower;

    auto vote_weight = 100;

    uint16_t current_power = scorum::rewards_math::calculate_restoring_power(
        voter.voting_power, dprops_service.head_block_time(), voter.last_vote_time, SCORUM_VOTE_REGENERATION_SECONDS);

    uint16_t used_power = scorum::rewards_math::calculate_used_power(current_power, vote_weight * SCORUM_1_PERCENT,
                                                                     SCORUM_VOTING_POWER_DECAY_PERCENT);

    auto voting_power_restoring_time = scorum::rewards_math::calculate_expected_restoring_time(
        current_power - used_power, dprops_service.head_block_time(), SCORUM_VOTE_REGENERATION_SECONDS);

    auto post = create_post(alice).push();
    post.vote(bob, vote_weight).in_block();

    auto initial_blocks = db.head_block_num();
    generate_blocks(voting_power_restoring_time + SCORUM_ACTIVE_SP_HOLDERS_REWARD_PERIOD);
    auto pass_blocks = db.head_block_num() - initial_blocks;

    auto active_sp_holders_reward = get_active_voters_reward(budget_service.get_fund_budget().per_block);

    BOOST_REQUIRE_EQUAL(voter.scorumpower, bob_sp_before + active_sp_holders_reward * pass_blocks);

    initial_blocks = db.head_block_num();
    generate_blocks(db.head_block_time() + SCORUM_ACTIVE_SP_HOLDERS_REWARD_PERIOD);
    pass_blocks = db.head_block_num() - initial_blocks;

    BOOST_REQUIRE_EQUAL(voter.scorumpower, bob_sp_before + active_sp_holders_reward * pass_blocks);

    auto& balancer = voters_reward_sp_service.get();

    pass_blocks += 1; // in_block

    BOOST_REQUIRE_EQUAL(balancer.balance, active_sp_holders_reward * pass_blocks);
}

SCORUM_TEST_CASE(payments_from_sp_balancer_arter_fund_budget_is_over)
{
    const auto fund_budget_period_in_blocks = 2;

    auto& fund_budget = budget_service.get_fund_budget();

    budget_service.update(fund_budget, [&](budget_object& b) {
        b.per_block = b.balance / fund_budget_period_in_blocks;
        b.deadline = dprops_service.head_block_time() + fund_budget_period_in_blocks * SCORUM_BLOCK_INTERVAL;
    });

    generate_blocks(fund_budget_period_in_blocks);

    BOOST_REQUIRE(!budget_service.is_fund_budget_exists());

    const auto& voter = account_service.get_account(bob.name);
    asset bob_sp_before = voter.scorumpower;

    auto post = create_post(alice).push();
    post.vote(bob).in_block();

    generate_blocks(db.head_block_time() + SCORUM_ACTIVE_SP_HOLDERS_REWARD_PERIOD);

    auto& balancer = voters_reward_sp_service.get();

    BOOST_REQUIRE(balancer.current_per_block_reward.amount != 0);

    BOOST_REQUIRE_GT(voter.scorumpower, bob_sp_before);
}

SCORUM_TEST_CASE(active_sp_holders_op_notifications_check)
{
    generate_blocks(db.head_block_time() + SCORUM_ACTIVE_SP_HOLDERS_REWARD_PERIOD);

    set_voter(bob);

    auto post = create_post(alice).push();
    post.vote(bob, 30).in_block();

    BOOST_CHECK_EQUAL(op_times(), 0);

    generate_blocks(db.head_block_time() + SCORUM_ACTIVE_SP_HOLDERS_REWARD_PERIOD);

    BOOST_CHECK_EQUAL(op_times(), 1);

    generate_blocks(db.head_block_time() + SCORUM_ACTIVE_SP_HOLDERS_REWARD_PERIOD);

    BOOST_CHECK_EQUAL(op_times(), 1);

    post.vote(bob, 90).in_block();

    generate_blocks(db.head_block_time() + SCORUM_ACTIVE_SP_HOLDERS_REWARD_PERIOD);

    BOOST_CHECK_EQUAL(op_times(), 2);
}

SCORUM_TEST_CASE(second_cashout_if_no_vote_after_first_cashout_check)
{
    generate_blocks(db.head_block_time() + SCORUM_ACTIVE_SP_HOLDERS_REWARD_PERIOD);

    set_voter(bob);

    auto start = db.head_block_time();

    auto post = create_post(alice).push();
    post.vote(bob, 100).in_block();

    generate_blocks(start
                    + fc::seconds(SCORUM_ACTIVE_SP_HOLDERS_REWARD_PERIOD.to_seconds() - SCORUM_BLOCK_INTERVAL * 2));

    post = create_post(bob).push();
    post.vote(bob, 100).in_block();

    BOOST_CHECK_EQUAL(op_times(), 0);

    generate_blocks(start + SCORUM_ACTIVE_SP_HOLDERS_REWARD_PERIOD);

    const auto& bob_obj = account_service.get_account(bob.name);
    BOOST_REQUIRE(bob_obj.voting_power_restoring_time > db.head_block_time());
    BOOST_REQUIRE_LT(bob_obj.voting_power, SCORUM_100_PERCENT);

    BOOST_CHECK_NE(bob_obj.active_sp_holders_cashout_time.to_iso_string(),
                   fc::time_point_sec::maximum().to_iso_string());

    BOOST_CHECK_EQUAL(op_times(), 1);

    generate_blocks(db.head_block_time() + SCORUM_ACTIVE_SP_HOLDERS_REWARD_PERIOD);

    BOOST_CHECK_EQUAL(bob_obj.active_sp_holders_cashout_time.to_iso_string(),
                      fc::time_point_sec::maximum().to_iso_string());

    BOOST_CHECK_EQUAL(op_times(), 2);
}

BOOST_AUTO_TEST_SUITE_END()
}
