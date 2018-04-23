#include <scorum/blockchain_statistics/blockchain_statistics_plugin.hpp>
#include <scorum/blockchain_statistics/blockchain_statistics_api.hpp>
#include <scorum/blockchain_statistics/node_monitoring_api.hpp>
#include <scorum/common_statistics/base_plugin_impl.hpp>

#include <scorum/app/impacted.hpp>
#include <scorum/chain/schema/account_objects.hpp>
#include <scorum/chain/schema/comment_objects.hpp>
#include <scorum/chain/schema/withdraw_scorumpower_objects.hpp>

#include <scorum/chain/services/account.hpp>
#include <scorum/chain/services/comment.hpp>
#include <scorum/chain/services/withdraw_scorumpower.hpp>

#include <scorum/chain/database/database.hpp>
#include <scorum/chain/operation_notification.hpp>

namespace scorum {
namespace blockchain_statistics {

namespace detail {

using namespace scorum::protocol;

class blockchain_statistics_plugin_impl
    : public common_statistics::common_statistics_plugin_impl<bucket_object, blockchain_statistics_plugin>
{
public:
    blockchain_statistics_plugin_impl(blockchain_statistics_plugin& plugin)
        : base_plugin_impl(plugin)
    {
    }
    virtual ~blockchain_statistics_plugin_impl()
    {
    }

private:
    virtual void process_bucket_creation(const bucket_object& bucket)
    {
        auto& db = _self.database();

        db.modify<bucket_object>(bucket, [&](bucket_object& bo) { bo.blocks = 1; });
    }

    virtual void process_block(const bucket_object& bucket, const signed_block& b) override;

    virtual void process_pre_operation(const bucket_object& bucket, const operation_notification& o) override;

    virtual void process_post_operation(const bucket_object& bucket, const operation_notification& o) override;
};

class operation_process
{
private:
    chain::database& _db;
    const bucket_object& _bucket;

public:
    operation_process(chain::database& db, const bucket_object& b)
        : _db(db)
        , _bucket(b)
    {
    }

    typedef void result_type;

    template <typename T> void operator()(const T&) const
    {
    }

    void operator()(const transfer_operation& op) const
    {
        _db.modify(_bucket, [&](bucket_object& b) {
            b.transfers++;

            if (op.amount.symbol() == SCORUM_SYMBOL)
                b.scorum_transferred += op.amount.amount;
        });
    }

    void operator()(const account_create_operation& op) const
    {
        _db.modify(_bucket, [&](bucket_object& b) { b.paid_accounts_created++; });
    }

    void operator()(const account_create_with_delegation_operation& op) const
    {
        _db.modify(_bucket, [&](bucket_object& b) { b.paid_accounts_created++; });
    }

    void operator()(const account_create_by_committee_operation& op) const
    {
        _db.modify(_bucket, [&](bucket_object& b) { b.free_accounts_created++; });
    }

    void operator()(const comment_operation& op) const
    {
        _db.modify(_bucket, [&](bucket_object& b) {
            auto& comment = _db.obtain_service<dbs_comment>().get(op.author, op.permlink);

            if (comment.created == _db.head_block_time())
            {
                if (comment.parent_author.length())
                    b.replies++;
                else
                    b.root_comments++;
            }
            else
            {
                if (comment.parent_author.length())
                    b.reply_edits++;
                else
                    b.root_comment_edits++;
            }
        });
    }

    void operator()(const vote_operation& op) const
    {
        _db.modify(_bucket, [&](bucket_object& b) {
            const auto& cv_idx = _db.get_index<comment_vote_index>().indices().get<by_comment_voter>();
            const auto& comment = _db.obtain_service<dbs_comment>().get(op.author, op.permlink);
            const auto& voter = _db.obtain_service<chain::dbs_account>().get_account(op.voter);
            const auto itr = cv_idx.find(boost::make_tuple(comment.id, voter.id));

            if (itr->num_changes)
            {
                if (comment.parent_author.size())
                    b.new_reply_votes++;
                else
                    b.new_root_votes++;
            }
            else
            {
                if (comment.parent_author.size())
                    b.changed_reply_votes++;
                else
                    b.changed_root_votes++;
            }
        });
    }

