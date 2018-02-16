/*
 * Copyright (c) 2016 Steemit, Inc., and contributors.
 */

// clang-format off

#pragma once

#define DAYS_TO_SECONDS(X)                     (60u*60u*24u*X)

#define SCORUM_BLOCKCHAIN_VERSION              ( version(0, 0, 1) )
#define SCORUM_BLOCKCHAIN_HARDFORK_VERSION     ( hardfork_version( SCORUM_BLOCKCHAIN_VERSION ) )

#define SCORUM_ADDRESS_PREFIX                  "SCR"

#define SCORUM_BLOCKID_POOL_SIZE 0xffff

#define SCORUM_CURRENCY_PRECISION  9

// SCORUM = SCR with 3 digits of precision
#define SCORUM_SYMBOL  (uint64_t(SCORUM_CURRENCY_PRECISION) | (uint64_t('S') << 8) | (uint64_t('C') << 16) | (uint64_t('R') << 24))
// VESTS = SP with 6 digits of precision
#define VESTS_SYMBOL   (uint64_t(SCORUM_CURRENCY_PRECISION) | (uint64_t('S') << 8) | (uint64_t('P') << 16))

#define SCORUM_MAX_SHARE_SUPPLY                share_value_type(100000000e+9) //100 million

#define SCORUM_VOTE_DUST_THRESHOLD             share_value_type(50)

#define SCORUM_ATOMICSWAP_CONTRACT_METADATA_MAX_LENGTH  10*1024
#define SCORUM_ATOMICSWAP_SECRET_MAX_LENGTH             1024

//Got only minimum for transactions bandwidth. Required spend SCR to enlarge up to SCORUM_VOTE_DUST_THRESHOLD
#define SCORUM_MIN_ACCOUNT_CREATION_FEE        asset(SCORUM_VOTE_DUST_THRESHOLD/2, SCORUM_SYMBOL)

#define SCORUM_CREATE_ACCOUNT_WITH_SCORUM_MODIFIER 30

#define SCORUM_MIN_DELEGATE_VESTING_SHARES_MODIFIER 10

#define SCORUM_MIN_PAYOUT                  (asset(5, SCORUM_SYMBOL))

#define SCORUM_ADJUST_REWARD_PERCENT                    5

////////////////////////////////////////////////////////////////////////////////////////////////////
#ifdef IS_TEST_NET

#define SCORUM_CASHOUT_WINDOW_SECONDS          (60*60) /// 1 hr
#define SCORUM_UPVOTE_LOCKOUT                  (fc::minutes(5))

#define SCORUM_OWNER_AUTH_RECOVERY_PERIOD                  fc::seconds(60)
#define SCORUM_ACCOUNT_RECOVERY_REQUEST_EXPIRATION_PERIOD  fc::seconds(12)
#define SCORUM_OWNER_UPDATE_LIMIT                          fc::seconds(0)
#define SCORUM_OWNER_AUTH_HISTORY_TRACKING_START_BLOCK_NUM 1

#define SCORUM_REWARDS_INITIAL_SUPPLY_PERIOD_IN_DAYS    5

#define SCORUM_GUARANTED_REWARD_SUPPLY_PERIOD_IN_DAYS   2
#define SCORUM_REWARD_INCREASE_THRESHOLD_IN_DAYS        3

#define SCORUM_BUDGET_LIMIT_COUNT_PER_OWNER       5
#define SCORUM_BUDGET_LIMIT_DB_LIST_SIZE          SCORUM_BUDGET_LIMIT_COUNT_PER_OWNER
#define SCORUM_BUDGET_LIMIT_API_LIST_SIZE         SCORUM_BUDGET_LIMIT_COUNT_PER_OWNER

#define SCORUM_REGISTRATION_BONUS_LIMIT_PER_MEMBER_PER_N_BLOCK    asset( 10, SCORUM_SYMBOL )

