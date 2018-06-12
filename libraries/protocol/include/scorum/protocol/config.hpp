/*
 * Copyright (c) 2016 Steemit, Inc., and contributors.
 */

// clang-format off

#pragma once
#include <cstdint>
#include <fc/time.hpp>

namespace scorum {
namespace protocol {
namespace detail {

    struct config
    {
        static std::unique_ptr<config> instance;

        const uint32_t blockid_pool_size;

        const uint32_t cashout_window_seconds;

        const fc::microseconds vote_regeneration_seconds;

        const fc::microseconds upvote_lockout;

        const fc::microseconds owner_auth_recovery_period;
        const fc::microseconds account_recovery_request_expiration_period;
        const fc::microseconds owner_update_limit;

        const fc::microseconds recent_rshares_decay_rate;

        const uint32_t rewards_initial_supply_period_in_days;
        const uint32_t guaranted_reward_supply_period_in_days;
        const uint32_t reward_increase_threshold_in_days;

        const uint32_t budgets_limit_per_owner;

        const uint32_t atomicswap_initiator_refund_lock_secs;
        const uint32_t atomicswap_participant_refund_lock_secs;

        const uint32_t atomicswap_limit_requested_contracts_per_owner;
        const uint32_t atomicswap_limit_requested_contracts_per_recipient;

        const uint32_t vesting_withdraw_intervals;
        const uint32_t vesting_withdraw_interval_seconds;

        const uint32_t min_vote_interval_sec;

        const uint32_t db_free_memory_threshold_mb;

        const fc::time_point_sec initial_date;

        const fc::time_point_sec blogging_start_date;

        const fc::time_point_sec fifa_world_cup_2018_bounty_cashout_date;

        const fc::microseconds expiraton_for_registration_bonus;
        
        const fc::time_point_sec witness_reward_migration_date;

        enum test_mode { test };

        explicit config(test_mode);
        config();
    };

    const config& get_config();

    void override_config(std::unique_ptr<config> new_config);
}
}
}

#define DAYS_TO_SECONDS(X)                     (60u*60u*24u*X)

#define SCORUM_BLOCKCHAIN_VERSION              ( version(0, 0, 3) )
#define SCORUM_BLOCKCHAIN_HARDFORK_VERSION     ( hardfork_version( SCORUM_BLOCKCHAIN_VERSION ) )

#define SCORUM_ADDRESS_PREFIX                  "SCR"

#define SCORUM_CURRENCY_PRECISION  9

// Scorum Coin = SCR with 9 digits of precision
#define SCORUM_SYMBOL  (uint64_t(SCORUM_CURRENCY_PRECISION) | (uint64_t('S') << 8) | (uint64_t('C') << 16) | (uint64_t('R') << 24))
// Scorum Power = SP with 9 digits of precision
#define SP_SYMBOL   (uint64_t(SCORUM_CURRENCY_PRECISION) | (uint64_t('S') << 8) | (uint64_t('P') << 16))

#define SCORUM_MAX_SHARE_SUPPLY                share_value_type(100000000e+9) //100 million

#define SCORUM_VOTE_DUST_THRESHOLD             share_type(50)

#define SCORUM_ATOMICSWAP_CONTRACT_METADATA_MAX_LENGTH  10*1024
#define SCORUM_ATOMICSWAP_SECRET_MAX_LENGTH             1024

//Got only minimum for transactions bandwidth. Required spend SCR to enlarge up to SCORUM_VOTE_DUST_THRESHOLD
#define SCORUM_MIN_ACCOUNT_CREATION_FEE        asset(SCORUM_VOTE_DUST_THRESHOLD/2, SCORUM_SYMBOL)

#define SCORUM_MIN_COMMENT_PAYOUT_SHARE        share_type(5)

#define SCORUM_MIN_PER_BLOCK_REWARD            share_type(1)

#define SCORUM_CREATE_ACCOUNT_WITH_SCORUM_MODIFIER  30

#define SCORUM_MIN_DELEGATE_VESTING_SHARES_MODIFIER 10

#define SCORUM_START_WITHDRAW_COEFFICIENT           10


////////////////////////////////////////////////////////////////////////////////////////////////////
#define SCORUM_BLOCKID_POOL_SIZE                (scorum::protocol::detail::get_config().blockid_pool_size)

#define SCORUM_CASHOUT_WINDOW_SECONDS           (scorum::protocol::detail::get_config().cashout_window_seconds)

#define SCORUM_UPVOTE_LOCKOUT                   (scorum::protocol::detail::get_config().upvote_lockout)

#define SCORUM_OWNER_AUTH_RECOVERY_PERIOD                   (scorum::protocol::detail::get_config().owner_auth_recovery_period)
#define SCORUM_ACCOUNT_RECOVERY_REQUEST_EXPIRATION_PERIOD   (scorum::protocol::detail::get_config().account_recovery_request_expiration_period)
#define SCORUM_OWNER_UPDATE_LIMIT                           (scorum::protocol::detail::get_config().owner_update_limit)