    void operator()(const author_reward_operation& op) const
    {
        _db.modify(_bucket, [&](bucket_object& b) {
            b.payouts++;
            b.scr_paid_to_authors += op.scorum_payout.amount;
            b.scorumpower_paid_to_authors += op.vesting_payout.amount;
        });
    }

    void operator()(const curation_reward_operation& op) const
    {
        _db.modify(_bucket, [&](bucket_object& b) { b.scorumpower_paid_to_curators += op.reward.amount; });
    }

    void operator()(const transfer_to_scorumpower_operation& op) const
    {
        _db.modify(_bucket, [&](bucket_object& b) {
            b.transfers_to_scorumpower++;
            b.scorum_transferred_to_scorumpower += op.amount.amount;
        });
    }

    void operator()(const fill_vesting_withdraw_operation& op) const
    {
        const auto& account = _db.obtain_service<chain::dbs_account>().get_account(op.from_account);
        const auto& withdraw_scorumpower_service = _db.obtain_service<chain::dbs_withdraw_scorumpower>();

        asset withdrawn = asset(0, SP_SYMBOL);
        asset to_withdraw = asset(0, SP_SYMBOL);
        asset vesting_withdraw_rate = asset(0, SP_SYMBOL);

        if (withdraw_scorumpower_service.is_exists(account.id))
        {
            const auto& wvo = withdraw_scorumpower_service.get(account.id);
            withdrawn = wvo.withdrawn;
            to_withdraw = wvo.to_withdraw;
            vesting_withdraw_rate = wvo.vesting_withdraw_rate;
        }

        _db.modify(_bucket, [&](bucket_object& b) {

            b.vesting_withdrawals_processed++;

            if (op.withdrawn.symbol() == SCORUM_SYMBOL)
                b.scorumpower_withdrawn += op.withdrawn.amount;
            else
                b.scorumpower_transferred += op.withdrawn.amount;

            if (withdrawn.amount + op.withdrawn.amount >= to_withdraw.amount
                || account.scorumpower.amount - op.withdrawn.amount == 0)
            {
                b.finished_vesting_withdrawals++;

                b.vesting_withdraw_rate_delta -= vesting_withdraw_rate.amount;
            }
        });
    }