#define SCORUM_ATOMICSWAP_INITIATOR_REFUND_LOCK_SECS           60*20
#define SCORUM_ATOMICSWAP_PARTICIPANT_REFUND_LOCK_SECS         60*10

#define SCORUM_ATOMICSWAP_LIMIT_REQUESTED_CONTRACTS_PER_OWNER            5
#define SCORUM_ATOMICSWAP_LIMIT_REQUESTED_CONTRACTS_PER_RECIPIENT        2

#else // IS LIVE SCORUM NETWORK

#define SCORUM_CASHOUT_WINDOW_SECONDS          (DAYS_TO_SECONDS(7))
#define SCORUM_UPVOTE_LOCKOUT                  (fc::hours(12))

#define SCORUM_OWNER_AUTH_RECOVERY_PERIOD                  fc::days(30)
#define SCORUM_ACCOUNT_RECOVERY_REQUEST_EXPIRATION_PERIOD  fc::days(1)
#define SCORUM_OWNER_UPDATE_LIMIT                          fc::minutes(60)
#define SCORUM_OWNER_AUTH_HISTORY_TRACKING_START_BLOCK_NUM 3186477

#define SCORUM_REWARDS_INITIAL_SUPPLY_PERIOD_IN_DAYS    (2 * 365)

#define SCORUM_GUARANTED_REWARD_SUPPLY_PERIOD_IN_DAYS   30
#define SCORUM_REWARD_INCREASE_THRESHOLD_IN_DAYS        100

#define SCORUM_BUDGET_LIMIT_COUNT_PER_OWNER       1000
#define SCORUM_BUDGET_LIMIT_DB_LIST_SIZE          1000
#define SCORUM_BUDGET_LIMIT_API_LIST_SIZE         1000

#define SCORUM_REGISTRATION_BONUS_LIMIT_PER_MEMBER_PER_N_BLOCK    asset( 1000, VESTS_SYMBOL )

#define SCORUM_ATOMICSWAP_INITIATOR_REFUND_LOCK_SECS           48*3600
#define SCORUM_ATOMICSWAP_PARTICIPANT_REFUND_LOCK_SECS         24*3600

#define SCORUM_ATOMICSWAP_LIMIT_REQUESTED_CONTRACTS_PER_OWNER            1000
#define SCORUM_ATOMICSWAP_LIMIT_REQUESTED_CONTRACTS_PER_RECIPIENT        10
#endif
////////////////////////////////////////////////////////////////////////////////////////////////////

#define SCORUM_REGISTRATION_LIMIT_COUNT_COMMITTEE_MEMBERS        30

#define SCORUM_BLOCK_INTERVAL                  3
#define SCORUM_BLOCKS_PER_YEAR                 (365*24*60*60/SCORUM_BLOCK_INTERVAL)
#define SCORUM_BLOCKS_PER_DAY                  (24*60*60/SCORUM_BLOCK_INTERVAL)
#define SCORUM_BLOCKS_PER_HOUR                 (60*60/SCORUM_BLOCK_INTERVAL)
#define SCORUM_START_VESTING_BLOCK             (SCORUM_BLOCKS_PER_DAY * 7)
#define SCORUM_START_MINER_VOTING_BLOCK        (SCORUM_BLOCKS_PER_DAY * 30)

#define SCORUM_NUM_INIT_DELEGATES              1

#define SCORUM_MAX_VOTED_WITNESSES              20
#define SCORUM_MAX_RUNNER_WITNESSES             1
#define SCORUM_MAX_WITNESSES                    (SCORUM_MAX_VOTED_WITNESSES+SCORUM_MAX_RUNNER_WITNESSES)
#define SCORUM_WITNESS_MISSED_BLOCKS_THRESHOLD  SCORUM_BLOCKS_PER_DAY/2
#define SCORUM_HARDFORK_REQUIRED_WITNESSES      17 // 17 of the 21 dpos witnesses (20 elected and 1 virtual time) required for hardfork. This guarantees 75% participation on all subsequent rounds.

