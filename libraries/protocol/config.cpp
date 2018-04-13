#include <scorum/protocol/config.hpp>

#define DAYS_TO_SECONDS(X) (60u*60u*24u*X)

namespace scorum {
namespace protocol {
namespace detail {

    uint32_t config::blockid_pool_size      = 0xffff;

    uint32_t config::cashout_window_seconds = DAYS_TO_SECONDS(7);

    fc::microseconds config::upvote_lockout = fc::hours(12);

    fc::microseconds config::owner_auth_recovery_period                 = fc::days(30);
    fc::microseconds config::account_recovery_request_expiration_period = fc::days(1);
    fc::microseconds config::owner_update_limit                         = fc::minutes(60);

    uint32_t config::rewards_initial_supply_period_in_days      = 2 * 365;
    uint32_t config::guaranted_reward_supply_period_in_days     = 30;
    uint32_t config::reward_increase_threshold_in_days          = 100;

    uint32_t config::budgets_limit_per_owner                    = 1000000;

    uint32_t config::atomicswap_initiator_refund_lock_secs      = 48*3600;
    uint32_t config::atomicswap_participant_refund_lock_secs    = 24*3600;

    uint32_t config::atomicswap_limit_requested_contracts_per_owner     = 1000;
    uint32_t config::atomicswap_limit_requested_contracts_per_recipient = 10;

    uint32_t config::vesting_withdraw_intervals                 = 52;
    uint32_t config::vesting_withdraw_interval_seconds          = DAYS_TO_SECONDS(7); // 1 week per interval

    bool config::is_test_net = false;


    void config::override_for_test_net()
    {
        config::blockid_pool_size           = 0xfff;

        config::cashout_window_seconds      = 60*60; //1 hr

        config::upvote_lockout              = fc::minutes(5);

        config::owner_auth_recovery_period                  = fc::seconds(60);
        config::account_recovery_request_expiration_period  = fc::seconds(12);
        config::owner_update_limit                          = fc::seconds(0);

        config::rewards_initial_supply_period_in_days   = 5;
        config::guaranted_reward_supply_period_in_days  = 2;
        config::reward_increase_threshold_in_days       = 3;

        config::budgets_limit_per_owner                 = 5;

        config::atomicswap_initiator_refund_lock_secs   = 60*20;
        config::atomicswap_participant_refund_lock_secs = 60*10;

        config::atomicswap_limit_requested_contracts_per_owner      = 5;
        config::atomicswap_limit_requested_contracts_per_recipient  = 2;

        config::vesting_withdraw_intervals              = 13;
        config::vesting_withdraw_interval_seconds       = 60*7;

        config::is_test_net = true;
    }
}
}
}
