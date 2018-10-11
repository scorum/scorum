#include <scorum/protocol/config.hpp>
#include <boost/make_unique.hpp>

#include <fc/exception/exception.hpp>

namespace scorum {
namespace protocol {
namespace detail {

std::unique_ptr<config> config::instance = boost::make_unique<config>();

#ifndef BLOGGING_START_DATE
#define BLOGGING_START_DATE "2018-06-12T09:00:00"
#endif

#ifndef FIFA_WORLD_CUP_2018_BOUNTY_CASHOUT_DATE
#define FIFA_WORLD_CUP_2018_BOUNTY_CASHOUT_DATE "2018-08-08T12:00:00"
#endif

#ifndef WITNESS_REWARD_MIGRATION_DATE
#define WITNESS_REWARD_MIGRATION_DATE "2018-07-12T00:00:00"
#endif

config::config() /// production config
    : blockid_pool_size(0xffff)

    , vesting_withdraw_intervals(52)
#ifdef LIVE_TESTNET
    , vesting_withdraw_interval_seconds(10) // 10 sec per interval

    , cashout_window_seconds(7200) // 2 hours

    , upvote_lockout(fc::minutes(30))

    , active_sp_holders_reward_period(fc::minutes(1))

    , advertising_cashout_period_sec(60)
#else
    , vesting_withdraw_interval_seconds(DAYS_TO_SECONDS(7)) // 1 week per interval

    , cashout_window_seconds(DAYS_TO_SECONDS(7))

    , upvote_lockout(fc::hours(12))

    , active_sp_holders_reward_period(fc::days(7))

    , advertising_cashout_period_sec(DAYS_TO_SECONDS(7))
#endif

    , reverse_auction_window_seconds(fc::minutes(30))

    , vote_regeneration_seconds(fc::days(5))
    , owner_auth_recovery_period(fc::days(30))
    , account_recovery_request_expiration_period(fc::days(1))
    , owner_update_limit(fc::minutes(60))

    , recent_rshares_decay_rate(fc::days(15))

    , rewards_initial_supply_period_in_days(2 * 365)
    , guaranted_reward_supply_period_in_days(30)
    , reward_increase_threshold_in_days(100)

    , budgets_limit_per_owner(100)

    , atomicswap_initiator_refund_lock_secs(48 * 3600)
    , atomicswap_participant_refund_lock_secs(24 * 3600)

    , atomicswap_limit_requested_contracts_per_owner(1000)
    , atomicswap_limit_requested_contracts_per_recipient(10)

    , min_vote_interval_sec(3)

    , db_free_memory_threshold_mb(100)

    , initial_date(fc::time_point_sec::min())

    , blogging_start_date(fc::time_point_sec::from_iso_string(BLOGGING_START_DATE))

    , fifa_world_cup_2018_bounty_cashout_date(
          fc::time_point_sec::from_iso_string(FIFA_WORLD_CUP_2018_BOUNTY_CASHOUT_DATE))

    , expiraton_for_registration_bonus(fc::days(182))

    , witness_reward_migration_date(fc::time_point_sec::from_iso_string(WITNESS_REWARD_MIGRATION_DATE))

    , scorum_max_witnesses(21)

    , scorum_max_voted_witnesses(20)

    /// 17 of the 21 dpos witnesses (20 elected and 1 virtual time) required for hardfork. This guarantees 75%
    /// participation on all subsequent rounds.
    , scorum_hardfork_required_witnesses(17)
{
    FC_ASSERT(blogging_start_date + cashout_window_seconds < fifa_world_cup_2018_bounty_cashout_date,
              "Required: fifa_world_cup_2018_bounty_cashout_date >= blogging_start_date + cashout_window_seconds.");
    FC_ASSERT(scorum_max_witnesses <= SCORUM_MAX_WITNESSES_LIMIT);
    FC_ASSERT(scorum_max_witnesses > scorum_max_voted_witnesses, "No place for runner");
    FC_ASSERT(scorum_max_witnesses > scorum_hardfork_required_witnesses,
              "Invalid amount for hardfork required witnesses");
}

config::config(test_mode) /// test config
    : blockid_pool_size(0xfff)

    , vesting_withdraw_intervals(13)
    , vesting_withdraw_interval_seconds(60 * 7)

    , cashout_window_seconds(fc::hours(1).to_seconds())

    , upvote_lockout(fc::minutes(5))

    , active_sp_holders_reward_period(fc::minutes(15))

    , advertising_cashout_period_sec(15)

    , reverse_auction_window_seconds(fc::seconds(30))

    , vote_regeneration_seconds(fc::minutes(10))

    , owner_auth_recovery_period(fc::seconds(60))
    , account_recovery_request_expiration_period(fc::seconds(12))
    , owner_update_limit(fc::seconds(0))

    , recent_rshares_decay_rate(fc::hours(1))

    , rewards_initial_supply_period_in_days(5)
    , guaranted_reward_supply_period_in_days(2)
    , reward_increase_threshold_in_days(3)

    , budgets_limit_per_owner(8)

    , atomicswap_initiator_refund_lock_secs(60 * 20)
    , atomicswap_participant_refund_lock_secs(60 * 10)

    , atomicswap_limit_requested_contracts_per_owner(5)
    , atomicswap_limit_requested_contracts_per_recipient(2)

    , min_vote_interval_sec(0)

    , db_free_memory_threshold_mb(1)

    , initial_date(fc::time_point_sec::from_iso_string("2018-04-01T00:00:00"))

    , blogging_start_date(initial_date + cashout_window_seconds * 10)

    , fifa_world_cup_2018_bounty_cashout_date(blogging_start_date + cashout_window_seconds * 22)

    , expiraton_for_registration_bonus(fc::minutes(30))

    , witness_reward_migration_date(initial_date + cashout_window_seconds * 10)

    , scorum_max_witnesses(3)

    , scorum_max_voted_witnesses(2)

    , scorum_hardfork_required_witnesses(2)
{
    FC_ASSERT(scorum_max_witnesses <= SCORUM_MAX_WITNESSES_LIMIT);
    FC_ASSERT(scorum_max_witnesses > scorum_max_voted_witnesses, "No place for runner");
    FC_ASSERT(scorum_max_witnesses > scorum_hardfork_required_witnesses,
              "Invalid amount for hardfork required witnesses");
}

const config& get_config()
{
    return *config::instance;
}

void override_config(std::unique_ptr<config> new_config)
{
    config::instance = std::move(new_config);
}
}
}
}