#define SCORUM_REWARDS_INITIAL_SUPPLY_PERIOD_IN_DAYS        (scorum::protocol::detail::get_config().rewards_initial_supply_period_in_days)

#define SCORUM_GUARANTED_REWARD_SUPPLY_PERIOD_IN_DAYS       (scorum::protocol::detail::get_config().guaranted_reward_supply_period_in_days)
#define SCORUM_REWARD_INCREASE_THRESHOLD_IN_DAYS            (scorum::protocol::detail::get_config().reward_increase_threshold_in_days)

#define SCORUM_BUDGETS_LIMIT_PER_OWNER                      (scorum::protocol::detail::get_config().budgets_limit_per_owner)

#define SCORUM_ATOMICSWAP_INITIATOR_REFUND_LOCK_SECS        (scorum::protocol::detail::get_config().atomicswap_initiator_refund_lock_secs)
#define SCORUM_ATOMICSWAP_PARTICIPANT_REFUND_LOCK_SECS      (scorum::protocol::detail::get_config().atomicswap_participant_refund_lock_secs)

#define SCORUM_ATOMICSWAP_LIMIT_REQUESTED_CONTRACTS_PER_OWNER       (scorum::protocol::detail::get_config().atomicswap_limit_requested_contracts_per_owner)
#define SCORUM_ATOMICSWAP_LIMIT_REQUESTED_CONTRACTS_PER_RECIPIENT   (scorum::protocol::detail::get_config().atomicswap_limit_requested_contracts_per_recipient)

#define SCORUM_VESTING_WITHDRAW_INTERVALS                           (scorum::protocol::detail::get_config().vesting_withdraw_intervals)
#define SCORUM_VESTING_WITHDRAW_INTERVAL_SECONDS                    (scorum::protocol::detail::get_config().vesting_withdraw_interval_seconds) /// 1 week per interval

#define SCORUM_MIN_VOTE_INTERVAL_SEC            (scorum::protocol::detail::get_config().min_vote_interval_sec)

#define SCORUM_DB_FREE_MEMORY_THRESHOLD_MB      (scorum::protocol::detail::get_config().db_free_memory_threshold_mb)
////////////////////////////////////////////////////////////////////////////////////////////////////

#define SCORUM_REGISTRATION_BONUS_LIMIT_PER_MEMBER_PER_N_BLOCK    100 /// * registration_bonus

#define SCORUM_REGISTRATION_BONUS_LIMIT_PER_MEMBER_N_BLOCK        2

#define SCORUM_REGISTRATION_COMMITTEE_MAX_MEMBERS_LIMIT        30
#define SCORUM_DEVELOPMENT_COMMITTEE_MAX_MEMBERS_LIMIT         30

#define SCORUM_BLOCK_INTERVAL                  3
#define SCORUM_BLOCKS_PER_YEAR                 (365*24*60*60/SCORUM_BLOCK_INTERVAL)
#define SCORUM_BLOCKS_PER_DAY                  (24*60*60/SCORUM_BLOCK_INTERVAL)
#define SCORUM_BLOCKS_PER_HOUR                 (60*60/SCORUM_BLOCK_INTERVAL)
#define SCORUM_START_MINER_VOTING_BLOCK        (SCORUM_BLOCKS_PER_DAY * 30)

#define SCORUM_MAX_VOTED_WITNESSES              20
#define SCORUM_MAX_RUNNER_WITNESSES             1
#define SCORUM_MAX_WITNESSES                    (SCORUM_MAX_VOTED_WITNESSES+SCORUM_MAX_RUNNER_WITNESSES)
#define SCORUM_WITNESS_MISSED_BLOCKS_THRESHOLD  SCORUM_BLOCKS_PER_DAY/2
#define SCORUM_HARDFORK_REQUIRED_WITNESSES      17 // 17 of the 21 dpos witnesses (20 elected and 1 virtual time) required for hardfork. This guarantees 75% participation on all subsequent rounds.

#define SCORUM_MAX_TIME_UNTIL_EXPIRATION       (60*60) // seconds,  aka: 1 hour
#define SCORUM_MAX_MEMO_SIZE                   2048
#define SCORUM_MAX_PROXY_RECURSION_DEPTH       4

#define SCORUM_MAX_WITHDRAW_ROUTES             10
#define SCORUM_SAVINGS_WITHDRAW_TIME           (fc::days(3))
#define SCORUM_SAVINGS_WITHDRAW_REQUEST_LIMIT  100
#define SCORUM_VOTE_REGENERATION_SECONDS       (scorum::protocol::detail::get_config().vote_regeneration_seconds)
#define SCORUM_MAX_VOTE_CHANGES                3
#define SCORUM_REVERSE_AUCTION_WINDOW_SECONDS  (fc::seconds(60*30)) // 30 minutes

#define SCORUM_MIN_ROOT_COMMENT_INTERVAL       (fc::minutes(5))
#define SCORUM_MIN_REPLY_INTERVAL              (fc::seconds(20))

#define SCORUM_MAX_ACCOUNT_WITNESS_VOTES       30

