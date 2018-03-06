#include <scorum/account_statistics/schema/metrics.hpp>
#include <scorum/account_statistics/schema/objects.hpp>

namespace scorum {
namespace account_statistics {

account_statistic& account_statistic::operator+=(const account_metric& stat)
{
    this->signed_transactions += stat.signed_transactions;

    // bandwidth
    this->market_bandwidth += stat.market_bandwidth;
    this->non_market_bandwidth += stat.non_market_bandwidth;
    this->total_ops += stat.total_ops;
    this->market_ops += stat.market_ops;
    this->forum_ops += stat.forum_ops;

    // money
    this->transfers_from += stat.transfers_from;
    this->transfers_to += stat.transfers_to;
    this->transfers_to_vesting += stat.transfers_to_vesting;

    this->scorum_sent += stat.scorum_sent;
    this->scorum_received += stat.scorum_received;
    this->scorum_transferred_to_vesting += stat.scorum_transferred_to_vesting;
    this->scorumpower_received_by_transfers += stat.scorumpower_received_by_transfers;

    this->new_vesting_withdrawal_requests += stat.new_vesting_withdrawal_requests;
    this->modified_vesting_withdrawal_requests += stat.modified_vesting_withdrawal_requests;
    this->vesting_withdrawals_processed += stat.vesting_withdrawals_processed;
    this->finished_vesting_withdrawals += stat.finished_vesting_withdrawals;

    this->scorumpower_withdrawn += stat.scorumpower_withdrawn;
    this->scorum_received_from_withdrawls += stat.scorum_received_from_withdrawls;
    this->scorum_received_from_routes += stat.scorum_received_from_routes;
    this->scorumpower_received_from_routes += stat.scorumpower_received_from_routes;

    // comments
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

    this->author_reward_payouts += stat.author_reward_payouts;
    this->author_rewards_scorumpower += stat.author_rewards_scorumpower;
    this->author_rewards_total_scorum_value += stat.author_rewards_total_scorum_value;

    this->curation_reward_payouts += stat.curation_reward_payouts;
    this->curation_rewards_scorumpower += stat.curation_rewards_scorumpower;
    this->curation_rewards_scorum_value += stat.curation_rewards_scorum_value;

    return (*this);
}

//////////////////////////////////////////////////////////////////////////
statistics& statistics::operator+=(const bucket_object& bucket)
{
    for (auto& item : bucket.account_statistic)
    {
        statistic_map[item.first] += item.second;
    }

    return (*this);
}
}
} // scorum::account_statistics
