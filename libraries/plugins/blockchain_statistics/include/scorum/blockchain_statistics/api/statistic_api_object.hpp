#pragma once

#include <scorum/protocol/asset.hpp>

namespace scorum {
namespace blockchain_statistics {

using protocol::share_type;

struct bucket_object;

struct statistics
{
    uint32_t blocks = 0; ///< Blocks produced
    uint32_t bandwidth = 0; ///< Bandwidth in bytes
    uint32_t operations = 0; ///< Operations evaluated
    uint32_t transactions = 0; ///< Transactions processed
    uint32_t transfers = 0; ///< Account to account transfers
    share_type scorum_transferred = 0; ///< SCR transferred from account to account
    uint32_t accounts_created = 0; ///< Total accounts created
    uint32_t paid_accounts_created = 0; ///< Accounts created with fee
    uint32_t mined_accounts_created = 0; ///< Accounts mined for free
    uint32_t total_comments = 0; ///< Total comments
    uint32_t total_comment_edits = 0; ///< Edits to comments
    uint32_t total_comments_deleted = 0; ///< Comments deleted
    uint32_t root_comments = 0; ///< Top level root comments
    uint32_t root_comment_edits = 0; ///< Edits to root comments
    uint32_t root_comments_deleted = 0; ///< Root comments deleted
    uint32_t replies = 0; ///< Replies to comments
    uint32_t reply_edits = 0; ///< Edits to replies
    uint32_t replies_deleted = 0; ///< Replies deleted
    uint32_t total_votes = 0; ///< Total votes on all comments
    uint32_t new_votes = 0; ///< New votes on comments
    uint32_t changed_votes = 0; ///< Changed votes on comments
    uint32_t total_root_votes = 0; ///< Total votes on root comments
    uint32_t new_root_votes = 0; ///< New votes on root comments
    uint32_t changed_root_votes = 0; ///< Changed votes on root comments
    uint32_t total_reply_votes = 0; ///< Total votes on replies
    uint32_t new_reply_votes = 0; ///< New votes on replies
    uint32_t changed_reply_votes = 0; ///< Changed votes on replies
    uint32_t payouts = 0; ///< Number of comment payouts
    share_type scr_paid_to_authors = 0; ///< Ammount of SCR paid to authors
    share_type vests_paid_to_authors = 0; ///< Ammount of SP paid to authors
    share_type vests_paid_to_curators = 0; ///< Ammount of SP paid to curators
    uint32_t transfers_to_vesting = 0; ///< Transfers of SCR into SP
    share_type scorum_vested = 0; ///< Ammount of SCR vested
    uint32_t new_vesting_withdrawal_requests = 0; ///< New vesting withdrawal requests
    uint32_t modified_vesting_withdrawal_requests = 0; ///< Changes to vesting withdrawal requests
    share_type vesting_withdraw_rate_delta = 0;
    uint32_t vesting_withdrawals_processed = 0; ///< Number of vesting withdrawals
    uint32_t finished_vesting_withdrawals = 0; ///< Processed vesting withdrawals that are now finished
    share_type vests_withdrawn = 0; ///< Ammount of SP withdrawn to SCR
    share_type vests_transferred = 0; ///< Ammount of SP transferred to another account
    share_type scorum_converted = 0; ///< Amount of SCR that was converted
    uint32_t limit_orders_created = 0; ///< Limit orders created
    uint32_t limit_orders_filled = 0; ///< Limit orders filled
    uint32_t limit_orders_cancelled = 0; ///< Limit orders cancelled
    uint32_t total_pow = 0; ///< POW submitted
    uint128_t estimated_hashpower = 0; ///< Estimated average hashpower over interval

    statistics& operator+=(const bucket_object& b);
};
}
} // scorum::blockchain_statistics

FC_REFLECT(
    scorum::blockchain_statistics::statistics,
    (blocks)(bandwidth)(operations)(transactions)(transfers)(scorum_transferred)(accounts_created)(
        paid_accounts_created)(mined_accounts_created)(total_comments)(total_comment_edits)(total_comments_deleted)(
        root_comments)(root_comment_edits)(root_comments_deleted)(replies)(reply_edits)(replies_deleted)(total_votes)(
        new_votes)(changed_votes)(total_root_votes)(new_root_votes)(changed_root_votes)(total_reply_votes)(
        new_reply_votes)(changed_reply_votes)(payouts)(scr_paid_to_authors)(vests_paid_to_authors)(
        vests_paid_to_curators)(transfers_to_vesting)(scorum_vested)(new_vesting_withdrawal_requests)(
        modified_vesting_withdrawal_requests)(vesting_withdraw_rate_delta)(vesting_withdrawals_processed)(
        finished_vesting_withdrawals)(vests_withdrawn)(vests_transferred)(scorum_converted)(limit_orders_created)(
        limit_orders_filled)(limit_orders_cancelled)(total_pow)(estimated_hashpower))
