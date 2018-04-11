#include <scorum/blockchain_statistics/schema/metrics.hpp>
#include <scorum/blockchain_statistics/schema/bucket_object.hpp>

namespace scorum {
namespace blockchain_statistics {

statistics& statistics::operator+=(const bucket_object& b)
{
    this->blocks += b.blocks;
    this->bandwidth += b.bandwidth;
    this->operations += b.operations;
    this->transactions += b.transactions;
    this->transfers += b.transfers;
    this->scorum_transferred += b.scorum_transferred;
    this->paid_accounts_created += b.paid_accounts_created;
    this->free_accounts_created += b.free_accounts_created;
    this->root_comments += b.root_comments;
    this->root_comment_edits += b.root_comment_edits;
    this->root_comments_deleted += b.root_comments_deleted;
    this->replies += b.replies;
    this->reply_edits += b.reply_edits;
    this->replies_deleted += b.replies_deleted;
    this->new_root_votes += b.new_root_votes;
    this->changed_root_votes += b.changed_root_votes;
    this->new_reply_votes += b.new_reply_votes;
    this->changed_reply_votes += b.changed_reply_votes;
    this->payouts += b.payouts;
    this->scr_paid_to_authors += b.scr_paid_to_authors;
    this->scorumpower_paid_to_authors += b.scorumpower_paid_to_authors;
    this->scorumpower_paid_to_curators += b.scorumpower_paid_to_curators;
    this->transfers_to_scorumpower += b.transfers_to_scorumpower;
    this->scorum_transferred_to_scorumpower += b.scorum_transferred_to_scorumpower;
    this->new_vesting_withdrawal_requests += b.new_vesting_withdrawal_requests;
    this->vesting_withdraw_rate_delta += b.vesting_withdraw_rate_delta;
    this->modified_vesting_withdrawal_requests += b.modified_vesting_withdrawal_requests;
    this->vesting_withdrawals_processed += b.vesting_withdrawals_processed;
    this->finished_vesting_withdrawals += b.finished_vesting_withdrawals;
    this->scorumpower_withdrawn += b.scorumpower_withdrawn;
    this->scorumpower_transferred += b.scorumpower_transferred;

    // total
    this->total_accounts_created += b.paid_accounts_created + b.free_accounts_created;
    this->total_comments += b.root_comments + b.replies;
    this->total_comment_edits += b.root_comment_edits + b.reply_edits;
    this->total_comments_deleted += b.root_comments_deleted + b.replies_deleted;
    this->new_votes += b.new_root_votes + b.new_reply_votes;
    this->changed_votes += b.changed_root_votes + b.changed_reply_votes;
    this->total_votes += b.new_root_votes + b.changed_root_votes + b.new_reply_votes + b.changed_reply_votes;
    this->total_root_votes += b.new_root_votes + b.changed_root_votes;
    this->total_reply_votes += b.new_reply_votes + b.changed_reply_votes;

    for (auto& item : b.missed_blocks)
    {
        this->missed_blocks[item.first] = item.second;
    }

    return (*this);
}
}
} // scorum::blockchain_statistics
