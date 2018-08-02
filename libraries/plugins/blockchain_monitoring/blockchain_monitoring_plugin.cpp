#include <scorum/blockchain_monitoring/blockchain_monitoring_plugin.hpp>
#include <scorum/blockchain_monitoring/blockchain_statistics_api.hpp>
#include <scorum/blockchain_monitoring/node_monitoring_api.hpp>
#include <scorum/common_statistics/base_plugin_impl.hpp>

#include <scorum/chain/schema/account_objects.hpp>
#include <scorum/chain/schema/comment_objects.hpp>
#include <scorum/chain/schema/withdraw_scorumpower_objects.hpp>

#include <scorum/chain/services/account.hpp>
#include <scorum/chain/services/comment.hpp>
#include <scorum/chain/services/withdraw_scorumpower.hpp>
#include <scorum/chain/services/dev_pool.hpp>

#include <scorum/chain/database/database.hpp>
#include <scorum/chain/operation_notification.hpp>

#include <chrono>

namespace scorum {
namespace blockchain_monitoring {

namespace detail {

using namespace scorum::protocol;

//////////////////////////////////////////////////////////////////////////
class perfomance_timer
{
    typedef std::chrono::high_resolution_clock Clock;
    Clock::time_point _block_start_time = Clock::time_point::min();
    Clock::duration _last_block_processing_duration = Clock::duration::zero();

    void start()
    {
        _block_start_time = Clock::now();
    }

    void stop()
    {
        if (_block_start_time != Clock::time_point::min())
        {
            _last_block_processing_duration = Clock::now() - _block_start_time;
            _block_start_time = Clock::time_point::min();
        }
    }

public:
    perfomance_timer(chain::database& db)
    {
        db.pre_applied_block.connect([&](const signed_block& b) { this->start(); });
        db.applied_block.connect([&](const signed_block& b) { this->stop(); });
    }

    std::chrono::microseconds get_last_block_duration() const
    {
        return std::chrono::duration_cast<std::chrono::microseconds>(_last_block_processing_duration);
    }
};
//////////////////////////////////////////////////////////////////////////
class blockchain_monitoring_plugin_impl
    : public common_statistics::common_statistics_plugin_impl<bucket_object, blockchain_monitoring_plugin>
{
public:
    perfomance_timer _timer;

    blockchain_monitoring_plugin_impl(blockchain_monitoring_plugin& plugin)
        : base_plugin_impl(plugin)
        , _timer(plugin.database())
    {
    }
    virtual ~blockchain_monitoring_plugin_impl()
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

    template <typename TSourceId>
    void collect_withdraw_stats(const bucket_object& bucket, const asset& vesting_shares, const TSourceId& source_id);
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
            auto reward_symbol = op.reward.symbol();
            if (SCORUM_SYMBOL == reward_symbol)
            {
                b.scr_paid_to_authors += op.reward.amount;
            }
            else if (SP_SYMBOL == reward_symbol)
            {
                b.scorumpower_paid_to_authors += op.reward.amount;
            }
        });
    }

    void operator()(const curation_reward_operation& op) const
    {
        _db.modify(_bucket, [&](bucket_object& b) {
            auto reward_symbol = op.reward.symbol();
            if (SCORUM_SYMBOL == reward_symbol)
            {
                b.scr_paid_to_curators += op.reward.amount;
            }
            else if (SP_SYMBOL == reward_symbol)
            {
                b.scorumpower_paid_to_curators += op.reward.amount;
            }
        });
    }

    void operator()(const transfer_to_scorumpower_operation& op) const
    {
        _db.modify(_bucket, [&](bucket_object& b) {
            b.transfers_to_scorumpower++;
            b.scorum_transferred_to_scorumpower += op.amount.amount;
        });
    }

    void operator()(const acc_finished_vesting_withdraw_operation& op) const
    {
        const auto& account = _db.account_service().get_account(op.from_account);

        collect_withdraw_stats(account.id, account.scorumpower);
    }

    void operator()(const devpool_finished_vesting_withdraw_operation&) const
    {
        const auto& dev_pool = _db.dev_pool_service().get();

        collect_withdraw_stats(dev_pool.id, dev_pool.sp_balance);
    }

    void operator()(const acc_to_acc_vesting_withdraw_operation& op) const
    {
        collect_withdraw_stats(op.withdrawn);
    }

    void operator()(const acc_to_devpool_vesting_withdraw_operation& op) const
    {
        collect_withdraw_stats(op.withdrawn);
    }

    void operator()(const devpool_to_acc_vesting_withdraw_operation& op) const
    {
        collect_withdraw_stats(op.withdrawn);
    }

    void operator()(const devpool_to_devpool_vesting_withdraw_operation& op) const
    {
        collect_withdraw_stats(op.withdrawn);
    }

    void operator()(const proposal_virtual_operation& op) const
    {
        op.proposal_op.weak_visit([&](const development_committee_transfer_operation& op) {
            _db.modify(_bucket, [&](bucket_object& b) {
                b.transfers++;

                if (op.amount.symbol() == SCORUM_SYMBOL)
                    b.scorum_transferred += op.amount.amount;
            });
        });
    }