#define SCORUM_100_PERCENT                     10000
#define SCORUM_1_PERCENT                       (SCORUM_100_PERCENT/100)
#define SCORUM_1_TENTH_PERCENT                 (SCORUM_100_PERCENT/1000)
#define SCORUM_PERCENT(X)                      (uint16_t)(X*SCORUM_1_PERCENT)

#define SCORUM_DEV_TEAM_PER_BLOCK_REWARD_PERCENT            SCORUM_PERCENT(50)
#define SCORUM_WITNESS_PER_BLOCK_REWARD_PERCENT             SCORUM_PERCENT(10)
#define SCORUM_ACTIVE_SP_HOLDERS_PER_BLOCK_REWARD_PERCENT   SCORUM_PERCENT(10)
#define SCORUM_CURATION_REWARD_PERCENT                      SCORUM_PERCENT(25)
#define SCORUM_PARENT_COMMENT_REWARD_PERCENT        		SCORUM_PERCENT(50)

#define SCORUM_ADJUST_REWARD_PERCENT                        SCORUM_PERCENT(5)

#define SCORUM_BANDWIDTH_AVERAGE_WINDOW_SECONDS (DAYS_TO_SECONDS(7))
#define SCORUM_BANDWIDTH_PRECISION              (uint64_t(1000000)) ///< 1 million
#define SCORUM_MAX_COMMENT_DEPTH                (6)
#define SCORUM_SOFT_MAX_COMMENT_DEPTH           (6)

#define SCORUM_MAX_RESERVE_RATIO                (20000)

#define SCORUM_CREATE_ACCOUNT_DELEGATION_RATIO     5
#define SCORUM_CREATE_ACCOUNT_DELEGATION_TIME      fc::days(30)

#define SCORUM_RECENT_RSHARES_DECAY_RATE       (scorum::protocol::detail::get_config().recent_rshares_decay_rate)
// note, if redefining these constants make sure calculate_claims doesn't overflow

#define SCORUM_MIN_ACCOUNT_NAME_LENGTH          3
#define SCORUM_MAX_ACCOUNT_NAME_LENGTH         16

#define SCORUM_MIN_PERMLINK_LENGTH             0
#define SCORUM_MAX_PERMLINK_LENGTH             256
#define SCORUM_MAX_WITNESS_URL_LENGTH          2048

#define SCORUM_MAX_SIG_CHECK_DEPTH             2

#define SCORUM_MAX_TRANSACTION_SIZE            (1024*64)
#define SCORUM_MIN_BLOCK_SIZE_LIMIT            (SCORUM_MAX_TRANSACTION_SIZE)
#define SCORUM_MAX_BLOCK_SIZE                  (SCORUM_MAX_TRANSACTION_SIZE*SCORUM_BLOCK_INTERVAL*2000)

#define SCORUM_MIN_UNDO_HISTORY                 10
#define SCORUM_MAX_UNDO_HISTORY                 10000

#define SCORUM_MIN_TRANSACTION_EXPIRATION_LIMIT (SCORUM_BLOCK_INTERVAL * 5) // 5 transactions per block

#define SCORUM_IRREVERSIBLE_THRESHOLD           (75 * SCORUM_1_PERCENT)

#define VIRTUAL_SCHEDULE_LAP_LENGTH             ( fc::uint128::max_value() )

#define SCORUM_COMMITTEE_QUORUM_PERCENT         (60u)

#define SCORUM_COMMITTEE_TRANSFER_QUORUM_PERCENT            (50u)
#define SCORUM_COMMITTEE_ADD_EXCLUDE_QUORUM_PERCENT         (60u)

#define SCORUM_VOTING_POWER_DECAY_PERCENT 5

#define SCORUM_MIN_QUORUM_VALUE_PERCENT         (50u)
#define SCORUM_MAX_QUORUM_VALUE_PERCENT         (100u)

#define SCORUM_PROPOSAL_LIFETIME_MIN_SECONDS    (DAYS_TO_SECONDS(1u))
#define SCORUM_PROPOSAL_LIFETIME_MAX_SECONDS    (DAYS_TO_SECONDS(10u))

/**
 *  Reserved Account IDs with special meaning
 */
/// Represents the canonical account for specifying you will vote for directly (as opposed to a proxy)
#define SCORUM_PROXY_TO_SELF_ACCOUNT           (account_name_type())
/// Represents the canonical root post parent account
#define SCORUM_ROOT_POST_PARENT_ACCOUNT        (account_name_type())


#define SCORUM_BLOGGING_START_DATE (scorum::protocol::detail::get_config().blogging_start_date)

#define SCORUM_FIFA_WORLD_CUP_2018_BOUNTY_CASHOUT_DATE (scorum::protocol::detail::get_config().fifa_world_cup_2018_bounty_cashout_date)

#define SCORUM_EXPIRATON_FOR_REGISTRATION_BONUS        (scorum::protocol::detail::get_config().expiraton_for_registration_bonus)

#define SCORUM_WITNESS_REWARD_MIGRATION_DATE (scorum::protocol::detail::get_config().witness_reward_migration_date)
///@}

// clang-format on
