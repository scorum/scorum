#include <scorum/blockchain_statistics/api/statistic_api_object.hpp>
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
    this->accounts_created += b.paid_accounts_created + b.mined_accounts_created;
    this->paid_accounts_created += b.paid_accounts_created;
    this->mined_accounts_created += b.mined_accounts_created;
    this->total_comments += b.root_comments + b.replies;
    this->total_comment_edits += b.root_comment_edits + b.reply_edits;
    this->total_comments_deleted += b.root_comments_deleted + b.replies_deleted;
    this->root_comments += b.root_comments;
    this->root_comment_edits += b.root_comment_edits;
    this->root_comments_deleted += b.root_comments_deleted;
    this->replies += b.replies;
    this->reply_edits += b.reply_edits;
    this->replies_deleted += b.replies_deleted;
    this->total_votes += b.new_root_votes + b.changed_root_votes + b.new_reply_votes + b.changed_reply_votes;
    this->new_votes += b.new_root_votes + b.new_reply_votes;
    this->changed_votes += b.changed_root_votes + b.changed_reply_votes;
    this->total_root_votes += b.new_root_votes + b.changed_root_votes;
    this->new_root_votes += b.new_root_votes;
    this->changed_root_votes += b.changed_root_votes;
    this->total_reply_votes += b.new_reply_votes + b.changed_reply_votes;
    this->new_reply_votes += b.new_reply_votes;
    this->changed_reply_votes += b.changed_reply_votes;
    this->payouts += b.payouts;
    this->scr_paid_to_authors += b.scr_paid_to_authors;
    this->vests_paid_to_authors += b.vests_paid_to_authors;
    this->vests_paid_to_curators += b.vests_paid_to_curators;
    this->transfers_to_vesting += b.transfers_to_vesting;
    this->scorum_vested += b.scorum_vested;
    this->new_vesting_withdrawal_requests += b.new_vesting_withdrawal_requests;
    this->vesting_withdraw_rate_delta += b.vesting_withdraw_rate_delta;
    this->modified_vesting_withdrawal_requests += b.modified_vesting_withdrawal_requests;
    this->vesting_withdrawals_processed += b.vesting_withdrawals_processed;
    this->finished_vesting_withdrawals += b.finished_vesting_withdrawals;
    this->vests_withdrawn += b.vests_withdrawn;
    this->vests_transferred += b.vests_transferred;
    this->scorum_converted += b.scorum_converted;
    this->limit_orders_created += b.limit_orders_created;
    this->limit_orders_filled += b.limit_orders_filled;
    this->limit_orders_cancelled += b.limit_orders_cancelled;
    this->total_pow += b.total_pow;
    this->estimated_hashpower += b.estimated_hashpower;

    return (*this);
}
}
} // scorum::blockchain_statistics
