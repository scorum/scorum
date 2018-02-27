#pragma once

#include <map>
#include <scorum/protocol/asset.hpp>
#include <scorum/protocol/types.hpp>

namespace scorum {
namespace account_statistics {

using scorum::protocol::asset;
using scorum::protocol::account_name_type;

struct bucket_object;

// clang-format off
struct account_metric
{
    uint32_t signed_transactions = 0; ///< Transactions this account signed

    // bandwidth
    uint32_t market_bandwidth = 0; ///< Charged bandwidth for market transactions
    uint32_t non_market_bandwidth = 0; ///< Charged bandwidth for non-market transactions
    uint32_t total_ops = 0; ///< Ops this account was an authority on
    uint32_t market_ops = 0; ///< Market operations
    uint32_t forum_ops = 0; ///< Forum operations

    // money
    uint32_t transfers_from = 0; ///< Account to account transfers from this account
    uint32_t transfers_to = 0; ///< Account to account transfers to this account
    uint32_t transfers_to_vesting = 0; ///< Transfers to vesting by this account. Note: Transfer to vesting from A to B
                                       ///counts as a transfer from A to B followed by a vesting deposit by B.

    asset scorum_sent = asset(0, SCORUM_SYMBOL); ///< SCR sent from this account
    asset scorum_received = asset(0, SCORUM_SYMBOL); ///< SCR received by this account
    asset scorum_transferred_to_vesting = asset(0, SCORUM_SYMBOL); ///< SCR vested by the account
    asset vests_received_by_transfers = asset(0, VESTS_SYMBOL); ///< New SP by vesting transfers

    uint32_t new_vesting_withdrawal_requests = 0; ///< New vesting withdrawal requests
    uint32_t modified_vesting_withdrawal_requests = 0; ///< Changes to vesting withdraw requests
    uint32_t vesting_withdrawals_processed = 0; ///< Vesting withdrawals processed for this account
    uint32_t finished_vesting_withdrawals = 0; ///< Processed vesting withdrawals that are now finished

    asset vests_withdrawn = asset(0, VESTS_SYMBOL); ///< SP withdrawn from the account
    asset scorum_received_from_withdrawls = asset(0, SCORUM_SYMBOL); ///< SCR received from this account's vesting withdrawals
    asset scorum_received_from_routes = asset(0, SCORUM_SYMBOL); ///< SCR received from another account's vesting withdrawals
    asset vests_received_from_routes = asset(0, VESTS_SYMBOL); ///< SP received from another account's vesting withdrawals

    // comments
    uint32_t root_comments = 0; ///< Top level root comments
    uint32_t root_comment_edits = 0; ///< Edits to root comments
    uint32_t root_comments_deleted = 0; ///< Root comments deleted
    uint32_t replies = 0; ///< Replies to comments
    uint32_t reply_edits = 0; ///< Edits to replies
    uint32_t replies_deleted = 0; ///< Replies deleted
    uint32_t new_root_votes = 0; ///< New votes on root comments
    uint32_t changed_root_votes = 0; ///< Changed votes for root comments
    uint32_t new_reply_votes = 0; ///< New votes on replies
    uint32_t changed_reply_votes = 0; ///< Changed votes for replies

    uint32_t author_reward_payouts = 0; ///< Number of author reward payouts
    asset author_rewards_vests = asset(0, VESTS_SYMBOL); ///< SP paid for author rewards
    asset author_rewards_total_scorum_value = asset(0, SCORUM_SYMBOL); ///< SCR value of author rewards

    uint32_t curation_reward_payouts = 0; ///< Number of curation reward payouts.
    asset curation_rewards_vests = asset(0, VESTS_SYMBOL); ///< SP paid for curation rewards
    asset curation_rewards_scorum_value = asset(0, SCORUM_SYMBOL); ///< SCR value of curation rewards
};
// clang-format on

struct account_statistic : public account_metric
{
    account_statistic& operator+=(const account_metric&);
};

struct statistics
{
    std::map<account_name_type, account_statistic> statistic_map;

    statistics& operator+=(const bucket_object&);
};
}
} // scorum::account_statistics

FC_REFLECT(scorum::account_statistics::account_metric,
           (signed_transactions)(market_bandwidth)(non_market_bandwidth)(total_ops)(market_ops)(forum_ops)(
               root_comments)(root_comment_edits)(root_comments_deleted)(replies)(reply_edits)(replies_deleted)(
               new_root_votes)(changed_root_votes)(new_reply_votes)(changed_reply_votes)(author_reward_payouts)(
               author_rewards_vests)(author_rewards_total_scorum_value)(curation_reward_payouts)(
               curation_rewards_vests)(curation_rewards_scorum_value)(transfers_to)(transfers_from)(scorum_sent)(
               scorum_received)(transfers_to_vesting)(scorum_transferred_to_vesting)(vests_received_by_transfers)(
               new_vesting_withdrawal_requests)(modified_vesting_withdrawal_requests)(vesting_withdrawals_processed)(
               finished_vesting_withdrawals)(vests_withdrawn)(scorum_received_from_withdrawls)(
               scorum_received_from_routes)(vests_received_from_routes))

FC_REFLECT_DERIVED(scorum::account_statistics::account_statistic,
                   (scorum::account_statistics::account_metric),
                   BOOST_PP_SEQ_NIL)

FC_REFLECT(scorum::account_statistics::statistics, (statistic_map))