    void operator()(const witness_miss_block_operation& op) const
    {
        _db.modify(_bucket, [&](bucket_object& b) { b.missed_blocks[op.block_num] = op.owner; });
    }
};

void blockchain_statistics_plugin_impl::process_block(const bucket_object& bucket, const signed_block& b)
{
    auto& db = _self.database();

    uint32_t trx_size = 0;
    uint32_t num_trx = b.transactions.size();

    for (const auto& trx : b.transactions)
    {
        trx_size += fc::raw::pack_size(trx);
    }

    db.modify(bucket, [&](bucket_object& bo) {
        bo.blocks++;
        bo.transactions += num_trx;
        bo.bandwidth += trx_size;
    });
}

void blockchain_statistics_plugin_impl::process_pre_operation(const bucket_object& bucket,
                                                              const operation_notification& o)
{
    auto& db = _self.database();

    if (o.op.which() == operation::tag<delete_comment_operation>::value)
    {
        delete_comment_operation op = o.op.get<delete_comment_operation>();
        auto comment = db.obtain_service<dbs_comment>().get(op.author, op.permlink);

        db.modify(bucket, [&](bucket_object& b) {
            if (comment.parent_author.length())
                b.replies_deleted++;
            else
                b.root_comments_deleted++;
        });
    }
    else if (o.op.which() == operation::tag<withdraw_scorumpower_operation>::value)
    {
        withdraw_scorumpower_operation op = o.op.get<withdraw_scorumpower_operation>();
        const auto& account = db.obtain_service<chain::dbs_account>().get_account(op.account);

        auto new_vesting_withdrawal_rate = op.scorumpower.amount / SCORUM_VESTING_WITHDRAW_INTERVALS;
        if (op.scorumpower.amount > 0 && new_vesting_withdrawal_rate == 0)
            new_vesting_withdrawal_rate = 1;

        const auto& withdraw_scorumpower_service = db.obtain_service<chain::dbs_withdraw_scorumpower>();

        asset vesting_withdraw_rate = asset(0, SP_SYMBOL);

        if (withdraw_scorumpower_service.is_exists(account.id))
        {
            const auto& wvo = withdraw_scorumpower_service.get(account.id);
            vesting_withdraw_rate = wvo.vesting_withdraw_rate;
        }

        db.modify(bucket, [&](bucket_object& b) {
            if (vesting_withdraw_rate.amount > 0)
                b.modified_vesting_withdrawal_requests++;
            else
                b.new_vesting_withdrawal_requests++;

            b.vesting_withdraw_rate_delta += new_vesting_withdrawal_rate - vesting_withdraw_rate.amount;
        });
    }
}

void blockchain_statistics_plugin_impl::process_post_operation(const bucket_object& bucket,
                                                               const operation_notification& o)
{
    auto& db = _self.database();

    if (!is_virtual_operation(o.op))
    {
        db.modify(bucket, [&](bucket_object& b) { b.operations++; });
    }
    o.op.visit(operation_process(db, bucket));
}

} // detail

blockchain_statistics_plugin::blockchain_statistics_plugin(application* app)
    : plugin(app)
    , _my(new detail::blockchain_statistics_plugin_impl(*this))
{
}

blockchain_statistics_plugin::~blockchain_statistics_plugin()
{
}

void blockchain_statistics_plugin::plugin_set_program_options(boost::program_options::options_description& cli,
                                                              boost::program_options::options_description& cfg)
{
    cli.add_options()(
        "chain-stats-bucket-size",
        boost::program_options::value<std::string>()->default_value("[60,3600,21600,86400,604800,2592000]"),
        "Track blockchain statistics by grouping orders into buckets of equal size measured in seconds specified as a "
        "JSON array of numbers")(
        "chain-stats-history-per-bucket", boost::program_options::value<uint32_t>()->default_value(100),
        "How far back in time to track history for each bucket size, measured in the number of buckets (default: 100)");
    cfg.add(cli);
}

void blockchain_statistics_plugin::plugin_initialize(const boost::program_options::variables_map& options)
{
    try
    {
        if (options.count("chain-stats-bucket-size"))
        {
            const std::string& buckets = options["chain-stats-bucket-size"].as<std::string>();
            _my->_tracked_buckets = fc::json::from_string(buckets).as<flat_set<uint32_t>>();
        }
        if (options.count("chain-stats-history-per-bucket"))
            _my->_maximum_history_per_bucket_size = options["chain-stats-history-per-bucket"].as<uint32_t>();

        ilog("chain-stats-bucket-size: ${b}", ("b", _my->_tracked_buckets));
        ilog("chain-stats-history-per-bucket: ${h}", ("h", _my->_maximum_history_per_bucket_size));

        _my->initialize();
    }
    FC_CAPTURE_AND_RETHROW()

    print_greeting();
}

void blockchain_statistics_plugin::plugin_startup()
{
    app().register_api_factory<blockchain_statistics_api>(API_BLOCKCHAIN_STATISTICS);
    app().register_api_factory<node_monitoring_api>(API_NODE_MONITORING);
}

const flat_set<uint32_t>& blockchain_statistics_plugin::get_tracked_buckets() const
{
    return _my->_tracked_buckets;
}

uint32_t blockchain_statistics_plugin::get_max_history_per_bucket() const
{
    return _my->_maximum_history_per_bucket_size;
}
}
} // scorum::blockchain_statistics

SCORUM_DEFINE_PLUGIN(blockchain_statistics, scorum::blockchain_statistics::blockchain_statistics_plugin);