#define SCORUM_MAX_TIME_UNTIL_EXPIRATION       (60*60) // seconds,  aka: 1 hour
#define SCORUM_MAX_MEMO_SIZE                   2048
#define SCORUM_MAX_PROXY_RECURSION_DEPTH       4

#define SCORUM_VESTING_WITHDRAW_INTERVALS           13
#define SCORUM_VESTING_WITHDRAW_INTERVAL_SECONDS    (DAYS_TO_SECONDS(7)) /// 1 week per interval

#define SCORUM_MAX_WITHDRAW_ROUTES             10
#define SCORUM_SAVINGS_WITHDRAW_TIME           (fc::days(3))
#define SCORUM_SAVINGS_WITHDRAW_REQUEST_LIMIT  100
#define SCORUM_VOTE_REGENERATION_SECONDS       (DAYS_TO_SECONDS(5))
#define SCORUM_MAX_VOTE_CHANGES                5
#define SCORUM_REVERSE_AUCTION_WINDOW_SECONDS  (60*30) /// 30 minutes
#define SCORUM_MIN_VOTE_INTERVAL_SEC           3

#define SCORUM_MIN_ROOT_COMMENT_INTERVAL       (fc::seconds(60*5)) // 5 minutes
#define SCORUM_MIN_REPLY_INTERVAL              (fc::seconds(20)) // 20 seconds

#define SCORUM_MAX_ACCOUNT_WITNESS_VOTES       30

#define SCORUM_100_PERCENT                     10000
#define SCORUM_1_PERCENT                       (SCORUM_100_PERCENT/100)
#define SCORUM_1_TENTH_PERCENT                 (SCORUM_100_PERCENT/1000)
#define SCORUM_PERCENT(X)                      (uint16_t)(X*SCORUM_1_PERCENT)

#define SCORUM_CONTENT_REWARD_PERCENT          (95*SCORUM_1_PERCENT)
#define SCORUM_CURATION_REWARD_PERCENT         (25*SCORUM_1_PERCENT)

#define SCORUM_BANDWIDTH_AVERAGE_WINDOW_SECONDS (DAYS_TO_SECONDS(7))
#define SCORUM_BANDWIDTH_PRECISION              (uint64_t(1000000)) ///< 1 million
#define SCORUM_MAX_COMMENT_DEPTH                0xffff // 64k
#define SCORUM_SOFT_MAX_COMMENT_DEPTH           0xff // 255

#define SCORUM_MAX_RESERVE_RATIO                (20000)

#define SCORUM_CREATE_ACCOUNT_DELEGATION_RATIO     5
#define SCORUM_CREATE_ACCOUNT_DELEGATION_TIME      fc::days(30)

#define SCORUM_RECENT_RSHARES_DECAY_RATE       (fc::days(15))
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

#define SCORUM_MAX_VOTES_PER_DAY_VOTING_POWER_RATE 40

#define SCORUM_MIN_QUORUM_VALUE_PERCENT         (50u)
#define SCORUM_MAX_QUORUM_VALUE_PERCENT         (100u)

#define SCORUM_PROPOSAL_LIFETIME_MIN_SECONDS    (DAYS_TO_SECONDS(1u))
#define SCORUM_PROPOSAL_LIFETIME_MAX_SECONDS    (DAYS_TO_SECONDS(10u))

#define SCORUM_REGISTRATION_BONUS_LIMIT_PER_MEMBER_N_BLOCK        2

/**
 *  Reserved Account IDs with special meaning
 */
/// Represents the canonical account for specifying you will vote for directly (as opposed to a proxy)
#define SCORUM_PROXY_TO_SELF_ACCOUNT           (account_name_type())
/// Represents the canonical root post parent account
#define SCORUM_ROOT_POST_PARENT_ACCOUNT        (account_name_type())
/// Represents the account that owns SP for development pool but can't spend it
#define SCORUM_DEV_POOL_SP_LOCKED_ACCOUNT      (account_name_type(std::string(sizeof(account_name_type), 0xff)))
///@}

// clang-format on
