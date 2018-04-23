#include <scorum/protocol/config.hpp>
#include <boost/make_unique.hpp>

#include <fc/exception/exception.hpp>

namespace scorum {
namespace protocol {
namespace detail {

std::unique_ptr<config> config::instance = boost::make_unique<config>();

#ifndef BLOGGING_START_DATE
static_assert(false, "Macro BLOGGING_START_DATE required.");
#endif

#ifndef BLOGGING_BOUNTY_CASHOUT_DATE
static_assert(false, "Macro BLOGGING_BOUNTY_CASHOUT_DATE required.");
#endif

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

    , initial_date(fc::time_point_sec::min())
#ifdef BLOGGING_START_DATE
    , blogging_start_date(fc::time_point_sec::from_iso_string(BOOST_PP_STRINGIZE(BLOGGING_START_DATE)))
#endif
#ifdef BLOGGING_BOUNTY_CASHOUT_DATE
    , blogging_bounty_cashout_date(
          fc::time_point_sec::from_iso_string(BOOST_PP_STRINGIZE(BLOGGING_BOUNTY_CASHOUT_DATE)))
#endif
{
    FC_ASSERT(blogging_start_date + cashout_window_seconds < blogging_bounty_cashout_date,
              "Required: blogging_bounty_cashout_date >= blogging_start_date + cashout_window_seconds.");
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

    , initial_date(fc::time_point_sec::from_iso_string("2018-04-01T00:00:00"))

    , blogging_start_date(initial_date + cashout_window_seconds * 10)

    , blogging_bounty_cashout_date(blogging_start_date + cashout_window_seconds * 11)
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
