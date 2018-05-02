#include <scorum/protocol/config.hpp>
#include <boost/make_unique.hpp>

namespace scorum {
namespace protocol {
namespace detail {

std::unique_ptr<config> config::instance = boost::make_unique<config>();

config::config() /// production config
    : blockid_pool_size(0xffff)

    , cashout_window_seconds(DAYS_TO_SECONDS(7))

    , upvote_lockout(fc::hours(12))

    , owner_auth_recovery_period(fc::days(30))
    , account_recovery_request_expiration_period(fc::days(1))
    , owner_update_limit(fc::minutes(60))

    , rewards_initial_supply_period_in_days(2 * 365)
    , guaranted_reward_supply_period_in_days(30)
    , reward_increase_threshold_in_days(100)

    , budgets_limit_per_owner(1000000)

    , atomicswap_initiator_refund_lock_secs(48 * 3600)
    , atomicswap_participant_refund_lock_secs(24 * 3600)

    , atomicswap_limit_requested_contracts_per_owner(1000)
    , atomicswap_limit_requested_contracts_per_recipient(10)

    , vesting_withdraw_intervals(52)
    , vesting_withdraw_interval_seconds(DAYS_TO_SECONDS(7)) // 1 week per interval

    , min_vote_interval_sec(3)

    , db_free_memory_threshold_mb(100)

    , expiraton_for_registration_bonus(fc::days(182))
{
    // do nothing
}

config::config(test_mode) /// test config
    : blockid_pool_size(0xfff)

    , cashout_window_seconds(60 * 60) // 1 hr

    , upvote_lockout(fc::minutes(5))

    , owner_auth_recovery_period(fc::seconds(60))
    , account_recovery_request_expiration_period(fc::seconds(12))
    , owner_update_limit(fc::seconds(0))

    , rewards_initial_supply_period_in_days(5)
    , guaranted_reward_supply_period_in_days(2)
    , reward_increase_threshold_in_days(3)

    , budgets_limit_per_owner(5)

    , atomicswap_initiator_refund_lock_secs(60 * 20)
    , atomicswap_participant_refund_lock_secs(60 * 10)

    , atomicswap_limit_requested_contracts_per_owner(5)
    , atomicswap_limit_requested_contracts_per_recipient(2)

    , vesting_withdraw_intervals(13)
    , vesting_withdraw_interval_seconds(60 * 7)

    , min_vote_interval_sec(0)

    , db_free_memory_threshold_mb(5)

    , expiraton_for_registration_bonus(fc::minutes(30))
{
    // do nothing
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