    void operator()(const witness_miss_block_operation& op) const
    {
        _db.modify(_bucket, [&](bucket_object& b) { b.missed_blocks[op.block_num] = op.owner; });
    }

    template <typename TSourceId> void collect_withdraw_stats(const TSourceId& source_id, const asset& source_sp) const
    {
        const auto& withdraw_scorumpower_service = _db.obtain_service<chain::dbs_withdraw_scorumpower>();

        asset vesting_withdraw_rate = asset(0, SP_SYMBOL);

        if (withdraw_scorumpower_service.is_exists(source_id))
        {
            const auto& wvo = withdraw_scorumpower_service.get(source_id);
            vesting_withdraw_rate = wvo.vesting_withdraw_rate;
        }

        _db.modify(_bucket, [&](bucket_object& b) {
            b.finished_vesting_withdrawals++;

            b.vesting_withdraw_rate_delta -= vesting_withdraw_rate.amount;
        });
    }

    void collect_withdraw_stats(const asset& withdrawn) const
    {
        _db.modify(_bucket, [&](bucket_object& b) {

            b.vesting_withdrawals_processed++;

            if (withdrawn.symbol() == SCORUM_SYMBOL)
                b.scorumpower_withdrawn += withdrawn.amount;
            else
                b.scorumpower_transferred += withdrawn.amount;
        });
    }
};

void blockchain_monitoring_plugin_impl::process_block(const bucket_object& bucket, const signed_block& b)
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

void blockchain_monitoring_plugin_impl::process_pre_operation(const bucket_object& bucket,
                                                              const operation_notification& note)
{
    auto& db = _self.database();

    note.op.weak_visit(
        [&](const delete_comment_operation& op) {
            auto comment = db.obtain_service<dbs_comment>().get(op.author, op.permlink);

            db.modify(bucket, [&](bucket_object& b) {
                if (comment.parent_author.length())
                    b.replies_deleted++;
                else
                    b.root_comments_deleted++;
            });
        },
        [&](const withdraw_scorumpower_operation& op) {
            collect_withdraw_stats(bucket, op.scorumpower, db.account_service().get_account(op.account).id);
        },
        [&](const proposal_virtual_operation& op) {
            op.proposal_op.weak_visit([&](const development_committee_withdraw_vesting_operation& proposal_op) {
                collect_withdraw_stats(bucket, proposal_op.vesting_shares, db.dev_pool_service().get().id);
            });
        });
}

void blockchain_monitoring_plugin_impl::process_post_operation(const bucket_object& bucket,
                                                               const operation_notification& o)
{
    auto& db = _self.database();

    if (!is_virtual_operation(o.op))
    {
        db.modify(bucket, [&](bucket_object& b) { b.operations++; });
    }
    o.op.visit(operation_process(db, bucket));
}

template <typename TSourceId>
void blockchain_monitoring_plugin_impl::collect_withdraw_stats(const bucket_object& bucket,
                                                               const asset& vesting_shares,
                                                               const TSourceId& source_id)
{
    auto& db = _self.database();

    auto new_vesting_withdrawal_rate = vesting_shares.amount / SCORUM_VESTING_WITHDRAW_INTERVALS;
    if (vesting_shares.amount > 0 && new_vesting_withdrawal_rate == 0)
        new_vesting_withdrawal_rate = 1;

    const auto& withdraw_scorumpower_service = db.obtain_service<chain::dbs_withdraw_scorumpower>();

    asset vesting_withdraw_rate = asset(0, SP_SYMBOL);

    if (withdraw_scorumpower_service.is_exists(source_id))
    {
        const auto& wvo = withdraw_scorumpower_service.get(source_id);
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

} // detail

blockchain_monitoring_plugin::blockchain_monitoring_plugin(application* app)
    : plugin(app)
    , _my(new detail::blockchain_monitoring_plugin_impl(*this))
{
}

blockchain_monitoring_plugin::~blockchain_monitoring_plugin()
{
}

void blockchain_monitoring_plugin::plugin_set_program_options(boost::program_options::options_description& cli,
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

void blockchain_monitoring_plugin::plugin_initialize(const boost::program_options::variables_map& options)
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

void blockchain_monitoring_plugin::plugin_startup()
{
    app().register_api_factory<blockchain_statistics_api>(API_BLOCKCHAIN_STATISTICS);
    app().register_api_factory<node_monitoring_api>(API_NODE_MONITORING);
}

const flat_set<uint32_t>& blockchain_monitoring_plugin::get_tracked_buckets() const
{
    return _my->_tracked_buckets;
}

uint32_t blockchain_monitoring_plugin::get_max_history_per_bucket() const
{
    return _my->_maximum_history_per_bucket_size;
}

uint32_t blockchain_monitoring_plugin::get_last_block_duration_microseconds() const
{
    return std::chrono::duration_cast<std::chrono::microseconds>(_my->_timer.get_last_block_duration()).count();
}
}
} // scorum::blockchain_monitoring

SCORUM_DEFINE_PLUGIN(blockchain_monitoring, scorum::blockchain_monitoring::blockchain_monitoring_plugin);
