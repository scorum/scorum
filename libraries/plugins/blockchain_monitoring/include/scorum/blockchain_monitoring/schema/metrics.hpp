#pragma once

#include <scorum/protocol/asset.hpp>

namespace scorum {
namespace blockchain_monitoring {

using scorum::protocol::share_type;
using scorum::protocol::account_name_type;

struct bucket_object;

/// @addtogroup blockchain_statistics_api
/// @{

struct base_metric
{
    // chain
    uint32_t blocks = 0; ///< Blocks produced
    uint32_t bandwidth = 0; ///< Bandwidth in bytes
    uint32_t operations = 0; ///< Operations evaluated
    uint32_t transactions = 0; ///< Transactions processed

    // money
    uint32_t paid_accounts_created = 0; ///< Accounts created with fee
    uint32_t free_accounts_created = 0; ///< Accounts created with fee

    uint32_t transfers = 0; ///< Account/devpool to account transfers
    uint32_t transfers_to_scorumpower = 0; ///< Transfers of SCR into SP
    uint32_t new_vesting_withdrawal_requests = 0; ///< New vesting withdrawal requests
    uint32_t modified_vesting_withdrawal_requests = 0; ///< Changes to vesting withdrawal requests
    uint32_t vesting_withdrawals_processed = 0; ///< Number of vesting withdrawals
    uint32_t finished_vesting_withdrawals = 0; ///< Processed vesting withdrawals that are now finished

    share_type scorum_transferred = 0; ///< SCR transferred from account to account
    share_type scorum_transferred_to_scorumpower = 0; ///< Amount of SCR vested
    share_type scorumpower_withdrawn = 0; ///< Amount of SP withdrawn to SCR
    share_type scorumpower_transferred = 0; ///< Amount of SP transferred to another account
    share_type vesting_withdraw_rate_delta = 0;

    // comments
    uint32_t root_comments = 0; ///< Top level root comments
    uint32_t root_comment_edits = 0; ///< Edits to root comments
    uint32_t root_comments_deleted = 0; ///< Root comments deleted
    uint32_t replies = 0; ///< Replies to comments
    uint32_t reply_edits = 0; ///< Edits to replies
    uint32_t replies_deleted = 0; ///< Replies deleted
    uint32_t new_root_votes = 0; ///< New votes on root comments
    uint32_t changed_root_votes = 0; ///< Changed votes on root comments
    uint32_t new_reply_votes = 0; ///< New votes on replies
    uint32_t changed_reply_votes = 0; ///< Changed votes on replies
    uint32_t payouts = 0; ///< Number of comment payouts
    share_type scr_paid_to_authors = 0; ///< Amount of SCR paid to authors
    share_type scorumpower_paid_to_authors = 0; ///< Amount of SP paid to authors
    share_type scr_paid_to_curators = 0; ///< Amount of SCR paid to curators
    share_type scorumpower_paid_to_curators = 0; ///< Amount of SP paid to curators
};

struct total_metric
{
    uint32_t total_accounts_created = 0; ///< Total accounts created
    uint32_t total_comments = 0; ///< Total comments
    uint32_t total_comment_edits = 0; ///< Edits to comments
    uint32_t total_comments_deleted = 0; ///< Comments deleted
    uint32_t total_votes = 0; ///< Total votes on all comments
    uint32_t new_votes = 0; ///< New votes on comments
    uint32_t changed_votes = 0; ///< Changed votes on comments
    uint32_t total_root_votes = 0; ///< Total votes on root comments
    uint32_t total_reply_votes = 0; ///< Total votes on replies
};

struct statistics : public base_metric, public total_metric
{
    statistics& operator+=(const bucket_object&);

    std::map<uint32_t, std::string> missed_blocks; ///< map missed block to witness which missed
};

/// @}

} // namespace blockchain_monitoring
} // namespace scorum

FC_REFLECT(scorum::blockchain_monitoring::base_metric,
           (blocks)(bandwidth)(operations)(transactions)(transfers)(scorum_transferred)(paid_accounts_created)(
               free_accounts_created)(root_comments)(root_comment_edits)(root_comments_deleted)(replies)(reply_edits)(
               replies_deleted)(new_root_votes)(changed_root_votes)(new_reply_votes)(changed_reply_votes)(payouts)(
               scr_paid_to_authors)(scorumpower_paid_to_authors)(scr_paid_to_curators)(scorumpower_paid_to_curators)(
               transfers_to_scorumpower)(scorum_transferred_to_scorumpower)(new_vesting_withdrawal_requests)(
               modified_vesting_withdrawal_requests)(vesting_withdraw_rate_delta)(vesting_withdrawals_processed)(
               finished_vesting_withdrawals)(scorumpower_withdrawn)(scorumpower_transferred))

FC_REFLECT(scorum::blockchain_monitoring::total_metric,
           (total_accounts_created)(total_comments)(total_comment_edits)(total_comments_deleted)(total_votes)(
               new_votes)(changed_votes)(total_root_votes)(total_reply_votes))

FC_REFLECT_DERIVED(scorum::blockchain_monitoring::statistics,
                   (scorum::blockchain_monitoring::base_metric)(scorum::blockchain_monitoring::total_metric),
                   (missed_blocks))
