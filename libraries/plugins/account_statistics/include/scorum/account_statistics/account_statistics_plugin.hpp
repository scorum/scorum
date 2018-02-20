#include <scorum/app/plugin.hpp>

#include <boost/multi_index/composite_key.hpp>

//
// Plugins should #define their SPACE_ID's so plugins with
// conflicting SPACE_ID assignments can be compiled into the
// same binary (by simply re-assigning some of the conflicting #defined
// SPACE_ID's in a build script).
//
// Assignment of SPACE_ID's cannot be done at run-time because
// various template automagic depends on them being known at compile
// time.
//
#ifndef ACCOUNT_STATISTICS_SPACE_ID
#define ACCOUNT_STATISTICS_SPACE_ID 10
#endif

#ifndef ACCOUNT_STATISTICS_PLUGIN_NAME
#define ACCOUNT_STATISTICS_PLUGIN_NAME "account_stats"
#endif

namespace scorum {
namespace account_statistics {

using namespace chain;
using app::application;

enum account_statistics_plugin_object_types
{
    account_stats_bucket_object_type = (ACCOUNT_STATISTICS_SPACE_ID << 8),
    account_activity_bucket_object_type = (ACCOUNT_STATISTICS_SPACE_ID << 8) + 1
};

struct account_stats_bucket_object : public object<account_stats_bucket_object_type, account_stats_bucket_object>
{
    CHAINBASE_DEFAULT_CONSTRUCTOR(account_stats_bucket_object)

    id_type id;

    fc::time_point_sec open; ///< Open time of the bucket
    uint32_t seconds = 0; ///< Seconds accounted for in the bucket
    account_name_type name; ///< Account name
    uint32_t transactions = 0; ///< Transactions this account signed
    uint32_t market_bandwidth = 0; ///< Charged bandwidth for market transactions
    uint32_t non_market_bandwidth = 0; ///< Charged bandwidth for non-market transactions
    uint32_t total_ops = 0; ///< Ops this account was an authority on
    uint32_t market_ops = 0; ///< Market operations
    uint32_t forum_ops = 0; ///< Forum operations
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
    share_type author_rewards_vests = 0; ///< SP paid for author rewards
    share_type author_rewards_total_scorum_value = 0; ///< SCR value of author rewards
    uint32_t curation_reward_payouts = 0; ///< Number of curation reward payouts.
    share_type curation_rewards_vests = 0; ///< SP paid for curation rewards
    share_type curation_rewards_scorum_value = 0; ///< SCR value of curation rewards
    uint32_t transfers_to = 0; ///< Account to account transfers to this account
    uint32_t transfers_from = 0; ///< Account to account transfers from this account
    share_type scorum_sent = 0; ///< SCR sent from this account
    share_type scorum_received = 0; ///< SCR received by this account
    uint32_t transfers_to_vesting = 0; ///< Transfers to vesting by this account. Note: Transfer to vesting from A to B
    /// counts as a transfer from A to B followed by a vesting deposit by B.
    share_type scorum_transferred_to_vesting = 0; ///< SCR vested by the account
    share_type new_vests = 0; ///< New SP by vesting transfers
    uint32_t new_vesting_withdrawal_requests = 0; ///< New vesting withdrawal requests
    uint32_t modified_vesting_withdrawal_requests = 0; ///< Changes to vesting withdraw requests
    uint32_t vesting_withdrawals_processed = 0; ///< Vesting withdrawals processed for this account
    uint32_t finished_vesting_withdrawals = 0; ///< Processed vesting withdrawals that are now finished
    share_type vests_withdrawn = 0; ///< SP withdrawn from the account
    share_type scorum_received_from_withdrawls = 0; ///< SCR received from this account's vesting withdrawals
    share_type scorum_received_from_routes = 0; ///< SCR received from another account's vesting withdrawals
    share_type vests_received_from_routes = 0; ///< SP received from another account's vesting withdrawals
};

typedef account_stats_bucket_object::id_type account_stats_bucket_id_type;

struct account_activity_bucket_object
    : public object<account_activity_bucket_object_type, account_activity_bucket_object>
{
    CHAINBASE_DEFAULT_CONSTRUCTOR(account_activity_bucket_object)

    id_type id;

    fc::time_point_sec open; ///< Open time for the bucket
    uint32_t seconds = 0; ///< Seconds accounted for in the bucket
    uint32_t active_market_accounts = 0; ///< Active market accounts in the bucket
    uint32_t active_forum_accounts = 0; ///< Active forum accounts in the bucket
    uint32_t active_market_and_forum_accounts = 0; ///< Active accounts in both the market and the forum
};

typedef account_activity_bucket_object::id_type account_activity_bucket_id_type;

namespace detail {
class account_statistics_plugin_impl;
}

class account_statistics_plugin : public scorum::app::plugin
{
public:
    account_statistics_plugin(application* app);
    virtual ~account_statistics_plugin();

    virtual std::string plugin_name() const override
    {
        return ACCOUNT_STATISTICS_PLUGIN_NAME;
    }
    virtual void plugin_set_program_options(boost::program_options::options_description& cli,
                                            boost::program_options::options_description& cfg) override;
    virtual void plugin_initialize(const boost::program_options::variables_map& options) override;
    virtual void plugin_startup() override;

    const flat_set<uint32_t>& get_tracked_buckets() const;
    uint32_t get_max_history_per_bucket() const;
    const flat_set<std::string>& get_tracked_accounts() const;

private:
    friend class detail::account_statistics_plugin_impl;
    std::unique_ptr<detail::account_statistics_plugin_impl> _my;
};
}
} // scorum::account_statistics

FC_REFLECT(
    scorum::account_statistics::account_stats_bucket_object,
    (id)(open)(seconds)(name)(transactions)(market_bandwidth)(non_market_bandwidth)(total_ops)(market_ops)(forum_ops)(
        root_comments)(root_comment_edits)(root_comments_deleted)(replies)(reply_edits)(replies_deleted)(
        new_root_votes)(changed_root_votes)(new_reply_votes)(changed_reply_votes)(author_reward_payouts)(
        author_rewards_vests)(author_rewards_total_scorum_value)(curation_reward_payouts)(curation_rewards_vests)(
        curation_rewards_scorum_value)(transfers_to)(transfers_from)(scorum_sent)(scorum_received)(
        transfers_to_vesting)(scorum_transferred_to_vesting)(new_vests)(new_vesting_withdrawal_requests)(
        modified_vesting_withdrawal_requests)(vesting_withdrawals_processed)(finished_vesting_withdrawals)(
        vests_withdrawn)(scorum_received_from_withdrawls)(scorum_received_from_routes)(vests_received_from_routes))

// clang-format off

FC_REFLECT(
   scorum::account_statistics::account_activity_bucket_object,
   (id)
   (open)
   (seconds)
   (active_market_accounts)
   (active_forum_accounts)
   (active_market_and_forum_accounts)
)

// clang-format on
