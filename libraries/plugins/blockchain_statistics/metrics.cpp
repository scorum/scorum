#include <scorum/blockchain_statistics/schema/metrics.hpp>

namespace scorum {
namespace blockchain_statistics {

statistics& statistics::operator+=(const base_metric& stat)
{
    this->blocks += stat.blocks;
    this->bandwidth += stat.bandwidth;
    this->operations += stat.operations;
    this->transactions += stat.transactions;
    this->transfers += stat.transfers;
    this->scorum_transferred += stat.scorum_transferred;
    this->paid_accounts_created += stat.paid_accounts_created;
    this->free_accounts_created += stat.free_accounts_created;
    this->root_comments += stat.root_comments;
    this->root_comment_edits += stat.root_comment_edits;
    this->root_comments_deleted += stat.root_comments_deleted;
    this->replies += stat.replies;
    this->reply_edits += stat.reply_edits;
    this->replies_deleted += stat.replies_deleted;
    this->new_root_votes += stat.new_root_votes;
    this->changed_root_votes += stat.changed_root_votes;
    this->new_reply_votes += stat.new_reply_votes;
    this->changed_reply_votes += stat.changed_reply_votes;
    this->payouts += stat.payouts;
    this->scr_paid_to_authors += stat.scr_paid_to_authors;
    this->scorumpower_paid_to_authors += stat.scorumpower_paid_to_authors;
    this->scorumpower_paid_to_curators += stat.scorumpower_paid_to_curators;
    this->transfers_to_vesting += stat.transfers_to_vesting;
    this->scorum_transferred_to_vesting += stat.scorum_transferred_to_vesting;
    this->new_vesting_withdrawal_requests += stat.new_vesting_withdrawal_requests;
    this->vesting_withdraw_rate_delta += stat.vesting_withdraw_rate_delta;
    this->modified_vesting_withdrawal_requests += stat.modified_vesting_withdrawal_requests;
    this->vesting_withdrawals_processed += stat.vesting_withdrawals_processed;
    this->finished_vesting_withdrawals += stat.finished_vesting_withdrawals;
    this->scorumpower_withdrawn += stat.scorumpower_withdrawn;
    this->scorumpower_transferred += stat.scorumpower_transferred;

    // total
    this->total_accounts_created += stat.paid_accounts_created + stat.free_accounts_created;
    this->total_comments += stat.root_comments + stat.replies;
    this->total_comment_edits += stat.root_comment_edits + stat.reply_edits;
    this->total_comments_deleted += stat.root_comments_deleted + stat.replies_deleted;
    this->new_votes += stat.new_root_votes + stat.new_reply_votes;
    this->changed_votes += stat.changed_root_votes + stat.changed_reply_votes;
    this->total_votes
        += stat.new_root_votes + stat.changed_root_votes + stat.new_reply_votes + stat.changed_reply_votes;
    this->total_root_votes += stat.new_root_votes + stat.changed_root_votes;
    this->total_reply_votes += stat.new_reply_votes + stat.changed_reply_votes;

    return (*this);
}
}
} // scorum::blockchain_statistics
